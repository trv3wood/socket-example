#include "response.h"
#include <spdlog/spdlog.h>
#include <string_view>

void Response::sendResponse(int socket, const std::string_view &response) {
    spdlog::debug("发送响应: {}", response);
    send(socket, response.data(), response.size(),
         MSG_DONTWAIT | MSG_NOSIGNAL);
}
