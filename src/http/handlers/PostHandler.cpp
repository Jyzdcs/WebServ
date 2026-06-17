#include "../../../include/http/MethodHandler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

static std::string extractFilename(const std::string& uri)
{
    std::size_t lastSlashPosition = uri.rfind('/');
    if (lastSlashPosition == std::string::npos || lastSlashPosition + 1 >= uri.size())
        return "";
    return uri.substr(lastSlashPosition + 1);
}

HttpResponse MethodHandler::handlePost(const HttpRequest& request, const LocationConfig& location)
{
    if (location.getUploadPath().empty())
        return buildError(500, "Internal Server Error");

    if (request.body.empty())
        return buildError(400, "Bad Request");

    std::string filename = extractFilename(request.uri);
    if (filename.empty())
        return buildError(400, "Bad Request");

    std::string destinationPath = location.getUploadPath() + "/" + filename;

    int fileDescriptor = open(destinationPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor == -1)
        return buildError(403, "Forbidden");

    const char* bodyData         = request.body.data();
    std::size_t totalBytesToWrite = request.body.size();
    std::size_t totalBytesWritten = 0;

    while (totalBytesWritten < totalBytesToWrite)
    {
        ssize_t bytesWritten = write(fileDescriptor, bodyData + totalBytesWritten,
                                     totalBytesToWrite - totalBytesWritten);
        if (bytesWritten <= 0)
        {
            close(fileDescriptor);
            return buildError(507, "Insufficient Storage");
        }
        totalBytesWritten += bytesWritten;
    }
    close(fileDescriptor);

    HttpResponse       response;
    std::ostringstream contentLength;
    response.status_code             = 201;
    response.status_msg              = "Created";
    response.headers["Location"]     = "/" + filename;
    contentLength << response.body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
