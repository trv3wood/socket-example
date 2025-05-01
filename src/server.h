#pragma once
#include "threadpool.h"
#include <atomic>
#include <mutex>
#include <spdlog/logger.h>
#include <thread>
#include <unordered_set>

class Server {
  public:
    Server();
    Server(int port, unsigned limit = std::thread::hardware_concurrency());
    ~Server();
    void run();
    void stop();

  private:
    void setupSocket();
    int m_server_fd;
    unsigned int m_thread_limit;
    int m_port;
    std::atomic<bool> m_running = true;
    std::mutex m_mtx;
    spdlog::logger *m_logger = nullptr;
    ThreadPool *m_threadPool;
    std::unordered_set<int> client_sockets;
};
