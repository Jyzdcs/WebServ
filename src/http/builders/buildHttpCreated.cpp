#include "../../../include/http/builders/HttpBuilders.hpp"
#include <sstream>

HttpResponse buildHttpCreated(const std::string& body)
{
    HttpResponse       response;
    std::ostringstream contentLength;

    response.status_code = 201;
    response.status_msg  = "Created";
    response.body        = body;
    response.headers["Content-Type"] = "text/plain";
    contentLength << body.size();
    response.headers["Content-Length"] = contentLength.str();
    return response;
}
