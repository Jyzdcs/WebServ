#ifndef METHOD_HANDLER_HPP
#define METHOD_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/processHttp.hpp"

class MethodHandler {
public:
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
