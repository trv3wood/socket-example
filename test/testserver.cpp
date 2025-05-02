#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#if SOCKETEXAMPLE_DEBUG
const int PORT = 8080; // 服务器端口
#else
const int PORT = 21; // 服务器端口
#endif
void client_thread(int client_id) {
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "客户端" << client_id << ": 创建socket失败" << std::endl;
        return;
    }

    // 设置服务器地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接服务器
    if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "客户端" << client_id << ": 连接失败" << std::endl;
        close(sock);
        return;
    }

    // 发送请求
    std::string msg = "请求来自客户端" + std::to_string(client_id);
    if (send(sock, msg.c_str(), msg.size(), 0) == -1) {
        std::cerr << "客户端" << client_id << ": 发送失败" << std::endl;
    }

    // 接收响应
    char buffer[1024];
    int bytes = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "客户端" << client_id << "收到: " << buffer << std::endl;
    }

    close(sock);
}

void async_clients(int num_clients) {
    std::vector<int> sockets;
    std::vector<pollfd> pollfds;
    // 创建所有socket并设置为非阻塞
    for (int i = 0; i < num_clients; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sock == -1) {
            std::cerr << "创建socket失败" << std::endl;
            continue;
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            if (errno != EINPROGRESS) {
                std::cerr << "连接失败" << std::endl;
                close(sock);
                continue;
            }
        }

        pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT; // 监控可写事件(连接完成)
        pollfds.push_back(pfd);
        sockets.push_back(sock);
    }
    // 轮询检查连接状态
    int remaining = num_clients;
    while (remaining > 0) {
        int ready = poll(pollfds.data(), pollfds.size(), 1000); // 1秒超时
        if (ready <= 0)
            continue;
        for (size_t i = 0; i < pollfds.size(); ++i) {
            if (pollfds[i].revents & POLLOUT) {
                // 连接成功，发送数据
                std::string msg = "异步请求" + std::to_string(i);
                send(pollfds[i].fd, msg.c_str(), msg.size(), 0);

                // 改为监控可读事件(响应)
                pollfds[i].events = POLLIN;
                remaining--;
            }
        }
    }
    // 关闭所有socket
    for (int sock : sockets) {
        close(sock);
    }
}

int main() {
    const int NUM_CLIENTS = 10; // 并发客户端数量
    std::vector<std::thread> threads;

    // 创建多个客户端线程
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        threads.emplace_back(client_thread, i);
    }

    auto start = std::chrono::high_resolution_clock::now();
    // 等待所有线程完成
    for (auto &t : threads) {
        t.join();
    }
    std::cout << std::format("所有客户端完成，耗时: {}秒\n",
                 std::chrono::duration<double>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count())
              << std::endl;
    
    async_clients(100);
    std::cout << std::format("异步客户端完成，耗时: {}秒\n",
                 std::chrono::duration<double>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count())
              << std::endl;

    return 0;
}
