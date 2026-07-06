#ifndef METHOD_HANDLER_HPP
#define METHOD_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/processHttp.hpp"

class MethodHandler {
public:
    // Dispatche la requête vers GET/POST/DELETE ou CGI (async).
    // Retourne COMPLETE avec httpResponse pour les cas statiques,
    // CGI_PENDING avec stdoutFd+pid+deadline pour les CGI.
    // shouldClose est défini par l'appelant (processHttp) après le retour.
    ProcessResult handle(const HttpRequest& req, const LocationConfig& loc,
                         const ServerConfig& server);

    static bool isCgiRequest(const HttpRequest& req, const LocationConfig& loc);
    static bool isMethodAllowed(const std::string& method, const LocationConfig& loc);

private:
    HttpResponse handleGet(const HttpRequest& req, const LocationConfig& loc);
    HttpResponse handlePost(const HttpRequest& req, const LocationConfig& loc);
    HttpResponse handleDelete(const HttpRequest& req, const LocationConfig& loc);
};

#endif
