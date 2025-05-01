#include "server.h"
#include <iostream>
#include <spdlog/spdlog.h>

int main() {
    Server server(8080, 4);
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
