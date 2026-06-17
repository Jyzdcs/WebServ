#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include <algorithm>
#include <sstream>

static std::string urlDecode(const std::string& encoded)
{
    std::string decoded;

    for (std::size_t pos = 0; pos < encoded.size(); ++pos)
    {
        bool isPercentSign      = encoded[pos] == '%';
        bool hasTwoCharAfter    = pos + 2 < encoded.size();

        if (!isPercentSign || !hasTwoCharAfter)
        {
            decoded += encoded[pos];
            continue;
        }

        char hexSequence[3] = { encoded[pos + 1], encoded[pos + 2], '\0' };
        char* parseEnd;
        int   decodedChar = std::strtol(hexSequence, &parseEnd, 16);

        bool validHex = (parseEnd == hexSequence + 2);
        if (!validHex)
        {
            decoded += encoded[pos];
            continue;
        }

        decoded += static_cast<char>(decodedChar);
        pos += 2;
    }

    return decoded;
}

static bool hasPathTraversal(const std::string& uri)
{
    std::string decodedUri      = urlDecode(uri);
    bool        containsDotDot  = decodedUri.find("..") != std::string::npos;

    return containsDotDot;
}

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

HttpResponse MethodHandler::handle(const HttpRequest& request, const LocationConfig& location, const ServerConfig& server)
{
    if (hasPathTraversal(request.uri))
        return buildError(400, "Bad Request");

    if (location.getPath().empty())
        return buildError(404, "Not Found");

    if (!isMethodAllowed(request.method, location))
        return buildError(405, "Method Not Allowed");

    bool   bodyLimitIsSet    = server.getMaxBodySize() > 0;
    bool   bodyExceedsLimit  = request.body.size() > server.getMaxBodySize();
    if (bodyLimitIsSet && bodyExceedsLimit)
        return buildError(413, "Payload Too Large");

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

    return buildError(405, "Method Not Allowed");
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& location)
{
    const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
    return std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end();
}

HttpResponse MethodHandler::buildError(int statusCode, const std::string& statusMessage)
{
    HttpResponse       response;
    std::ostringstream contentLength;

    response.status_code = statusCode;
    response.status_msg  = statusMessage;
    response.body        = "<html><body><h1>" + statusMessage + "</h1></body></html>";
    response.headers["Content-Type"] = "text/html";
    contentLength << response.body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
