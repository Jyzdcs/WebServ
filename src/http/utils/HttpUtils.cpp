#include "../../../include/http/utils/HttpUtils.hpp"
#include <sstream>

HttpResponse buildHttpError(int statusCode, const std::string& statusMessage)
{
    HttpResponse       response;
    std::ostringstream contentLength;

    response.status_code = statusCode;
    response.status_msg  = statusMessage;
    response.body        = "<html><body><h1>" + statusMessage + "</h1></body></html>";
    response.headers["Content-Type"] = "text/html";
    contentLength << response.body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}

HttpResponse buildRedirect(const std::string& url)
{
    HttpResponse response;
    response.status_code              = 301;
    response.status_msg               = "Moved Permanently";
    response.headers["Location"]      = url;
    response.headers["Content-Length"] = "0";
    return response;
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
