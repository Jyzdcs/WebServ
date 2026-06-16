#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

struct HttpRequest {
    std::string                         method;   // "GET", "POST", "DELETE"
    std::string                         uri;      // "/index.html"
    std::string                         version;  // "HTTP/1.1"
    std::map<std::string, std::string>  headers;  // {"Host": "localhost", ...}
    std::string                         body;     // contenu du body (POST)
};

#endif
