#include "../../include/http/MethodHandler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>

static std::string getContentType(const std::string& path)
{
    std::string ext;
    std::size_t dot = path.rfind('.');
    if (dot != std::string::npos)
        ext = path.substr(dot);

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")                   return "text/css";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".json")                  return "application/json";
    if (ext == ".png")                   return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")                   return "image/gif";
    if (ext == ".ico")                   return "image/x-icon";
    if (ext == ".txt")                   return "text/plain";
    return "application/octet-stream";
}

static HttpResponse buildAutoindex(const std::string& path, const std::string& uri)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        HttpResponse res;
        res.status_code = 403;
        res.status_msg  = "Forbidden";
        return res;
    }

    std::string html = "<html><body><h1>Index of " + uri + "</h1><ul>";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        std::string href = uri;
        if (href[href.size() - 1] != '/')
            href += '/';
        href += name;
        html += "<li><a href=\"" + href + "\">" + name + "</a></li>";
    }
    closedir(dir);
    html += "</ul></body></html>";

    HttpResponse    res;
    std::ostringstream ss;
    res.status_code = 200;
    res.status_msg  = "OK";
    res.body        = html;
    res.headers["Content-Type"] = "text/html";
    ss << html.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}

HttpResponse MethodHandler::handleGet(const HttpRequest& req, const LocationConfig& loc)
{
    std::string path = loc.getRoot() + req.uri;

    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        if (!loc.getIndex().empty())
        {
            if (path[path.size() - 1] != '/')
                path += '/';
            path += loc.getIndex();
        }
        else if (loc.getAutoindex())
            return buildAutoindex(path, req.uri);
        else
            return buildError(403, "Forbidden");
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1)
        return buildError(404, "Not Found");

    HttpResponse    res;
    char            buf[4096];
    ssize_t         n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        res.body.append(buf, n);
    close(fd);

    res.status_code = 200;
    res.status_msg  = "OK";
    res.headers["Content-Type"] = getContentType(path);

    std::ostringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}
