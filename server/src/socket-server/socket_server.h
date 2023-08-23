#ifndef SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_
#define SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_

#include <arpa/inet.h>
#include <fcntl.h>
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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace socket_server {

class EnhancedSocket {
 public:
  explicit EnhancedSocket(int fd);
  ~EnhancedSocket();

  bool Read();
  void Write();
  int GetMessageCount();

 private:
  std::vector<std::string> GetCompleteMessages(const std::string &msg);
  bool FindHeader();

  int fd_;
  int message_count_;
  std::string buffer_;
  size_t remaining_len_;
};

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
  void InitTaskNotificationPipe();
  void CloseTaskNotificationPipe();
  void NotifyAddTaskCreated();
  void HandleTasks();
  void HandleAddTask(int fd);
  void HandleEraseTask(int fd);
  void ReceiveAndSend();

  int pipe_read_fd_;
  int pipe_write_fd_;
  int server_fd_;
  std::mutex mtx_;
  std::list<Task> queue_;
  std::map<int, std::shared_ptr<EnhancedSocket>> enhanced_sockets_;
};

void PrintTime(const std::string &msg);

void CheckValidity(int value, int expected, const std::string &msg);

void CheckValidity(int value, const std::string &msg);
}  // namespace socket_server

#endif  // SOCKET_PROGRAMMING_SERVER_SRC_SOCKET_SERVER_SOCKET_SERVER_H_
