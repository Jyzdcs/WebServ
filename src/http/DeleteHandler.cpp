#include "../../include/http/MethodHandler.hpp"
#include <unistd.h>
#include <sys/stat.h>

HttpResponse MethodHandler::handleDelete(const HttpRequest& req, const LocationConfig& loc)
{
    std::string path = loc.getRoot() + req.uri;

    if (req.uri.find("..") != std::string::npos)
        return buildError(400, "Bad Request");

    struct stat st;
    if (stat(path.c_str(), &st) == -1)
        return buildError(404, "Not Found");

    if (S_ISDIR(st.st_mode))
        return buildError(403, "Forbidden");

    if (unlink(path.c_str()) == 0)
    {
        HttpResponse res;
        res.status_code = 204;
        res.status_msg  = "No Content";
        return res;
    }
    return buildError(403, "Forbidden");
}
