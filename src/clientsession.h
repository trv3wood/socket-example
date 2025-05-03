#pragma once
#include "datachannel.h"
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class ClientSession {
 public:
    ClientSession(const ClientSession &) = delete;
    ClientSession(ClientSession &&) = default;
    ClientSession &operator=(const ClientSession &) = delete;
    ClientSession &operator=(ClientSession &&) = delete;
    ClientSession(int ctrcl_socket, std::string &&wd);
    void start();
    ~ClientSession();

 private:
    bool m_connected = true;
    const int m_ctrcl_socket;
    enum State {
        WAIT_USER,
        WAIT_PASS,
        AUTHENTICATED,
        TRANSFER
    } m_state = WAIT_USER;
    DataChannel m_data_channel;

    // clang-format off
    using CommandArgs = std::vector<std::string>;
    std::string m_working_dir;
    bool setWorkingDir(std::string&& path);

    void processCommand(const std::string &raw_cmd);
    // Handle USER command
    void handleUser(const CommandArgs &args);
    // Handle PASS command
    void handlePass(const CommandArgs &args);
    // Handle QUIT command
    void handleQuit(const CommandArgs &args);
    // Handle CWD command
    void handleCwd(CommandArgs &&args);
    // Handle PWD command
    void handlePwd(const CommandArgs &args) const;
    // Handle LIST command
    void handleList(const CommandArgs &args);
    // Handle RETR command
    void handleRetr(const CommandArgs &args) ;
    // Handle STOR command
    void handleStor(const CommandArgs &args) const ;
    void handlePasv(const CommandArgs &args);
    void handlePort(const CommandArgs &args) ;
};
