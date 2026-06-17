#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include <algorithm>
#include <sstream>

static bool isCgiRequest(const HttpRequest& req, const LocationConfig& loc)
{
    if (loc.getCgiExtension().empty() || loc.getCgiPath().empty())
        return false;
    std::string uri = req.uri;
    std::size_t q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);
    const std::string& ext = loc.getCgiExtension();
    if (uri.size() < ext.size())
        return false;
    return uri.substr(uri.size() - ext.size()) == ext;
}

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

    if (isCgiRequest(req, loc))
    {
        CgiHandler cgi;
        return cgi.execute(req, loc);
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
    HttpResponse       res;
    std::ostringstream ss;

    res.status_code = code;
    res.status_msg  = msg;
    res.body        = "<html><body><h1>" + msg + "</h1></body></html>";
    res.headers["Content-Type"] = "text/html";
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}
