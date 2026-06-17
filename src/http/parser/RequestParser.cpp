#include "../../../include/http/RequestParser.hpp"
#include <sstream>

bool RequestParser::isValid(const std::string& rawRequest)
{
    if (rawRequest.find("\r\n\r\n") == std::string::npos)
        return false;

    std::size_t firstLineEnd        = rawRequest.find("\r\n");
    std::size_t headerBodySeparator = rawRequest.find("\r\n\r\n");
    std::string firstLine           = rawRequest.substr(0, firstLineEnd);

    std::istringstream firstLineStream(firstLine);
    std::string        method, uri, version;
    if (!(firstLineStream >> method >> uri >> version))
        return false;

    std::string extraToken;
    if (firstLineStream >> extraToken)
        return false;

    if (version.substr(0, 5) != "HTTP/")
        return false;

    bool        hasHostHeader   = false;
    std::size_t currentPosition = firstLineEnd + 2;

    while (currentPosition < headerBodySeparator)
    {
        std::size_t lineEndPosition = rawRequest.find("\r\n", currentPosition);
        std::string currentLine     = rawRequest.substr(currentPosition, lineEndPosition - currentPosition);

        std::size_t colonPosition = currentLine.find(':');
        if (colonPosition != std::string::npos)
        {
            bool hasTabAfterColon = (colonPosition + 1 < currentLine.size()
                                     && currentLine[colonPosition + 1] == '\t');
            if (hasTabAfterColon)
                return false;
            if (currentLine.substr(0, colonPosition) == "Host")
                hasHostHeader = true;
        }
        currentPosition = lineEndPosition + 2;
    }

    if (version == "HTTP/1.1" && !hasHostHeader)
        return false;

    return true;
}

void RequestParser::parseFirstLine(const std::string& firstLine, HttpRequest& request)
{
    std::istringstream lineStream(firstLine);
    lineStream >> request.method;
    lineStream >> request.uri;
    lineStream >> request.version;
}

void RequestParser::parseHeaders(const std::string& rawRequest, HttpRequest& request,
                                  std::size_t firstLineEnd, std::size_t headerBodySeparator)
{
    std::size_t currentPosition = firstLineEnd + 2;
    std::size_t headersEnd      = headerBodySeparator;

    while (currentPosition < headersEnd)
    {
        std::size_t lineEndPosition = rawRequest.find("\r\n", currentPosition);
        std::string currentLine     = rawRequest.substr(currentPosition, lineEndPosition - currentPosition);

        std::size_t colonPosition = currentLine.find(':');
        if (colonPosition != std::string::npos)
        {
            std::string headerKey   = currentLine.substr(0, colonPosition);
            std::string headerValue;
            std::size_t valueStart  = colonPosition + 1;
            while (valueStart < currentLine.size() && currentLine[valueStart] == ' ')
                valueStart++;
            if (valueStart < currentLine.size())
                headerValue = currentLine.substr(valueStart);
            request.headers[headerKey] = headerValue;
        }
        currentPosition = lineEndPosition + 2;
    }
}

void RequestParser::parseBody(const std::string& rawRequest, HttpRequest& request,
                               std::size_t headerBodySeparator)
{
    request.body = rawRequest.substr(headerBodySeparator + 4);
}

HttpRequest RequestParser::parse(const std::string& rawRequest)
{
    HttpRequest request;

    if (!isValid(rawRequest))
        return request;

    std::size_t firstLineEnd        = rawRequest.find("\r\n");
    std::size_t headerBodySeparator = rawRequest.find("\r\n\r\n");

    parseFirstLine(rawRequest.substr(0, firstLineEnd), request);
    parseHeaders(rawRequest, request, firstLineEnd, headerBodySeparator);
    parseBody(rawRequest, request, headerBodySeparator);
    return request;
}
