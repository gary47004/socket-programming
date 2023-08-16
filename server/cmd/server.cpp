#include <thread>

#include "socket-server/socket_server.h"

constexpr char kServerIP[] = "0.0.0.0";
constexpr int kServerPort = 8900;

int main(int argc, char *argv[]) {
  bool continued = true;
  auto server = socket_server::SocketServer(kServerIP, kServerPort);

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
