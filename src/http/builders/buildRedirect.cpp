#include "../../../include/http/builders/HttpBuilders.hpp"

HttpResponse buildRedirect(const std::string& url)
{
    HttpResponse response;
    response.status_code               = 301;
    response.status_msg                = "Moved Permanently";
    response.headers["Location"]       = url;
    response.headers["Content-Length"] = "0";
    return response;
}
