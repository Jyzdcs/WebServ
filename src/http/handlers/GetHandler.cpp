#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#include <cerrno>

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
    std::string uriPath  = request.uri;
    std::size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos)
        uriPath = uriPath.substr(0, queryPos);

    std::string filePath = location.getRoot() + uriPath;

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode))
    {
        if (!location.getIndex().empty())
        {
            std::string indexPath = filePath;
            if (indexPath[indexPath.size() - 1] != '/')
                indexPath += '/';
            indexPath += location.getIndex();

            struct stat indexInfo;
            if (stat(indexPath.c_str(), &indexInfo) == 0)
                filePath = indexPath;
            else if (location.getAutoindex())
                return buildAutoindex(filePath, uriPath);
            else
                return buildHttpError(404, "Not Found");
        }
        else if (location.getAutoindex())
            return buildAutoindex(filePath, uriPath);
        else
            return buildHttpError(403, "Forbidden");
    }

    int fileDescriptor = open(filePath.c_str(), O_RDONLY);
    if (fileDescriptor == -1)
    {
        if (errno == EACCES || errno == EPERM)
            return buildHttpError(403, "Forbidden");
        return buildHttpError(404, "Not Found");
    }

    HttpResponse response;
    char         readBuffer[4096];
    ssize_t      bytesRead;

    while ((bytesRead = read(fileDescriptor, readBuffer, sizeof(readBuffer))) > 0)
        response.body.append(readBuffer, bytesRead);
    if (bytesRead < 0)
    {
        close(fileDescriptor);
        return buildHttpError(500, "Internal Server Error");
    }
    close(fileDescriptor);

    response.status_code = 200;
    response.status_msg  = "OK";
    response.headers["Content-Type"] = getContentType(filePath);

    std::ostringstream contentLength;
    contentLength << response.body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
