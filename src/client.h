#pragma once
// 客户端信息结构体
#include <netinet/in.h>
struct ClientInfo {
    int socket;
    sockaddr_in address;
};
