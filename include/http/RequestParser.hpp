#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequest.hpp"
#include <string>

class RequestParser {
public:
    HttpRequest parse(const std::string& raw);

private:
    bool        isValid(const std::string& raw);
    void        parseFirstLine(const std::string& line, HttpRequest& req);
    void        parseHeaders(const std::string& raw, HttpRequest& req, std::size_t firstLineEnd, std::size_t headerBodySep);
    void        parseBody(const std::string& raw, HttpRequest& req, std::size_t headerBodySep);
};

#endif