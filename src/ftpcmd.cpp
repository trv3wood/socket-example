#include "ftpcmd.h"
#include <cstring>
#include <netinet/in.h>
#include <regex>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

std::vector<std::regex> FTPCommandParser::m_command_regexes{
    std::regex(R"(^USER\s+(\S+))"),
    std::regex(R"(^PASS\s+(\S+))"),
    std::regex(R"(^QUIT\s*)"),
    std::regex(R"(^CWD\s+(\S+))"),
    std::regex(R"(^PWD\s*)"),
    std::regex(R"(^LIST\s*(\S*))"),
    std::regex(R"(^RETR\s+(\S+))"),
    std::regex(R"(^STOR\s+(\S+))"),
    std::regex(R"(^PASV)"),
    std::regex(R"(^PORT\s+(\d+),(\d+),(\d+),(\d+),(\d+),(\d+))"),
    std::regex(R"(^FEAT)"),
    std::regex(R"(^AUTH\s+(\S+))"),
    std::regex(R"(^NOOP\s*)"),
    std::regex(R"(^ABOR)"),
};
FTPCommand FTPCommandParser::parse(const std::string &raw_cmd) {
    FTPCommand ftp_cmd;
    for (size_t i = 0; i < m_command_regexes.size(); ++i) {
        std::smatch match;
        if (std::regex_search(raw_cmd, match, m_command_regexes[i])) {
            spdlog::debug("匹配到命令: {}", match[0].str());
            ftp_cmd.command = static_cast<FTPCMD>(i);
            for (size_t j = 1; j < match.size(); ++j) {
                ftp_cmd.args.push_back(match[j].str());
            }
            return ftp_cmd;
        }
    }
    ftp_cmd.command = FTPCMD::Unknown;
    return ftp_cmd;
}
