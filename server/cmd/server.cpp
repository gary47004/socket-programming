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

int main(int argc, char *argv[]) {
  bool continued = true;

  // bzero(&serverInfo, sizeof(serverInfo));

  auto server = socket_server::SocketServer("0.0.0.0", 8900);
  std::thread t1([&]() {
    while (continued) {
      server.Accept();
    }
  });

  std::thread t2([&]() {
    while (continued) {
      server.Run();
    }
  });

  t1.join();
  t2.join();
  return 0;
}
