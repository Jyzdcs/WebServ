#include "../../../include/http/builders/HttpBuilders.hpp"
#include <sstream>

HttpResponse buildHttpOk(const std::string& body, const std::string& contentType)
{
    HttpResponse       response;
    std::ostringstream contentLength;

    response.status_code = 200;
    response.status_msg  = "OK";
    response.body        = body;
    response.headers["Content-Type"] = contentType;
    contentLength << body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
