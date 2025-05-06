#include "datachannel.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

void DataChannel::setup() {
    m_server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(m_server_sock, (sockaddr *)&m_addr, sizeof(m_addr)) < 0) {
        spdlog::error("绑定数据socket失败");
        return;
    }
    if (listen(m_server_sock, 1) < 0) {
        spdlog::error("监听数据socket失败");
        return;
    }
    socklen_t addr_len = sizeof(m_addr);
    if (getsockname(m_server_sock, (sockaddr *)&m_addr, &addr_len) < 0) {
        spdlog::error("获取数据socket地址失败");
        return;
    }
    m_conn_mode = PASV_READY;
}
int DataChannel::port() const { return ntohs(m_addr.sin_port); }
DataChannel::~DataChannel() {
    close(m_server_sock);
    close(m_data_sock);
    spdlog::debug("DataChannel关闭");
}

void DataChannel::reset() {
    spdlog::debug("重置数据通道");
    close(m_server_sock);
    close(m_data_sock);
    m_server_sock = -1;
    m_addr = {AF_INET, 0, INADDR_ANY};
    m_conn_mode = INACTIVE;
}
DataChannel::DataChannel(int ctrcl_socket) : m_ctrcl_socket(ctrcl_socket) {
    spdlog::debug("DataChannel创建");
}
