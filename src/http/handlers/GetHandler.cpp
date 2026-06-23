#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#include <cerrno>

// génère une page HTML listant le contenu d'un répertoire
static HttpResponse buildAutoindex(const std::string& directoryPath, const std::string& requestUri)
{
    DIR* directory = opendir(directoryPath.c_str());
    if (!directory)
        return buildHttpError(403, "Forbidden");

    std::string listingHtml = "<html><body><h1>Index of " + requestUri + "</h1><ul>";

    struct dirent* dirEntry;
    while ((dirEntry = readdir(directory)) != NULL)
    {
        std::string entryName = dirEntry->d_name;
        if (entryName == "." || entryName == "..")
            continue;
        std::string entryLink = requestUri;
        if (entryLink[entryLink.size() - 1] != '/')
            entryLink += '/';
        entryLink   += entryName;
        listingHtml += "<li><a href=\"" + entryLink + "\">" + entryName + "</a></li>";
    }
    closedir(directory);
    listingHtml += "</ul></body></html>";

    return buildHttpOk(listingHtml, "text/html");
}


// retourne le path complet du fichier index s'il existe, sinon ""
// ex: dirPath="/var/www/", index="index.html" → "/var/www/index.html" si le fichier existe
static std::string resolveIndexPath(const std::string& dirPath, const LocationConfig& location)
{
    if (location.getIndex().empty())
        return "";

    std::string indexPath = dirPath;
    if (indexPath[indexPath.size() - 1] != '/')
        indexPath += '/';
    indexPath += location.getIndex();

    struct stat indexInfo;
    if (stat(indexPath.c_str(), &indexInfo) == 0)
        return indexPath;
    return "";
}

// gère le cas répertoire quand aucun index n'a été trouvé
// priorité : autoindex → 404 (index configuré mais absent) → 403 (pas d'index du tout)
static HttpResponse handleDirectory(const std::string& dirPath, const std::string& uriPath,
                                     const LocationConfig& location)
{
    if (location.getAutoindex())
        return buildAutoindex(dirPath, uriPath);

    if (!location.getIndex().empty())
        return buildHttpError(404, "Not Found");

    return buildHttpError(403, "Forbidden");
}

// ouvre et lit un fichier, retourne une réponse 200 avec le body et les headers corrects
static HttpResponse buildFileResponse(const std::string& filePath)
{
    int fileDescriptor = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor == -1)
    {
        if (errno == EACCES || errno == EPERM)
            return buildHttpError(403, "Forbidden");
        return buildHttpError(404, "Not Found");
    }

    std::string body;
    if (!readFdToString(fileDescriptor, body))
        return buildHttpError(500, "Internal Server Error");

    return buildHttpOk(body, getContentType(filePath));
}

HttpResponse MethodHandler::handleGet(const HttpRequest& request, const LocationConfig& location)
{
    std::string uriPath  = extractUriPath(request.uri);
    std::string filePath = location.getRoot() + uriPath;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode))
    {
        std::string indexPath = resolveIndexPath(filePath, location);
        if (!indexPath.empty())
            return buildFileResponse(indexPath);
        return handleDirectory(filePath, uriPath, location);
    }

    return buildFileResponse(filePath);
}
