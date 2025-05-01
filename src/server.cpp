#include "server.h"
#include "clientinfo.h"
#include "threadpool.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

Server::Server()
    : m_port(8080), m_thread_limit(std::thread::hardware_concurrency()),
      m_server_fd(-1), m_logger(spdlog::default_logger_raw()),
      m_threadPool(new ThreadPool(m_thread_limit)) {
    setupSocket();
}

Server::Server(int port, unsigned limit)
    : m_port(port),
      m_thread_limit(std::min(limit, std::thread::hardware_concurrency())),
      m_server_fd(-1), m_logger(spdlog::default_logger_raw()),
      m_threadPool(new ThreadPool(m_thread_limit)) {
    setupSocket();
}

Server::~Server() {
    delete m_threadPool;
    close(m_server_fd);
    m_logger->info("服务器关闭");
    spdlog::drop_all(); // 清理所有日志
}

void Server::setupSocket() {
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
}

void Server::stop() {
    m_running = false;
    // 关闭监听socket以唤醒accept
    shutdown(m_server_fd, SHUT_RDWR);

    // 关闭所有客户端socket
    std::lock_guard<std::mutex> lock(m_mtx);
    for (int sock : client_sockets) {
        shutdown(sock, SHUT_RDWR);
    }
}
void Server::run() {
    // 设置监听socket为非阻塞
    fcntl(m_server_fd, F_SETFL, O_NONBLOCK);
    struct timeval tv;
    while (m_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(m_server_fd, &read_fds);

        tv.tv_sec = 1; // 1秒超时
        tv.tv_usec = 0;

        int activity = select(m_server_fd + 1, &read_fds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            m_logger->error("select错误");
            break;
        }
        if (FD_ISSET(m_server_fd, &read_fds)) {
            ClientInfo client;
            socklen_t addr_len = sizeof(client.address);
            client.socket =
                accept(m_server_fd, (sockaddr *)&client.address, &addr_len);

            if (client.socket >= 0) {
                // 记录客户端socket
                {
                    std::lock_guard<std::mutex> lock(m_mtx);
                    client_sockets.insert(client.socket);
                }
                m_logger->info("新客户端连接: {}",
                               inet_ntoa(client.address.sin_addr));

                // 设置客户端socket为非阻塞
                fcntl(client.socket, F_SETFL, O_NONBLOCK);

                m_threadPool->enqueue([this, client]() {
                    while (m_running) {
                        char buffer[BUFFER_SIZE];
                        int bytes_read =
                            read(client.socket, buffer, sizeof(buffer) - 1);

                        if (bytes_read > 0) {
                            buffer[bytes_read] = '\0';
                            send(client.socket, buffer, bytes_read, 0);
                        } else if (bytes_read == 0 ||
                                   (errno != EAGAIN && errno != EWOULDBLOCK)) {
                            break;
                        }

                        // 添加定期退出检查
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100));
                    }

                    // 关闭时移除socket记录
                    {
                        std::lock_guard<std::mutex> lock(m_mtx);
                        client_sockets.erase(client.socket);
                    }
                    m_logger->info("关闭客户端连接: {}",
                                   inet_ntoa(client.address.sin_addr));
                    close(client.socket);
                });
            }
        }
    }

    close(m_server_fd);
}