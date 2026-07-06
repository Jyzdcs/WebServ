#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/processHttp.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

bool MethodHandler::isCgiRequest(const HttpRequest& request, const LocationConfig& location)
{
    if (location.getCgiExtension().empty() || location.getCgiPath().empty())
        return false;

    std::string uri = request.uri;
    std::size_t q   = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);

    const std::string& ext = location.getCgiExtension();
    if (uri.size() < ext.size())
        return false;

    return uri.substr(uri.size() - ext.size()) == ext;
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& location)
{
    const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
    if (std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end())
        return true;
    // HEAD est implicitement autorisé si GET l'est (RFC 7231)
    if (method == "HEAD")
        return std::find(allowedMethods.begin(), allowedMethods.end(), "GET") != allowedMethods.end();
    return false;
}

static HttpResponse applyCustomErrorPage(const HttpResponse& response,
                                          const ServerConfig& server,
                                          const LocationConfig& location)
{
    if (response.status_code < 400)
        return response;

    const std::map<int, std::string>& errorPages = server.getErrorPages();
    std::map<int, std::string>::const_iterator pageIt = errorPages.find(response.status_code);
    if (pageIt == errorPages.end())
        return response;

    std::string filePath = location.getRoot() + pageIt->second;
    int         fd       = open(filePath.c_str(), O_RDONLY);
    if (fd == -1)
        return response;

    HttpResponse customResponse = response;
    customResponse.body = "";
    if (!readFdToString(fd, customResponse.body))
        return response;

    customResponse.headers["Content-Type"] = getContentType(filePath);
    std::ostringstream contentLength;
    contentLength << customResponse.body.size();
    customResponse.headers["Content-Length"] = contentLength.str();
    return customResponse;
}

ProcessResult MethodHandler::handle(const HttpRequest& request, const LocationConfig& location,
                                    const ServerConfig& server)
{
    ProcessResult result;
    HttpResponse  response;

    if (hasPathTraversal(request.uri))
        response = buildHttpError(400, "Bad Request");
    else if (location.getPath().empty())
        response = buildHttpError(404, "Not Found");
    else if (!location.getRedirectUrl().empty())
    {
        result.state        = ProcessResult::COMPLETE;
        result.httpResponse = buildRedirect(location.getRedirectUrl());
        return result;
    }
    else if (!MethodHandler::isMethodAllowed(request.method, location))
        response = buildHttpError(405, "Method Not Allowed");
    else if (server.getMaxBodySize() > 0 && request.body.size() > server.getMaxBodySize())
        response = buildHttpError(413, "Payload Too Large");
    else if (isCgiRequest(request, location))
    {
        CgiHandler cgi;
        return cgi.start(request, location);
    }
    else if (request.method == "GET" || request.method == "HEAD")
    {
        response = handleGet(request, location);
        if (request.method == "HEAD")
            response.body = "";
    }
    else if (request.method == "POST")
        response = handlePost(request, location);
    else if (request.method == "DELETE")
        response = handleDelete(request, location);
    else
        response = buildHttpError(405, "Method Not Allowed");

    result.state        = ProcessResult::COMPLETE;
    result.httpResponse = applyCustomErrorPage(response, server, location);
    return result;
}
