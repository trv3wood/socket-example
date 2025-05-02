#include "server.h"
#include <iostream>
#if SOCKETEXAMPLE_DEBUG
const int PORT = 8080; // 服务器端口
#else
const int PORT = 21; // 服务器端口
#endif

int main() {
    Server server(PORT);
    std::jthread t([&server]() { server.run(); });
    std::string command;
    while (std::cin >> command) {
        if (command == "q") {
            std::cout << "正在等待任务完成..." << std::endl;
            server.stop();
            t.request_stop();
            break;
        }
    }
}
