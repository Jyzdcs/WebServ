#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include "HttpRequest.hpp"
#include <string>
#include <exception>

class RequestParser {
public:
    class ParseException : public std::exception {
    public:
        ParseException(int code, const std::string& msg) : _code(code), _msg(msg) {}
        int         getCode() const { return _code; }
        const char* what()    const throw() { return _msg.c_str(); }
    private:
        int         _code;
        std::string _msg;
    };

    HttpRequest parse(const std::string& raw);

private:
    void isValid(const std::string& raw);
    void parseFirstLine(const std::string& line, HttpRequest& req);
    void parseHeaders(const std::string& raw, HttpRequest& req, std::size_t firstLineEnd, std::size_t headerBodySep);
    void parseBody(const std::string& raw, HttpRequest& req, std::size_t headerBodySep);
};

#endif