#pragma once
#include "client.h"
#include <thread>
#include <vector>
class Server {
public:
    Server() = default;
    ~Server();
    Server& port(int port);
    Server& start();
    void run();
private:
    int m_port;
    int m_server_fd;
    std::vector<std::thread> m_threads;
    static void handleClient(ClientInfo*);
};