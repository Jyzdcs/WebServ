#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <algorithm>

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

HttpResponse MethodHandler::handle(const HttpRequest& request, const LocationConfig& location, const ServerConfig& server)
{
    if (hasPathTraversal(request.uri))
        return buildHttpError(400, "Bad Request");

    if (location.getPath().empty())
        return buildHttpError(404, "Not Found");

    if (!isMethodAllowed(request.method, location))
        return buildHttpError(405, "Method Not Allowed");

    bool bodyLimitIsSet   = server.getMaxBodySize() > 0;
    bool bodyExceedsLimit = request.body.size() > server.getMaxBodySize();
    if (bodyLimitIsSet && bodyExceedsLimit)
        return buildHttpError(413, "Payload Too Large");

    if (!location.getRedirectUrl().empty())
    {
        HttpResponse redirectResponse;
        redirectResponse.status_code            = 301;
        redirectResponse.status_msg             = "Moved Permanently";
        redirectResponse.headers["Location"]    = location.getRedirectUrl();
        return redirectResponse;
    }

    if (isCgiRequest(request, location))
    {
        CgiHandler cgiHandler;
        return cgiHandler.execute(request, location);
    }

    if (request.method == "GET")
        return handleGet(request, location);
    if (request.method == "POST")
        return handlePost(request, location);
    if (request.method == "DELETE")
        return handleDelete(request, location);

    return buildHttpError(405, "Method Not Allowed");
}
