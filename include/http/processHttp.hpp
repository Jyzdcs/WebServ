#ifndef PROCESS_HTTP_HPP
#define PROCESS_HTTP_HPP

#include "../config/ServerConfig.hpp"
#include "../http/HttpResponse.hpp"
#include <string>
#include <ctime>
#include <sys/types.h>

// Retour de processHttp() et de MethodHandler::handle().
// COMPLETE  : rawResponse est prête à envoyer, shouldClose indique si la connexion doit fermer.
// CGI_PENDING : le script tourne, stdoutFd/pid/deadline sont à gérer dans poll().
struct ProcessResult {
    enum State { COMPLETE, CGI_PENDING };

    State        state;
    HttpResponse httpResponse; // utilisé par handle() — sérialisé dans rawResponse par processHttp()
    std::string  rawResponse;  // prêt à envoyer (set par processHttp, pas par handle())
    bool         shouldClose;
    int          stdoutFd;
    pid_t        pid;
    time_t       deadline;

    ProcessResult() : state(COMPLETE), shouldClose(false), stdoutFd(-1), pid(-1), deadline(0) {}
};

ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server);

// Appelée par le Core Server quand stdoutFd est EOF ou deadline dépassée.
// timedOut=true : le Core Server a déjà kill(pid, SIGKILL).
// Retourne une string HTTP prête à envoyer.
std::string finishCgi(const std::string& output, pid_t pid, bool timedOut,
                      bool shouldClose, const ServerConfig& server);

#endif
