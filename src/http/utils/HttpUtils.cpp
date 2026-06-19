#include "../../../include/http/utils/HttpUtils.hpp"
#include <unistd.h>

// écrit le contenu de data dans fd en boucle jusqu'à épuisement
// retourne true si tout a été écrit, false si write() a échoué
bool writeFdFromString(int fd, const std::string& data)
{
    const char* ptr          = data.data();
    std::size_t totalToWrite = data.size();
    std::size_t totalWritten = 0;

    while (totalWritten < totalToWrite)
    {
        ssize_t written = write(fd, ptr + totalWritten, totalToWrite - totalWritten);
        if (written <= 0)
            return false;
        totalWritten += static_cast<std::size_t>(written);
    }
    return true;
}

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
