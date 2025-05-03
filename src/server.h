#pragma once
#include "clientinfo.h"
#include "threadpool.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <thread>
#include <unordered_set>

class Server {
 public:
    Server(int port, unsigned limit = std::thread::hardware_concurrency() * 2);
    ~Server();
    void run();
    void stop();

 private:
    void setupServerSocket();
    void processClient(const ClientInfo &client);
    void setupEpoll();
    void handleNewConnections();
    void cleanUp();
    int m_server_fd;
    int m_epoll_fd;
    int m_port;
    static const int MAX_EVENTS = 64;
    unsigned int m_thread_limit;
    std::atomic<bool> m_running = true;
    std::mutex m_mtx;
    ThreadPool m_threadPool;
    std::filesystem::path m_working_dir;
    std::unordered_set<int> m_client_sockets;
};
