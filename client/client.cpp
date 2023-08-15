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
  int sockfd = 0;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("Fail to create a socket.\n");
    exit(-1);
  }

  // socket的連線
  struct sockaddr_in info;
  bzero(&info, sizeof(info));
  info.sin_family = AF_INET;
  info.sin_addr.s_addr = inet_addr(kServerIP);
  info.sin_port = htons(kServerPort);
  int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
  if (err == -1) {
    printf("connection error\n");
    exit(-1);
  }

  char receivedMessage[2048] = {};
  ssize_t n = write(sockfd, kGreetingMessage, sizeof(kGreetingMessage));
  printf("sending size: %ld\n", n);
  n = read(sockfd, receivedMessage, sizeof(receivedMessage));
  printf("reading size: %ld, message: %s\n", n, receivedMessage);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    memset(receivedMessage, 0, sizeof(receivedMessage));

    n = write(sockfd, kMessage, sizeof(kMessage));
    printf("sending size: %ld\n", n);
    n = read(sockfd, receivedMessage, sizeof(receivedMessage));
    if (n == 0) {
      break;
    }
    printf("reading size: %ld, message: %s\n", n, receivedMessage);
  }
  close(sockfd);
  return 0;
}
