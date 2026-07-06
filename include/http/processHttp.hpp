#ifndef PROCESS_HTTP_HPP
#define PROCESS_HTTP_HPP

#include "../config/ServerConfig.hpp"
#include "../http/HttpResponse.hpp"
#include <string>
#include <ctime>
#include <sys/types.h>

struct ProcessResult {
    enum State { COMPLETE, CGI_PENDING };

    State        state;
    HttpResponse httpResponse;
    std::string  rawResponse;
    bool         shouldClose;
    int          stdoutFd;
    pid_t        pid;
    time_t       deadline;

    ProcessResult() : state(COMPLETE), shouldClose(false), stdoutFd(-1), pid(-1), deadline(0) {}
};

ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server);

std::string finishCgi(const std::string& output, pid_t pid, bool timedOut,
                      bool shouldClose, const ServerConfig& server);

#endif
