#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>

struct HttpResponse {
    int                                 status_code;  // 200, 404, 500...
    std::string                         status_msg;   // "OK", "Not Found", ...
    std::map<std::string, std::string>  headers;      // {"Content-Type": "text/html", ...}
    std::string                         body;

    HttpResponse() : status_code(0) {}
};

#endif
