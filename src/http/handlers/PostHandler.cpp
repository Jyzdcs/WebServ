#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>


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

    if (!writeFdFromString(fd, body))
    {
        close(fd);
        return buildHttpError(507, "Insufficient Storage");
    }
    close(fd);

    HttpResponse response        = buildHttpCreated("");
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
