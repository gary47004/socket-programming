#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "socket-server/socket_server.h"

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

int main(int argc, char *argv[]) {
  std::mutex mtx_;
  std::list<socket_server::Task> queue_;
  std::unordered_map<int, uint64_t> sockets_and_count_;
  bool continued = true;

  char inputBuffer[2048] = {};
  char message1[] = "8=header|9=00010|35=0|36=A";
  char message2[] = "8=header|9=00010|35=1|36=B";

  // socket的建立
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  CheckValidity(sockfd, "fail to create a socket");

  // socket的連線
  struct sockaddr_in serverInfo, clientInfo;
  socklen_t addrlen = sizeof(clientInfo);
  bzero(&serverInfo, sizeof(serverInfo));

  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = inet_addr("0.0.0.0");
  serverInfo.sin_port = htons(8900);
  int err = bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
  CheckValidity(err, "falied to bind");
  err = listen(sockfd, 500);
  CheckValidity(err, "falied to listen");

  std::thread t1([&]() {
    while (continued) {
      int forClientSockfd =
          accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen);
      // Socket在港口等了又等，終於有客人拜訪了，我們可以用函式accept()去接見這名客人。當accept()被調用時，它會為該請求產生出一個新的Socket，並把這個請求從監聽隊列剔除掉。

      std::lock_guard<std::mutex> lg(mtx_);
      queue_.push_back({forClientSockfd, socket_server::State::kAdd});
    }
  });

  std::thread t2([&]() {
    while (continued) {
      while (true) {
        std::lock_guard<std::mutex> lg(mtx_);
        if (queue_.empty()) {
          break;
        }
        auto t = queue_.front();
        queue_.pop_front();
        if (t.state == socket_server::State::kAdd) {
          sockets_and_count_[t.fd] = 0;
          PrintTime("fd: " + std::to_string(t.fd));
        } else {
          auto count = sockets_and_count_[t.fd];
          PrintTime("fd: " + std::to_string(t.fd) +
                    " count: " + std::to_string(count));
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
        memset(inputBuffer, 0, sizeof(inputBuffer));
        n = read(fd, inputBuffer, sizeof(inputBuffer));
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
        if (count <= 574) {
          error = write(fd, message1, sizeof(message1));
          CheckValidity(error, sizeof(message1), "failed to write");
        }
      }
    }
  });

  t1.join();
  t2.join();
  close(sockfd);
  return 0;
}
