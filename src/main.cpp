#include "server.h"
#include <iostream>

int main() {
    Server server;
    server.set_log_level(spdlog::level::info);
    std::thread t([&server]() { server.run(); });
    t.detach();
    std::string command;
    while (std::cin >> command) {
        if (command == "q") {
            std::cout << "正在等待任务完成..." << std::endl;
            break;
        }
    }
}
