#ifndef PROCESS_HTTP_HPP
#define PROCESS_HTTP_HPP

#include "../config/ServerConfig.hpp"
#include <string>

struct ProcessResult {
    std::string response;
    bool        shouldClose;
};

ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server);

#endif
