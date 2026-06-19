#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>

HttpResponse MethodHandler::handleDelete(const HttpRequest& request, const LocationConfig& location)
{
    std::string uriPath  = request.uri;
    std::size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos)
        uriPath = uriPath.substr(0, queryPos);

    std::string filePath = location.getRoot() + uriPath;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == -1)
    {
        if (errno == EACCES || errno == EPERM)
            return buildHttpError(403, "Forbidden");
        return buildHttpError(404, "Not Found");
    }

    if (S_ISDIR(fileInfo.st_mode))
        return buildHttpError(403, "Forbidden");

    if (unlink(filePath.c_str()) == 0)
        return buildHttpNoContent();
    return buildHttpError(403, "Forbidden");
}
