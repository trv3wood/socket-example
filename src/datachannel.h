#pragma once
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>
class DataChannel {
 public:
    DataChannel() { spdlog::debug("DataChannel创建"); }
    void setup();
    int port() const;
    void reset();

    ~DataChannel();

    int m_server_sock = -1;
    int m_data_sock = -1;                       // 数据连接socket
    sockaddr_in m_addr{AF_INET, 0, INADDR_ANY}; // 数据连接地址
    enum DataConnMode {
        INACTIVE,   // 无数据连接
        PASV_READY, // PASV模式已准备
        PORT_READY  // PORT模式已准备
    } m_conn_mode = INACTIVE;
};
