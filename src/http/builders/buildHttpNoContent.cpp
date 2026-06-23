#include "../../../include/http/builders/HttpBuilders.hpp"

HttpResponse buildHttpNoContent()
{
    HttpResponse response;
    response.status_code               = 204;
    response.status_msg                = "No Content";
    response.headers["Content-Length"] = "0";
    return response;
}
