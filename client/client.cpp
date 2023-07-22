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

int main(int argc, char *argv[]) {
  // FIX protocol?
  char message1[] = "HEADER=header|LENGTH=00026Hello Server, I am Client!";
  char message2[] = "HEADER=header|LENGTH=00012How are you?";

  // socket的建立
  int sockfd = 0;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd == -1) {
    printf("Fail to create a socket.\n");
  }

  // socket的連線

  struct sockaddr_in info;
  bzero(&info, sizeof(info));
  info.sin_family = AF_INET;

  // localhost test
  info.sin_addr.s_addr = inet_addr("0.0.0.0");
  info.sin_port = htons(8900);

  int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
  if (err == -1) {
    printf("Connection error\n");
  }

  char receiveMessage[2048] = {};
  ssize_t n = write(sockfd, message1, sizeof(message1));
  printf("Sending n: %ld\n", n);
  n = read(sockfd, receiveMessage, sizeof(receiveMessage));
  printf("Reading n: %ld\n", n);
  printf("%s\n\n", receiveMessage);
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    memset(receiveMessage, 0, sizeof(receiveMessage));

    n = write(sockfd, message2, sizeof(message2));
    printf("Sending n: %ld\n", n);
    n = read(sockfd, receiveMessage, sizeof(receiveMessage));
    if (n == 0) {
      break;
    }
    printf("Reading n: %ld\n", n);
    printf("%s\n\n", receiveMessage);
  }
  close(sockfd);
  return 0;
}
