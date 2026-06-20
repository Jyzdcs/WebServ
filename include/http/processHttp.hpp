#ifndef PROCESS_HTTP_HPP
#define PROCESS_HTTP_HPP

#include "../config/ServerConfig.hpp"
#include <string>

std::string processHttp(const std::string& rawRequest, const ServerConfig& server);

#endif
