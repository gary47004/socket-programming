#include "socket-server/socket_server.h"

namespace socket_server {
constexpr timeval kDefaultTimeout = timeval{.tv_sec = 5, .tv_usec = 0};
constexpr char kMessage[] = "8=header|9=00010|35=0|36=A";
constexpr int kMaxClientNum = 100;
constexpr char kNoitfyingMessage[] = "*";

SocketServer::SocketServer(const std::string &ip, int port) {
  InitTaskNotificationPipe();

  // socket的建立
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  CheckValidity(server_fd_, "fail to create a socket");

  // socket的連線
  sockaddr_in addr;
  addr.sin_family = PF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());
  CheckValidity(
      bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)),
      "failed to bind");
  CheckValidity(listen(server_fd_, kMaxClientNum), "failed to listen");
}

SocketServer::~SocketServer() {
  CloseAllClientFd();
  close(server_fd_);
  CloseTaskNotificationPipe();
}

void SocketServer::Run() {
  HandleTasks();
  ReceiveAndSend();
}

void SocketServer::Accept() {
  sockaddr_in client_info;
  socklen_t len = sizeof(client_info);
  int client_fd =
      accept(server_fd_, reinterpret_cast<sockaddr *>(&client_info), &len);
  // 有client連線，我們可以用函式accept()去接受此client。當accept()被調用時，它會為該請求產生出一個新的Socket，並把這個請求從監聽隊列剔除掉。

  std::lock_guard<std::mutex> lg(mtx_);
  queue_.push_back(Task{.fd = client_fd, .state = State::kAdd});
  NotifyAddTaskCreated();
}

void SocketServer::InitTaskNotificationPipe() {
  int fds[2];
  if (pipe(fds) != 0) {
    printf("Cannot create a pipe.\n");
    exit(-1);
  }
  pipe_read_fd_ = fds[0];
  pipe_write_fd_ = fds[1];

  if (fcntl(pipe_write_fd_, F_SETFL,
            fcntl(pipe_write_fd_, F_GETFL) | O_NONBLOCK) < 0) {
    printf("Cannot set the write fd of the pipe to non-blocking.\n");
    exit(-1);
  }
  if (fcntl(pipe_write_fd_, F_SETFD,
            fcntl(pipe_write_fd_, F_GETFD) | FD_CLOEXEC) < 0) {
    printf("Cannot set the write fd of the pipe to close on exec.\n");
    exit(-1);
  }
  if (fcntl(pipe_read_fd_, F_SETFD,
            fcntl(pipe_read_fd_, F_GETFD) | FD_CLOEXEC) < 0) {
    printf("Cannot set the read fd of the pipe to close on exec.\n");
    exit(-1);
  }
}

void SocketServer::CloseTaskNotificationPipe() {
  close(pipe_read_fd_);
  close(pipe_write_fd_);
}

void SocketServer::NotifyAddTaskCreated() {
  ssize_t n =
      write(pipe_write_fd_, &kNoitfyingMessage, sizeof(kNoitfyingMessage));
  CheckValidity(n, sizeof(kNoitfyingMessage), "failed to write to pipe");
}

void SocketServer::CloseAllClientFd() {
  for (auto [fd, count] : client_fd_and_msg_count_) {
    close(fd);
  }
}

void SocketServer::HandleTasks() {
  while (true) {
    std::lock_guard<std::mutex> lg(mtx_);
    if (queue_.empty()) {
      break;
    }
    auto t = queue_.front();
    queue_.pop_front();
    if (t.state == State::kAdd) {
      HandleAddTask(t.fd);
    } else {
      HandleEraseTask(t.fd);
    }
  }
}

void SocketServer::HandleAddTask(int fd) {
  client_fd_and_msg_count_[fd] = 0;
  PrintTime("client connects, fd: " + std::to_string(fd));
}

void SocketServer::HandleEraseTask(int fd) {
  auto count = client_fd_and_msg_count_[fd];
  PrintTime("client disconnects, fd: " + std::to_string(fd) +
            ", message count: " + std::to_string(count));
  close(fd);
  client_fd_and_msg_count_.erase(fd);
}

void SocketServer::ReceiveAndSend() {
  fd_set sk_set;
  FD_ZERO(&sk_set);
  int max_fd = pipe_read_fd_;
  FD_SET(pipe_read_fd_, &sk_set);
  for (auto [fd, count] : client_fd_and_msg_count_) {
    FD_SET(fd, &sk_set);
    if (max_fd < fd) {
      max_fd = fd;
    }
  }
  timeval timeout = kDefaultTimeout;
  int error = select(max_fd + 1, &sk_set, NULL, NULL, &timeout);
  CheckValidity(error, "failed to select");
  if (FD_ISSET(pipe_read_fd_, &sk_set)) {
    memset(buffer_, 0, sizeof(buffer_));
    size_t n = read(pipe_read_fd_, buffer_, sizeof(buffer_));
    if (n == -1) {
      printf("failed to read from pipe\n");
    }
  }
  for (auto [fd, count] : client_fd_and_msg_count_) {
    if (!FD_ISSET(fd, &sk_set)) {
      continue;
    }
    if (Read(fd)) {
      Write(fd);
    }
  }
}

bool SocketServer::Read(int fd) {
  memset(buffer_, 0, sizeof(buffer_));
  size_t n = read(fd, buffer_, sizeof(buffer_));
  if (n == -1) {
    printf("failed to read\n");
    return false;
  }
  if (n == 0) {
    std::lock_guard<std::mutex> lg(mtx_);
    queue_.push_back({fd, State::kErase});
    return false;
  }
  printf("reading size: %ld, message: %s\n", n, buffer_);
  client_fd_and_msg_count_[fd]++;
  return true;
}

void SocketServer::Write(int fd) {
  ssize_t n = write(fd, kMessage, sizeof(kMessage));
  CheckValidity(n, sizeof(kMessage), "failed to write");
  printf("sending size: %ld\n", n);
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
  printf("%s %s\n", buf, msg.c_str());
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
