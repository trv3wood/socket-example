#pragma once
#include <string_view>
#include <sys/socket.h>
class Response {
 public:
    static void sendResponse(int socket, const std::string_view &response);
    constexpr static std::string_view PEND =
        "150 File status okay; about to open data connection\r\n";
    constexpr static std::string_view READY =
        "220 Service ready for new user\r\n";
    constexpr static std::string_view CLOSECTRL =
        "221 Service closing control connection. Logged out if "
        "appropriate.\r\n";
    constexpr static std::string_view CLOSEDATACONN =
        "226 Closing data connection\r\n";
    constexpr static std::string_view LOGGED =
        "230 User logged in, proceed\r\n";
    constexpr static std::string_view FILEACTOK =
        "250 Requested file action was okay, completed\r\n";
    constexpr static std::string_view NEEDPASS =
        "331 User name okay, password needed.\r\n";
    constexpr static std::string_view FAILDATACONN =
        "425 Can't open data connection.\r\n";
    constexpr static std::string_view BADSEQ =
        "503 Bad sequence of commands\r\n";
    constexpr static std::string_view NOLOG = "530 not logged in.\r\n";
    constexpr static std::string_view NOTIMPL =
        "502 Command not implemented\r\n";
    constexpr static std::string_view FILEUNAVAIL =
        "550 Requested action not taken. File unavailable\r\n";
};