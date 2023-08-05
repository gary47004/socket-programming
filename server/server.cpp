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

// 786 is the length of the request
constexpr int64_t kk = 786 * 10000;
constexpr int kTimeForWaiting = 15;
constexpr int kNumOfResp = 200000;

enum State { kAdd, kErase };

struct Task {
  int fd;
  State state;
};

void PrintTime(const std::string &msg) {
  char fmt[64];
  char buf[64];
  struct timeval tv;
  struct tm *tm;

  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
  strftime(fmt, sizeof(fmt), "%H:%M:%S:%%06u", tm);
  snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
  printf("%s message: %s\n", buf, msg.c_str());
}

void GetWriteError(int i, int n) {
  if (i == -1) {
    printf("Writing error\n");
  }
  if (i < n) {
    printf("writing len error\n");
  }
}

int main(int argc, char *argv[]) {
  constexpr timeval kDefaultTimeout = timeval{.tv_sec = 0, .tv_usec = 1000};

  std::mutex mtx_;
  std::list<Task> queue_;
  std::unordered_map<int, uint64_t> sockets_and_count_;
  bool continued = true;

  char inputBuffer[2048] = {};
  char message1[] = "HEADER=header|LENGTH=00026Hello Client, I am Server!";
  char message2[] = "HEADER=header|LENGTH=00021I am fine, thank you!";

  // socket的建立
  int sockfd = 0, forClientSockfd = 0;
  // not sure this works
  close(sockfd);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Fail to create a socket.\n");
  }

  // socket的連線
  struct sockaddr_in serverInfo, clientInfo;
  socklen_t addrlen = sizeof(clientInfo);
  bzero(&serverInfo, sizeof(serverInfo));

  serverInfo.sin_family = PF_INET;
  serverInfo.sin_addr.s_addr = inet_addr("0.0.0.0");
  serverInfo.sin_port = htons(8900);
  int err = bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
  if (err == -1) {
    printf("Binding eror\n");
  }
  err = listen(sockfd, 500);
  if (err != 0) {
    printf("Listening error\n");
  }

  std::thread t1([&]() {
    while (continued) {
      forClientSockfd =
          accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen);
      // Socket在港口等了又等，終於有客人拜訪了，我們可以用函式accept()去接見這名客人。當accept()被調用時，它會為該請求產生出一個新的Socket，並把這個請求從監聽隊列剔除掉。

      std::lock_guard<std::mutex> lg(mtx_);
      queue_.push_back({forClientSockfd, State::kAdd});
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
        if (t.state == State::kAdd) {
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
      timeval timeout = kDefaultTimeout;
      int error = select(max_fd + 1, &sk_set, NULL, NULL, &timeout);
      if (error == -1) {
        printf("select error\n");
      }
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
          // client distconnects
          printf("reading error len\n");
          std::lock_guard<std::mutex> lg(mtx_);
          queue_.push_back({fd, State::kErase});
          continue;
        }
        count += n;
        sockets_and_count_[fd] = count;
        if (count <= 574) {
          error = write(fd, message1, sizeof(message1));
          GetWriteError(error, sizeof(message1));
        }
        if ((count - 574) % kk == 0) {
          PrintTime("count: " + std::to_string(count));
        }
      }
    }
  });

  std::thread t3([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(kTimeForWaiting));
    int error = 0;
    std::lock_guard<std::mutex> lg(mtx_);
    PrintTime("TEST START");
    for (int i = 0; i < kNumOfResp; i++) {
      for (auto [fd, c] : sockets_and_count_) {
        error = write(fd, message2, sizeof(message2));
        GetWriteError(error, sizeof(message2));
      }
    }
  });

  t1.join();
  t2.join();
  t3.join();
  close(forClientSockfd);
  close(sockfd);
  return 0;
}
