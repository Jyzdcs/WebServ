#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

// strip le query string de l'URI
// ex: "/upload/file.txt?foo=bar" → "/upload/file.txt"
static std::string extractUriPath(const std::string& uri)
{
    std::size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos)
        return uri.substr(0, queryPos);
    return uri;
}

// ouvre le fichier de destination et écrit le body dedans
// retourne 201 Created avec Location, ou une erreur
static HttpResponse writeBodyToFile(const std::string& destPath, const std::string& body,
                                     const std::string& filename)
{
    int fd = open(destPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        if (errno == EACCES || errno == EPERM)
            return buildHttpError(403, "Forbidden");
        return buildHttpError(500, "Internal Server Error");
    }

    const char* data         = body.data();
    std::size_t totalToWrite = body.size();
    std::size_t totalWritten = 0;

    while (totalWritten < totalToWrite)
    {
        ssize_t written = write(fd, data + totalWritten, totalToWrite - totalWritten);
        if (written <= 0)
        {
            close(fd);
            return buildHttpError(507, "Insufficient Storage");
        }
        totalWritten += static_cast<std::size_t>(written);
    }
    close(fd);

    HttpResponse response    = buildHttpCreated("");
    response.headers["Location"] = "/" + filename;
    return response;
}

HttpResponse MethodHandler::handlePost(const HttpRequest& request, const LocationConfig& location)
{
    // Erreur: location sans upload_store configuré
    if (location.getUploadPath().empty())
        return buildHttpError(500, "Internal Server Error");

    std::string uriPath  = extractUriPath(request.uri);
    std::string filename = extractFilename(uriPath);
    if (filename.empty())
        return buildHttpError(400, "Bad Request");

    std::string destPath = location.getUploadPath() + "/" + filename;
    return writeBodyToFile(destPath, request.body, filename);
}
