#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

// vérifie si l'URI cible un script CGI en comparant son extension avec celle configurée
// ex: uri="/cgi-bin/form.py", cgiExtension=".py" → true
static bool isCgiRequest(const HttpRequest& request, const LocationConfig& location)
{
    bool noCgiConfigured = location.getCgiExtension().empty() || location.getCgiPath().empty();
    if (noCgiConfigured)
        return false;

    // strip le query string avant de comparer l'extension
    // ex: "/cgi-bin/form.py?name=foo" → "/cgi-bin/form.py"
    std::string uriWithoutQuery = request.uri;
    std::size_t queryStart      = uriWithoutQuery.find('?');
    if (queryStart != std::string::npos)
        uriWithoutQuery = uriWithoutQuery.substr(0, queryStart);

    const std::string& cgiExtension = location.getCgiExtension();
    bool               uriTooShort  = uriWithoutQuery.size() < cgiExtension.size();
    if (uriTooShort)
        return false;

    // compare les derniers N chars de l'URI avec l'extension configurée
    std::string uriExtension = uriWithoutQuery.substr(uriWithoutQuery.size() - cgiExtension.size());
    return uriExtension == cgiExtension;
}

bool MethodHandler::isMethodAllowed(const std::string& method, const LocationConfig& location)
{
    const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
    return std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end();
}

// remplace le body de l'erreur par le fichier HTML configuré dans error_page
// ex: error_page 404 /errors/404.html → lit root + /errors/404.html et l'envoie
// si le fichier n'existe pas ou si le status n'est pas une erreur → retourne la réponse originale
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

    // ex: root="/var/www" + path="/errors/404.html" = "/var/www/errors/404.html"
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

HttpResponse MethodHandler::handle(const HttpRequest& request, const LocationConfig& location, const ServerConfig& server)
{
    HttpResponse response;

    if (hasPathTraversal(request.uri))
        // Erreur: "GET /../../../etc/passwd HTTP/1.1"
        response = buildHttpError(400, "Bad Request");
    else if (location.getPath().empty())
        // aucune location ne matche l'URI → le router a retourné une LocationConfig vide
        response = buildHttpError(404, "Not Found");
    else if (!MethodHandler::isMethodAllowed(request.method, location))
        // ex: DELETE sur une location qui n'autorise que GET/POST
        response = buildHttpError(405, "Method Not Allowed");
    else if (server.getMaxBodySize() > 0 && request.body.size() > server.getMaxBodySize())
        // ex: client_max_body_size 1m et body = 2Mo
        response = buildHttpError(413, "Payload Too Large");
    else if (!location.getRedirectUrl().empty())
        // retour direct : la redirect ne passe pas par applyCustomErrorPage
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

    // remplace le body d'erreur par la page HTML custom si configurée
    return applyCustomErrorPage(response, server, location);
}
