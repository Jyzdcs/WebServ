#include "../../../include/http/utils/HttpUtils.hpp"
#include <unistd.h>

bool readFdToString(int fd, std::string& body)
{
    char    buf[4096];
    ssize_t bytesRead = 0;

    while ((bytesRead = read(fd, buf, sizeof(buf))) > 0)
        body.append(buf, bytesRead);
    close(fd);
    return bytesRead == 0;
}

std::string getContentType(const std::string& filePath)
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
