#include "../../../include/http/RequestParser.hpp"
#include <sstream>

void RequestParser::isValid(const std::string& rawRequest)
{
    // Erreur: "GET / HTTP/1.1\r\nHost: localhost\r\n"  (headers jamais fermés)
    if (rawRequest.find("\r\n\r\n") == std::string::npos)
        throw ParseException(400, "Bad Request");

    std::size_t firstLineEnd        = rawRequest.find("\r\n");
    std::size_t headerBodySeparator = rawRequest.find("\r\n\r\n");
    std::string firstLine           = rawRequest.substr(0, firstLineEnd);

    std::istringstream firstLineStream(firstLine);
    std::string        method, uri, version;

    // Erreur: "GET /\r\n\r\n"  (version manquante, moins de 3 tokens)
    if (!(firstLineStream >> method >> uri >> version))
        throw ParseException(400, "Bad Request");

    std::string extraToken;
    // Erreur: "GET / HTTP/1.1 extra\r\n\r\n"  (4ème token sur la request-line)
    if (firstLineStream >> extraToken)
        throw ParseException(400, "Bad Request");

    // Erreur: "GET / MYPROTO/1.1\r\n\r\n"  (protocole inconnu)
    if (version.substr(0, 5) != "HTTP/")
        throw ParseException(400, "Bad Request");

    // Erreur: "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n"  (version non supportée)
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
        throw ParseException(505, "HTTP Version Not Supported");

    bool        hasHostHeader   = false;
    std::size_t currentPosition = firstLineEnd + 2;

    // vérifie chaque header : pas de tab après le colon, détecte la présence du header Host
    // ex: "Host: localhost\r\nContent-Type: text/html\r\n" → deux passages dans la boucle
    while (currentPosition < headerBodySeparator)
    {
        std::size_t lineEndPosition = rawRequest.find("\r\n", currentPosition);
        std::string currentLine     = rawRequest.substr(currentPosition, lineEndPosition - currentPosition);

        std::size_t colonPosition = currentLine.find(':');
        if (colonPosition != std::string::npos)
        {
            // Erreur: "Host:\tlocalhost"  (tab juste après le colon)
            bool hasTabAfterColon = (colonPosition + 1 < currentLine.size()
                                     && currentLine[colonPosition + 1] == '\t');
            if (hasTabAfterColon)
                throw ParseException(400, "Bad Request");

            // les noms de headers sont case-insensitive : host/HOST/HoSt sont tous valides
            std::string headerName      = currentLine.substr(0, colonPosition);
            std::string headerNameLower = headerName;
            for (std::size_t i = 0; i < headerNameLower.size(); i++)
                headerNameLower[i] = std::tolower(headerNameLower[i]);
            if (headerNameLower == "host")
                hasHostHeader = true;
        }
        currentPosition = lineEndPosition + 2;
    }

    // Erreur: "GET / HTTP/1.1\r\nContent-Type: text/html\r\n\r\n"  (pas de Host, obligatoire en HTTP/1.1)
    if (version == "HTTP/1.1" && !hasHostHeader)
        throw ParseException(400, "Bad Request");
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
    isValid(rawRequest);

    HttpRequest request;
    std::size_t firstLineEnd        = rawRequest.find("\r\n");
    std::size_t headerBodySeparator = rawRequest.find("\r\n\r\n");

    parseFirstLine(rawRequest.substr(0, firstLineEnd), request);
    parseHeaders(rawRequest, request, firstLineEnd, headerBodySeparator);
    parseBody(rawRequest, request, headerBodySeparator);
    return request;
}
