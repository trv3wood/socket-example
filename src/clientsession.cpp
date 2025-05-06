#include "clientsession.h"
#include "ftpcmd.h"
#include "response.h"
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

void ClientSession::start() {
    Response::sendResponse(m_ctrcl_socket, Response::READY);
    while (m_connected) {
        std::string buffer(128, '\0');
        ssize_t bytes_received =
            recv(m_ctrcl_socket, buffer.data(), buffer.size(), 0);
        if (bytes_received <= 0) {
            m_connected = false;
            spdlog::debug("ClientSession: 接收数据失败或客户端连接关闭");
            break;
        }
        buffer.resize(bytes_received);
        spdlog::debug("接收到命令: {}", buffer);
        processCommand(buffer);
    }
}
void ClientSession::processCommand(const std::string &raw_cmd) {
    FTPCommand ftp_cmd = FTPCommandParser::parse(raw_cmd);
    if (ftp_cmd.command == FTPCMD::QUIT) {
        handleQuit(ftp_cmd.args);
        return;
    }
    spdlog::debug("状态{}", (int)m_state);
    switch (m_state) {
    case WAIT_USER:
        switch (ftp_cmd.command) {
        case USER:
            handleUser(ftp_cmd.args);
            m_state = WAIT_PASS;
            break;
        case FEAT:
            Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
            break;
        case AUTH:
            Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
            break;
        case NOOP:
            Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
            break;
        default:
            Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
            break;
        }
        break;
    case WAIT_PASS:
        switch (ftp_cmd.command) {
        case PASS: handlePass(ftp_cmd.args); break;
        default:
            Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
            break;
        }
        break;
    case AUTHENTICATED:
        switch (ftp_cmd.command) {
        case PASV: handlePasv(ftp_cmd.args); break;
        case PORT: handlePort(ftp_cmd.args); break;
        case CWD: handleCwd(std::move(ftp_cmd.args)); return;
        case PWD: handlePwd(ftp_cmd.args); return;
        default:
            Response::sendResponse(m_ctrcl_socket, Response::BADSEQ);
            return;
        }
        m_state = TRANSFER;
        break;
    case TRANSFER:
        switch (m_data_channel.m_conn_mode) {
        case DataChannel::INACTIVE:
            Response::sendResponse(m_ctrcl_socket, Response::BADSEQ);
            break;
        case DataChannel::PASV_READY:
            switch (ftp_cmd.command) {
            case LIST: handleList(ftp_cmd.args); break;
            case RETR: handleRetr(ftp_cmd.args); break;
            case STOR: handleStor(ftp_cmd.args); break;
            default:
                Response::sendResponse(m_ctrcl_socket, Response::BADSEQ);
                break;
            }
            break;
        case DataChannel::PORT_READY:
            switch (ftp_cmd.command) {
            case LIST: handleList(ftp_cmd.args); break;
            case RETR: handleRetr(ftp_cmd.args); break;
            case STOR: handleStor(ftp_cmd.args); break;
            default:
                Response::sendResponse(m_ctrcl_socket, Response::BADSEQ);
                break;
            }
            break;
        }
        m_data_channel.reset();
        m_state = AUTHENTICATED;
        break;
    }
}

