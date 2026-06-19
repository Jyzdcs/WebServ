#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

static bool isCgiRequest(const HttpRequest& request, const LocationConfig& location)
{
    bool noCgiConfigured = location.getCgiExtension().empty() || location.getCgiPath().empty();
    if (noCgiConfigured)
        return false;

    std::string uriWithoutQuery = request.uri;
    std::size_t queryStart      = uriWithoutQuery.find('?');
    if (queryStart != std::string::npos)
        uriWithoutQuery = uriWithoutQuery.substr(0, queryStart);

    const std::string& cgiExtension = location.getCgiExtension();
    bool               uriTooShort  = uriWithoutQuery.size() < cgiExtension.size();
    if (uriTooShort)
        return false;

    std::string uriExtension = uriWithoutQuery.substr(uriWithoutQuery.size() - cgiExtension.size());
    return uriExtension == cgiExtension;
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& location)
{
    const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
    return std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end();
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
    char    buf[4096];
    ssize_t bytesRead = 0;

    while ((bytesRead = read(fd, buf, sizeof(buf))) > 0)
        customResponse.body.append(buf, bytesRead);
    if (bytesRead < 0)
    {
        close(fd);
        return response;
    }
    close(fd);

    customResponse.headers["Content-Type"] = getContentType(filePath);
    std::ostringstream contentLength;
    contentLength << customResponse.body.size();
    customResponse.headers["Content-Length"] = contentLength.str();
    return customResponse;
}

HttpResponse MethodHandler::handle(const HttpRequest& request, const LocationConfig& location, const ServerConfig& server)
{
    HttpResponse response;

    if (hasPathTraversal(request.uri))
        response = buildHttpError(400, "Bad Request");
    else if (location.getPath().empty())
        response = buildHttpError(404, "Not Found");
    else if (!MethodHandler::isMethodAllowed(request.method, location))
        response = buildHttpError(405, "Method Not Allowed");
    else if (server.getMaxBodySize() > 0 && request.body.size() > server.getMaxBodySize())
        response = buildHttpError(413, "Payload Too Large");
    else if (!location.getRedirectUrl().empty())
        return buildRedirect(location.getRedirectUrl());
    else if (isCgiRequest(request, location))
    {
        CgiHandler cgiHandler;
        response = cgiHandler.execute(request, location);
    }
    else if (request.method == "GET")
        response = handleGet(request, location);
    else if (request.method == "POST")
        response = handlePost(request, location);
    else if (request.method == "DELETE")
        response = handleDelete(request, location);
    else
        response = buildHttpError(405, "Method Not Allowed");

    return applyCustomErrorPage(response, server, location);
}
