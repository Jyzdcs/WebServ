#include "../../include/http/MethodHandler.hpp"
#include <algorithm>
#include <sstream>

HttpResponse MethodHandler::handle(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server)
{
    (void)server;

    if (!isMethodAllowed(req.method, loc))
        return buildError(405, "Method Not Allowed");

    if (!loc.getRedirectUrl().empty())
    {
        HttpResponse res;
        res.status_code = 301;
        res.status_msg  = "Moved Permanently";
        res.headers["Location"] = loc.getRedirectUrl();
        return res;
    }

    if (req.method == "GET")
        return handleGet(req, loc);
    if (req.method == "POST")
        return handlePost(req, loc);
    if (req.method == "DELETE")
        return handleDelete(req, loc);

    return buildError(405, "Method Not Allowed");
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& loc)
{
    const std::vector<std::string>& methods = loc.getAllowedMethods();
    return std::find(methods.begin(), methods.end(), method) != methods.end();
}

HttpResponse MethodHandler::buildError(int code, const std::string& msg)
{
    HttpResponse    res;
    std::ostringstream ss;

    res.status_code = code;
    res.status_msg  = msg;
    res.body        = "<html><body><h1>" + msg + "</h1></body></html>";
    res.headers["Content-Type"] = "text/html";
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}

HttpResponse MethodHandler::handlePost(const HttpRequest& req, const LocationConfig& loc)
{
    (void)req;
    (void)loc;
    return buildError(501, "Not Implemented");
}

