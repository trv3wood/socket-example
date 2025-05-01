#pragma once
#include "clientinfo.h"
#include <list>
#include <mutex>
#include <spdlog/logger.h>
#include <thread>

struct ThreadInfo {
    std::thread thread;
    bool finished = false;
};
class Server {
  public:
    Server();
    Server(int port, unsigned limit = std::thread::hardware_concurrency());
    ~Server();
    Server &set_log_level(spdlog::level::level_enum level) {
        m_logger->set_level(level);
        return *this;
    }
    void run();

  private:
    Server &setupSocket();
    int m_port;
    int m_server_fd;
    unsigned int m_thread_limit;
    spdlog::logger *m_logger = nullptr;
    std::list<ThreadInfo> m_threads;

    void cleanupThreads();

    void worker(ThreadInfo &threadInfo, const ClientInfo &);
    template <typename F> static void blocking(std::mutex &mtx, F &&f) {
        std::unique_lock lock(mtx);
        f();
    }
    void infoThreads() const;
};
