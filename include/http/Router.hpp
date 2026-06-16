#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "HttpRequest.hpp"
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"

class Router {
public:
    LocationConfig route(const HttpRequest& req, const ServerConfig& server);
};

#endif
