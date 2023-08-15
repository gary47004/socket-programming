#include "socket-server/socket_server.h"

namespace socket_server {
constexpr timeval kDefaultTimeout = timeval{.tv_sec = 0, .tv_usec = 1000};
constexpr char kMessage[] = "8=header|9=00010|35=0|36=A";

SocketServer::SocketServer(const std::string &ip, int port) {
  // socket的建立
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  CheckValidity(server_fd_, "fail to create a socket");

  // socket的連線
  sockaddr_in addr;  // member?
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());
  CheckValidity(
      bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)),
      "failed to bind");
  CheckValidity(listen(server_fd_, 5), "failed to listen");
}

SocketServer::~SocketServer() { close(server_fd_); }

void SocketServer::Run() {
  while (true) {
    std::lock_guard<std::mutex> lg(mtx_);
    if (queue_.empty()) {
      break;
    }
    auto t = queue_.front();
    queue_.pop_front();
    if (t.state == socket_server::State::kAdd) {
      sockets_and_count_[t.fd] = 0;
      PrintTime("client connects, fd: " + std::to_string(t.fd));
    } else {
      auto count = sockets_and_count_[t.fd];
      PrintTime("client disconnects, fd: " + std::to_string(t.fd) +
                ", count: " + std::to_string(count));
      close(t.fd);
      sockets_and_count_.erase(t.fd);
    }
  }
  fd_set sk_set;
  FD_ZERO(&sk_set);
  int max_fd = 0;
  for (auto [fd, c] : sockets_and_count_) {
    FD_SET(fd, &sk_set);
    if (max_fd < fd) {
      max_fd = fd;
    }
  }
  timeval timeout = socket_server::kDefaultTimeout;
  int error = select(max_fd + 1, &sk_set, NULL, NULL, &timeout);
  CheckValidity(error, "failed to select");
  ssize_t n = 0;
  for (auto [fd, count] : sockets_and_count_) {
    if (!FD_ISSET(fd, &sk_set)) {
      continue;
    }
    memset(buffer_, 0, sizeof(buffer_));
    n = read(fd, buffer_, sizeof(buffer_));
    if (n == -1) {
      printf("failed to read\n");
      continue;
    }

    if (n == 0) {
      printf("client disconnects\n");
      std::lock_guard<std::mutex> lg(mtx_);
      queue_.push_back({fd, socket_server::State::kErase});
      continue;
    }
    count += n;
    sockets_and_count_[fd] = count;
    error = write(fd, kMessage, sizeof(kMessage));
    CheckValidity(error, sizeof(kMessage), "failed to write");
  }
}

void SocketServer::Accept() {
  sockaddr_in client_info;
  socklen_t len = sizeof(client_info);
  int client_fd =
      accept(server_fd_, reinterpret_cast<sockaddr *>(&client_info), &len);
  // 有client連線，我們可以用函式accept()去接受此client。當accept()被調用時，它會為該請求產生出一個新的Socket，並把這個請求從監聽隊列剔除掉。

  std::lock_guard<std::mutex> lg(mtx_);
  queue_.push_back({client_fd, State::kAdd});
}

void PrintTime(const std::string &msg) {
  char fmt[64];
  char buf[64];
  timeval tv;
  tm tm;

  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &tm);
  strftime(fmt, sizeof(fmt), "%H:%M:%S:%%06u", &tm);
  snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
  printf("%s message: %s\n", buf, msg.c_str());
}

void CheckValidity(int value, int expected, const std::string &msg) {
  if (value == expected) {
    return;
  }
  printf("%s\n", msg.c_str());
  exit(-1);
}

void CheckValidity(int value, const std::string &msg) {
  if (value >= 0) {
    return;
  }
  CheckValidity(value, 0, msg);
}
}  // namespace socket_server
