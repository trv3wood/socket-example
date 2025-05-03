#include "server.h"
#include "clientinfo.h"
#include "clientsession.h"
#include "threadpool.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

Server::Server(int port, unsigned limit)
    : m_port(port),
      m_thread_limit(std::min(limit, std::thread::hardware_concurrency()) * 2),
      m_server_fd(-1), m_threadPool(m_thread_limit), m_working_dir("/var/ftp") {
    if (!std::filesystem::exists(m_working_dir) ||
        !std::filesystem::is_directory(m_working_dir)) {
        spdlog::error("工作目录不存在或不是目录: {}", m_working_dir.string());
        exit(1);
    }
#if SOCKETEXAMPLE_DEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif
    setupServerSocket();
}

Server::~Server() {
    cleanUp();
    spdlog::info("服务器关闭");
}

void Server::setupServerSocket() {
    // 创建socket
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        spdlog::error("无法创建socket");
        exit(1);
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        spdlog::error("设置socket选项失败");
        exit(1);
    }

    // 绑定地址和端口
    sockaddr_in address{AF_INET, htons(m_port), INADDR_ANY};

    if (bind(m_server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        spdlog::error("绑定失败");
        exit(1);
    }

    // 开始监听
    if (listen(m_server_fd, 1024) < 0) {
        spdlog::error("监听失败");
        exit(1);
    }

    spdlog::info("服务器正在监听端口 {} 按q退出", m_port);
}

void Server::stop() {
    m_running = false;
    // 关闭监听socket以唤醒accept
    shutdown(m_server_fd, SHUT_RDWR);

    // 关闭所有客户端socket
    std::lock_guard<std::mutex> lock(m_mtx);
    for (int sock : m_client_sockets) { shutdown(sock, SHUT_RDWR); }
}
void Server::run() {
    setupEpoll();
    struct epoll_event events[MAX_EVENTS];
    while (m_running) {
        int num_events =
            epoll_wait(m_epoll_fd, events, MAX_EVENTS, 500); // 500ms超时

        if (num_events == -1) {
            if (errno == EINTR) continue;
            spdlog::error("epoll错误: {}", strerror(errno));
            break;
        }
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == m_server_fd) { handleNewConnections(); }
        }
    }
}

void Server::processClient(const ClientInfo &client) {
    ClientSession session(client.socket, m_working_dir.string());
    session.start();
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    event.data.fd = client.socket;
    if (epoll_wait(m_epoll_fd, &event, 1, 100) == 0) { // 100ms超时
        if (!m_running) return;
        // 检查停止标志
    }
    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, client.socket, nullptr);
    // 关闭时移除socket记录
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_client_sockets.erase(client.socket);
    }
    spdlog::debug("关闭客户端连接: {}", inet_ntoa(client.address.sin_addr));
}

void Server::setupEpoll() {
    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd == -1) { throw std::runtime_error("epoll创建失败"); }
    // 添加服务器socket到epoll
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 边缘触发模式
    event.data.fd = m_server_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &event) == -1) {
        close(m_epoll_fd);
        throw std::runtime_error("epoll注册失败");
    }
}
void Server::handleNewConnections() {
    while (m_running) { // 边缘触发需要循环accept
        ClientInfo client;
        socklen_t addr_len = sizeof(client.address);
        client.socket =
            accept4(m_server_fd, (sockaddr *)&client.address, &addr_len, 0);

        if (client.socket == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            continue;
        }
        // 注册客户端socket到epoll
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        event.data.fd = client.socket;
        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client.socket, &event) == -1) {
            spdlog::error("客户端注册失败: {}", strerror(errno));
            close(client.socket);
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_client_sockets.insert(client.socket);
        }

        m_threadPool.enqueue([this, client]() { processClient(client); });
    }
}
void Server::cleanUp() {
    std::vector<int> sockets;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        sockets.assign(m_client_sockets.begin(), m_client_sockets.end());
    }

    for (int sock : sockets) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }

    close(m_epoll_fd);
    close(m_server_fd);
}
