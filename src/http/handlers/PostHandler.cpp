#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

HttpResponse MethodHandler::handlePost(const HttpRequest& request, const LocationConfig& location)
{
    if (location.getUploadPath().empty())
        return buildHttpError(500, "Internal Server Error");

    if (request.body.empty())
        return buildHttpError(400, "Bad Request");

    std::string filename = extractFilename(request.uri);
    if (filename.empty())
        return buildHttpError(400, "Bad Request");

    std::string destinationPath = location.getUploadPath() + "/" + filename;

    int fileDescriptor = open(destinationPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor == -1)
        return buildHttpError(403, "Forbidden");

    const char* bodyData          = request.body.data();
    std::size_t totalBytesToWrite = request.body.size();
    std::size_t totalBytesWritten = 0;

    while (totalBytesWritten < totalBytesToWrite)
    {
        ssize_t bytesWritten = write(fileDescriptor, bodyData + totalBytesWritten,
                                     totalBytesToWrite - totalBytesWritten);
        if (bytesWritten <= 0)
        {
            close(fileDescriptor);
            return buildHttpError(507, "Insufficient Storage");
        }
        totalBytesWritten += static_cast<std::size_t>(bytesWritten);
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
