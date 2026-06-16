#ifndef METHOD_HANDLER_HPP
#define METHOD_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"

class MethodHandler {
public:
    HttpResponse handle(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server);

private:
    HttpResponse handleGet(const HttpRequest& req, const LocationConfig& loc);
    HttpResponse handlePost(const HttpRequest& req, const LocationConfig& loc);
    HttpResponse handleDelete(const HttpRequest& req, const LocationConfig& loc);

    bool         isMethodAllowed(const std::string& method, const LocationConfig& loc);
    HttpResponse buildError(int code, const std::string& msg);
};

#endif
