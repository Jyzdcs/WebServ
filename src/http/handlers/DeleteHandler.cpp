#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>

// strip le query string de l'URI
// ex: "/files/photo.jpg?v=2" → "/files/photo.jpg"
static std::string extractUriPath(const std::string& uri)
{
    std::size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos)
        return uri.substr(0, queryPos);
    return uri;
}

// vérifie l'existence du fichier et le supprime
// retourne 204 No Content ou une erreur
static HttpResponse deleteFile(const std::string& filePath)
{
    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == -1)
    {
        if (errno == EACCES || errno == EPERM)
            return buildHttpError(403, "Forbidden");
        return buildHttpError(404, "Not Found");
    }

    // Erreur: on ne peut pas supprimer un répertoire
    if (S_ISDIR(fileInfo.st_mode))
        return buildHttpError(403, "Forbidden");

    if (unlink(filePath.c_str()) == 0)
        return buildHttpNoContent();
    return buildHttpError(403, "Forbidden");
}

HttpResponse MethodHandler::handleDelete(const HttpRequest& request, const LocationConfig& location)
{
    std::string uriPath  = extractUriPath(request.uri);
    std::string filePath = location.getRoot() + uriPath;
    return deleteFile(filePath);
}
