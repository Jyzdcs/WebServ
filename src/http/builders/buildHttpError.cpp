#include "../../../include/http/builders/HttpBuilders.hpp"
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
