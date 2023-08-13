#ifndef SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_H_
#define SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_H_

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

namespace socket_server {
constexpr timeval kDefaultTimeout = timeval{.tv_sec = 0, .tv_usec = 1000};

enum State { kAdd, kErase };

struct Task {
  int fd;
  State state;
};
/*
class SocketServer {
 public:
  SocketServer(const std::string &ip, int port);
  ~SocketServer();

  void Run();

 private:
  void AddTask(int fd);
  void EraseTask(int fd);
  void HandleTask(const Task &task);
  void HandleAccept(int fd);
  void HandleRead(int fd);
  void HandleWrite(int fd);
  void HandleError(int fd);
  void HandleTimeout(int fd);
  void HandleClose(int fd);
  void HandleException(int fd);
  void HandleUnknown(int fd);
  void Handle(int fd, int events);
  void Handle();

  std::mutex mtx_;
  std::list<Task> queue_;
  std::unordered_map<int, uint64_t> sockets_and_count_;
  std::string ip_;
  int port_;
  int sockfd_;
  fd_set readfds_;
  fd_set writefds_;
  fd_set exceptfds_;
  timeval timeout_;
};*/
}  // namespace socket_server

#endif  // SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_H_
