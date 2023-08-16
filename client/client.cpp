#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <exception>
#include <thread>

constexpr char kGreetingMessage[] = "8=header|9=00010|35=0|36=A";
constexpr char kMessage[] = "8=header|9=00010|35=1|36=B";
constexpr char kServerIP[] = "0.0.0.0";
constexpr int kServerPort = 8900;

int main(int argc, char *argv[]) {
  // socket的建立
  int fd = 0;
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    printf("failed to create a socket\n");
    exit(-1);
  }

  // socket的連線
  sockaddr_in info;
  info.sin_family = AF_INET;
  info.sin_port = htons(kServerPort);
  info.sin_addr.s_addr = inet_addr(kServerIP);
  int error = connect(fd, reinterpret_cast<sockaddr *>(&info), sizeof(info));
  if (error == -1) {
    printf("failed to connect\n");
    exit(-1);
  }

  char received_msg[2048] = {};
  ssize_t n = write(fd, kGreetingMessage, sizeof(kGreetingMessage));
  printf("sending size: %ld\n", n);
  n = read(fd, received_msg, sizeof(received_msg));
  printf("reading size: %ld, message: %s\n", n, received_msg);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    memset(received_msg, 0, sizeof(received_msg));

    n = write(fd, kMessage, sizeof(kMessage));
    printf("sending size: %ld\n", n);
    n = read(fd, received_msg, sizeof(received_msg));
    printf("reading size: %ld, message: %s\n", n, received_msg);
  }
  close(fd);
  return 0;
}
