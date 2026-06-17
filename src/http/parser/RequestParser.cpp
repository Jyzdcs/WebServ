#include "../../../include/http/RequestParser.hpp"
#include <sstream>

HttpRequest RequestParser::parse(const std::string& raw)
{
    HttpRequest req;

    if (!isValid(raw))
        return req;

    std::size_t firstLineEnd  = raw.find("\r\n");
    std::size_t headerBodySep = raw.find("\r\n\r\n");

    parseFirstLine(raw.substr(0, firstLineEnd), req);
    parseHeaders(raw, req, firstLineEnd, headerBodySep);
    parseBody(raw, req, headerBodySep);
    return req;
}

bool RequestParser::isValid(const std::string& raw)
{
    if (raw.find("\r\n\r\n") == std::string::npos)
        return false;

    std::size_t firstLineEnd  = raw.find("\r\n");
    std::size_t headerBodySep = raw.find("\r\n\r\n");
    std::string firstLine     = raw.substr(0, firstLineEnd);

    std::istringstream ss(firstLine);
    std::string        method, uri, version;
    if (!(ss >> method >> uri >> version))
        return false;

    std::string extra;
    if (ss >> extra)
        return false;

    if (version.substr(0, 5) != "HTTP/")
        return false;

    bool hasHost = false;
    std::size_t start = firstLineEnd + 2;
    while (start < headerBodySep)
    {
        std::size_t lineEnd = raw.find("\r\n", start);
        std::string line    = raw.substr(start, lineEnd - start);

        std::size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            if (colon + 1 < line.size() && line[colon + 1] == '\t')
                return false;
            if (line.substr(0, colon) == "Host")
                hasHost = true;
        }
        start = lineEnd + 2;
    }

    if (version == "HTTP/1.1" && !hasHost)
        return false;

    return true;
}

void RequestParser::parseFirstLine(const std::string& line, HttpRequest& req)
{
    std::istringstream ss(line);
    ss >> req.method;
    ss >> req.uri;
    ss >> req.version;
}

void RequestParser::parseHeaders(const std::string& raw, HttpRequest& req, std::size_t firstLineEnd, std::size_t headerBodySep)
{
    std::size_t start = firstLineEnd + 2;
    std::size_t end   = headerBodySep;

    while (start < end)
    {
        std::size_t lineEnd = raw.find("\r\n", start);
        std::string line    = raw.substr(start, lineEnd - start);

        std::size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key      = line.substr(0, colon);
            std::string value;
            std::size_t valStart = colon + 1;
            while (valStart < line.size() && line[valStart] == ' ')
                valStart++;
            if (valStart < line.size())
                value = line.substr(valStart);
            req.headers[key] = value;
        }
        start = lineEnd + 2;
    }
}

void RequestParser::parseBody(const std::string& raw, HttpRequest& req, std::size_t headerBodySep)
{
    req.body = raw.substr(headerBodySep + 4);
}
