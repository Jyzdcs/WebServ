#include "../../../include/http/MethodHandler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

static std::string extractFilename(const std::string& uri)
{
    std::size_t slash = uri.rfind('/');
    if (slash == std::string::npos || slash + 1 >= uri.size())
        return "";
    return uri.substr(slash + 1);
}

static bool hasPathTraversal(const std::string& uri)
{
    return uri.find("..") != std::string::npos;
}

HttpResponse MethodHandler::handlePost(const HttpRequest& req, const LocationConfig& loc)
{
    if (loc.getUploadPath().empty())
        return buildError(500, "Internal Server Error");

    if (req.body.empty())
        return buildError(400, "Bad Request");

    if (hasPathTraversal(req.uri))
        return buildError(400, "Bad Request");

    std::string filename = extractFilename(req.uri);
    if (filename.empty())
        return buildError(400, "Bad Request");

    std::string path = loc.getUploadPath() + "/" + filename;

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return buildError(403, "Forbidden");

    const char* data    = req.body.data();
    std::size_t total   = req.body.size();
    std::size_t written = 0;

    while (written < total)
    {
        ssize_t n = write(fd, data + written, total - written);
        if (n <= 0)
        {
            close(fd);
            return buildError(507, "Insufficient Storage");
        }
        written += n;
    }
    close(fd);

    HttpResponse       res;
    res.status_code = 201;
    res.status_msg  = "Created";
    res.headers["Location"] = "/" + filename;
    std::ostringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}