ClientSession::~ClientSession() { close(m_ctrcl_socket); }
ClientSession::ClientSession(int ctrl_socket, std::string &&wd)
    : m_ctrcl_socket(ctrl_socket), m_working_dir(std::move(wd)),
      m_data_channel(ctrl_socket) {
    std::filesystem::current_path(m_working_dir);
    spdlog::debug("创建客户端会话: {}", __FUNCTION__);
}
void ClientSession::handleUser(const CommandArgs &args) {
    Response::sendResponse(m_ctrcl_socket, Response::NEEDPASS);
}
void ClientSession::handlePass(const CommandArgs &args) {
    // todo: check password
    m_state = AUTHENTICATED;
    Response::sendResponse(m_ctrcl_socket, Response::LOGGED);
}
void ClientSession::handleQuit(const CommandArgs &args) {
    Response::sendResponse(m_ctrcl_socket, Response::CLOSECTRL);
    m_connected = false;
}
void ClientSession::handleCwd(CommandArgs &&args) {
    std::filesystem::path path(args[0]);
    if (path.is_relative()) {
        path = std::filesystem::path(m_working_dir) / path;
    }
    if (setWorkingDir(path.string())) {
        Response::sendResponse(m_ctrcl_socket, Response::FILEACTOK);
    } else {
        Response::sendResponse(m_ctrcl_socket, Response::FILEUNAVAIL);
    }
}
void ClientSession::handlePwd(const CommandArgs &args) const {
    Response::sendResponse(
        m_ctrcl_socket,
        std::format("257 \"{}\" is current directory.\r\n", m_working_dir));
}
void ClientSession::handleList(const CommandArgs &args) {
    Response::sendResponse(m_ctrcl_socket, Response::PEND);
    std::filesystem::path path(m_working_dir);
    for (const auto &arg : args) { path.append(arg); }
    spdlog::debug(path.c_str());
    if (!std::filesystem::is_directory(path)) {
        spdlog::warn("目录不存在或不是目录: {}", path.c_str());
        Response::sendResponse(m_ctrcl_socket, Response::FILEUNAVAIL);
        return;
    }
    std::filesystem::directory_iterator list(path);
    std::string list_data;
    for (const auto &entry : list) {
        list_data += entry.path().filename().string() + "\r\n";
    }
    m_data_channel.m_data_sock =
        accept(m_data_channel.m_server_sock, nullptr, nullptr);
    if (m_data_channel.m_data_sock < 0) {
        spdlog::error("接受数据连接失败");
        Response::sendResponse(m_ctrcl_socket, Response::FAILDATACONN);
        return;
    }
    spdlog::debug("接受数据连接: {}", m_data_channel.m_data_sock);
    ssize_t bytes_sent = send(m_data_channel.m_data_sock, list_data.c_str(),
                              list_data.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
    Response::sendResponse(m_ctrcl_socket, Response::CLOSEDATACONN);
}
bool ClientSession::setWorkingDir(std::string &&path) {
    if (!std::filesystem::is_directory(path)) {
        spdlog::warn("工作目录不存在或不是目录: {}", path);
        return false;
    }
    m_working_dir = path;
    std::filesystem::current_path(m_working_dir);
    return true;
}
void ClientSession::handlePasv(const CommandArgs &args) {
    m_data_channel.setup();
    if (m_data_channel.m_conn_mode == DataChannel::INACTIVE) {
        Response::sendResponse(m_ctrcl_socket, Response::FAILDATACONN);
        return;
    }
    Response::sendResponse(
        m_ctrcl_socket,
        std::format("227 Entering Passive Mode (127,0,0,1,{},{})\r\n",
                    m_data_channel.port() / 256, m_data_channel.port() % 256));
}
void ClientSession::handleRetr(const CommandArgs &args) {
    // start send
    Response::sendResponse(m_ctrcl_socket, Response::PEND);
    int file_fd = open(args[0].c_str(), O_RDONLY);
    if (file_fd < 0) {
        spdlog::error("打开文件失败: {}", args[0]);
        Response::sendResponse(m_ctrcl_socket, Response::FILEUNAVAIL);
        return;
    }
    m_data_channel.m_data_sock =
        accept(m_data_channel.m_server_sock, nullptr, nullptr);
    if (m_data_channel.m_data_sock < 0) {
        spdlog::error("接受数据连接失败");
        Response::sendResponse(m_ctrcl_socket, Response::FAILDATACONN);
        return;
    }
    spdlog::debug("接受数据连接: {}", m_data_channel.m_data_sock);
    sendfile(m_data_channel.m_data_sock, file_fd, nullptr, BUFSIZ);
    close(file_fd);
    Response::sendResponse(m_ctrcl_socket, Response::CLOSEDATACONN);
}
void ClientSession::handleStor(const CommandArgs &args) const {
    Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
}
void ClientSession::handlePort(const CommandArgs &args) {
    Response::sendResponse(m_ctrcl_socket, Response::NOTIMPL);
}
