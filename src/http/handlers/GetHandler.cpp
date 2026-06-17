#include "../../../include/http/MethodHandler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>

static std::string getContentType(const std::string& filePath)
{
    std::string fileExtension;
    std::size_t dotPosition = filePath.rfind('.');
    if (dotPosition != std::string::npos)
        fileExtension = filePath.substr(dotPosition);

    if (fileExtension == ".html" || fileExtension == ".htm") return "text/html";
    if (fileExtension == ".css")                             return "text/css";
    if (fileExtension == ".js")                              return "application/javascript";
    if (fileExtension == ".json")                            return "application/json";
    if (fileExtension == ".png")                             return "image/png";
    if (fileExtension == ".jpg" || fileExtension == ".jpeg") return "image/jpeg";
    if (fileExtension == ".gif")                             return "image/gif";
    if (fileExtension == ".ico")                             return "image/x-icon";
    if (fileExtension == ".txt")                             return "text/plain";
    return "application/octet-stream";
}

static HttpResponse buildAutoindex(const std::string& directoryPath, const std::string& requestUri)
{
    DIR* directory = opendir(directoryPath.c_str());
    if (!directory)
    {
        HttpResponse response;
        response.status_code = 403;
        response.status_msg  = "Forbidden";
        return response;
    }

    std::string listingHtml = "<html><body><h1>Index of " + requestUri + "</h1><ul>";

    struct dirent* dirEntry;
    while ((dirEntry = readdir(directory)) != NULL)
    {
        std::string entryName = dirEntry->d_name;
        std::string entryLink = requestUri;
        if (entryLink[entryLink.size() - 1] != '/')
            entryLink += '/';
        entryLink  += entryName;
        listingHtml += "<li><a href=\"" + entryLink + "\">" + entryName + "</a></li>";
    }
    closedir(directory);
    listingHtml += "</ul></body></html>";

    HttpResponse       response;
    std::ostringstream contentLength;
    response.status_code = 200;
    response.status_msg  = "OK";
    response.body        = listingHtml;
    response.headers["Content-Type"] = "text/html";
    contentLength << listingHtml.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}

HttpResponse MethodHandler::handleGet(const HttpRequest& request, const LocationConfig& location)
{
    std::string filePath = location.getRoot() + request.uri;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode))
    {
        if (!location.getIndex().empty())
        {
            if (filePath[filePath.size() - 1] != '/')
                filePath += '/';
            filePath += location.getIndex();
        }
        else if (location.getAutoindex())
            return buildAutoindex(filePath, request.uri);
        else
            return buildError(403, "Forbidden");
    }

    int fileDescriptor = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor == -1)
        return buildError(404, "Not Found");

    HttpResponse response;
    char         readBuffer[4096];
    ssize_t      bytesRead;

    while ((bytesRead = read(fileDescriptor, readBuffer, sizeof(readBuffer))) > 0)
        response.body.append(readBuffer, bytesRead);
    close(fileDescriptor);

    response.status_code = 200;
    response.status_msg  = "OK";
    response.headers["Content-Type"] = getContentType(filePath);

    std::ostringstream contentLength;
    contentLength << response.body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
