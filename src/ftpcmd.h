#pragma once
#include <netinet/in.h>
#include <regex>
#include <string>
#include <vector>
enum FTPCMD {
    USER,
    PASS,
    QUIT,
    CWD,
    PWD,
    LIST,
    RETR,
    STOR,
    PASV,
    PORT,
    FEAT,
    AUTH,
    NOOP,
    ABOR,
    Unknown,
};

struct FTPCommand {
    FTPCMD command;
    std::vector<std::string> args;
};

class FTPCommandParser {
 public:
    static FTPCommand parse(const std::string &raw_cmd);

 private:
    static std::vector<std::regex> m_command_regexes;
};
