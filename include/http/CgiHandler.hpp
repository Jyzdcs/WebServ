#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../../include/config/LocationConfig.hpp"
#include "../../include/http/processHttp.hpp"
#include <string>
#include <vector>

class CgiHandler {
public:
    // Interface async : lance le script sans bloquer.
    // Retourne CGI_PENDING (stdoutFd + pid + deadline) si le script est lancé.
    // Retourne COMPLETE (httpResponse avec l'erreur) si validation échoue (404/403/500).
    ProcessResult start(const HttpRequest& req, const LocationConfig& loc);

    // Parsе la sortie brute du script CGI (headers + body) en HttpResponse.
    // Public pour être accessible depuis finishCgi().
    HttpResponse parseOutput(const std::string& raw);

private:
    std::vector<std::string> buildEnv(const HttpRequest& req, const std::string& scriptPath);
};

#endif
