#include "server.h"
#include "clientinfo.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <condition_variable>
#include <cstdlib>
#include <format>
#include <mutex>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// 全局互斥锁，用于保护共享资源
std::mutex mtx;
std::condition_variable cv;

const int BUFFER_SIZE = 1024;

Server::Server()
    : m_port(8080), m_thread_limit(std::thread::hardware_concurrency()),
      m_server_fd(-1), m_logger(spdlog::default_logger_raw()) {
    setupSocket();
}

Server::Server(int port, unsigned limit)
    : m_port(port),
      m_thread_limit(std::min(limit, std::thread::hardware_concurrency())),
      m_server_fd(-1), m_logger(spdlog::default_logger_raw()) {
    setupSocket();
}

Server::~Server() {
    // 等待所有线程完成
    for (auto &threadInfo : m_threads) {
        if (std::thread &t = threadInfo.thread; t.joinable()) {
            t.join();
        }
    }
    close(m_server_fd);
    m_logger->info("服务器关闭");
    spdlog::drop_all(); // 清理所有日志
}

Server &Server::setupSocket() {
    // 创建socket
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        m_logger->error("无法创建socket");
        exit(1);
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        m_logger->error("设置socket选项失败");
        exit(1);
    }

    // 绑定地址和端口
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);

    if (bind(m_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        m_logger->error("绑定失败");
        exit(1);
    }

    // 开始监听
    if (listen(m_server_fd, 5) < 0) {
        m_logger->error("监听失败");
        exit(1);
    }

    m_logger->info("服务器正在监听端口 {}", m_port);
    return *this;
}

void Server::run() {
    for (;;) {
        // 接受客户端连接
        ClientInfo client;
        socklen_t addr_len = sizeof(client.address);
        client.socket =
            accept(m_server_fd, (struct sockaddr *)&client.address, &addr_len);

        if (client.socket < 0) {
            m_logger->error("接受连接失败");
            continue;
        }

        cleanupThreads();
        // 创建线程处理客户端
        try {
            m_threads.emplace_back();
            auto &threadInfo = m_threads.back();
            threadInfo.thread = std::thread([this, &client, &threadInfo]() {
                this->worker(threadInfo, client);
                // 线程完成后通知主线程
                threadInfo.finished = true;
                this->infoThreads();
                cv.notify_one();
            });

        } catch (const std::exception &e) {
            m_logger->error("创建线程失败: {}", e.what());
            close(client.socket);
        }
        // 如果线程数达到上限，等待
        while (m_threads.size() > m_thread_limit) {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this]() {
                return std::any_of(
                    m_threads.begin(), m_threads.end(),
                    [](const ThreadInfo &t) { return t.finished; });
            });
            cleanupThreads();
        }
    }
}

// 线程函数，处理客户端连接
void Server::worker(ThreadInfo &threadInfo, const ClientInfo &client) {
    char buffer[BUFFER_SIZE] = {0};
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(client.address.sin_addr), client_ip, INET_ADDRSTRLEN);

    std::ostringstream thread_id_stream;
    thread_id_stream << std::this_thread::get_id();
    m_logger->info("线程 {} 处理客户端: {}", thread_id_stream.str(), client_ip);

    while (true) {
        // 读取客户端数据
        int bytes_read = read(client.socket, buffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            m_logger->info("客户端 {} 断开连接", client_ip);
            break;
        }

        buffer[bytes_read - 1] = '\0';
        m_logger->info("从 {} 收到: {}, {} bytes", client_ip, buffer,
                       bytes_read);

        // 将数据原样返回给客户端
        if (send(client.socket, buffer, bytes_read, 0) < 0) {
            m_logger->error("发送数据失败");
            break;
        }
    }

    // 关闭客户端socket
    close(client.socket);
}

// 清理已完成的线程
void Server::cleanupThreads() {
    for (auto it = m_threads.begin(); it != m_threads.end();) {
        if (it->finished) {
            if (it->thread.joinable()) {
                it->thread.join(); // 必须join才能安全销毁
            }
            it = m_threads.erase(it); // 从列表中移除
        } else {
            ++it;
        }
    }
}

void Server::infoThreads() const {
    for (const auto &threadInfo : m_threads) {
        m_logger->info("线程 {} 状态: {}",
                       std::format("{:x}", std::hash<std::thread::id>{}(
                                               threadInfo.thread.get_id())),
                       threadInfo.finished ? "完成" : "运行中");
    }
    m_logger->info("当前线程数: {}, 最大线程数: {}", m_threads.size(),
                   m_thread_limit);
}