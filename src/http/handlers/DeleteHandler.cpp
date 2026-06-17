#include "../../../include/http/MethodHandler.hpp"
#include <unistd.h>
#include <sys/stat.h>

HttpResponse MethodHandler::handleDelete(const HttpRequest& request, const LocationConfig& location)
{
    std::string filePath = location.getRoot() + request.uri;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == -1)
        return buildError(404, "Not Found");

    if (S_ISDIR(fileInfo.st_mode))
        return buildError(403, "Forbidden");

    if (unlink(filePath.c_str()) == 0)
    {
        HttpResponse response;
        response.status_code = 204;
        response.status_msg  = "No Content";
        return response;
    }
    return buildError(403, "Forbidden");
}
