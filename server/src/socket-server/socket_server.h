#ifndef SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_
#define SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_

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
#include <unordered_map>

namespace socket_server {

enum State { kAdd, kErase };

struct Task {
  int fd;
  State state;
};

class SocketServer {
 public:
  SocketServer(const std::string &ip, int port);
  ~SocketServer();

  void Run();
  void Accept();

 private:
  void HandleTasks();
  void HandleAddTask(int fd);
  void HandleEraseTask(int fd);
  void ReceiveAndSend();
  bool Read(int fd);
  void Write(int fd);
  void CloseAllClientFd();

  int server_fd_;
  std::mutex mtx_;
  std::list<Task> queue_;
  std::unordered_map<int, uint64_t> client_fd_and_msg_count_;
  char buffer_[2048];
};

void PrintTime(const std::string &msg);

void CheckValidity(int value, int expected, const std::string &msg);

void CheckValidity(int value, const std::string &msg);
}  // namespace socket_server

#endif  // SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_
