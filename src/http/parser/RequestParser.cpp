#include "../../../include/http/RequestParser.hpp"
#include <sstream>

void RequestParser::parseRequestLine(const std::string& firstLine, HttpRequest& request)
{
    std::istringstream lineStream(firstLine);
    std::string        extraToken;

    // Erreur: "GET /\r\n\r\n"  (version manquante, moins de 3 tokens)
    if (!(lineStream >> request.method >> request.uri >> request.version))
        throw ParseException(400, "Bad Request");

    // Erreur: "GET / HTTP/1.1 extra\r\n\r\n"  (4ème token sur la request-line)
    if (lineStream >> extraToken)
        throw ParseException(400, "Bad Request");

    // Erreur: "get / HTTP/1.1\r\n\r\n"  (méthode en minuscules)
    for (std::size_t i = 0; i < request.method.size(); i++)
        if (request.method[i] < 'A' || request.method[i] > 'Z')
            throw ParseException(400, "Bad Request");

    // Erreur: "GET / MYPROTO/1.1\r\n\r\n"  (protocole inconnu)
    if (request.version.substr(0, 5) != "HTTP/")
        throw ParseException(400, "Bad Request");

    // Erreur: "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n"  (version non supportée)
    if (request.version != "HTTP/1.0" && request.version != "HTTP/1.1")
        throw ParseException(505, "HTTP Version Not Supported");
}

void RequestParser::parseHeaderLine(const std::string& line,
                                     std::string& headerKey, std::string& headerValue)
{
    std::size_t colonPosition = line.find(':');
    if (colonPosition == std::string::npos)
        return;

    // Erreur: "Host:\tlocalhost"  (tab juste après le colon)
    if (colonPosition + 1 < line.size() && line[colonPosition + 1] == '\t')
        throw ParseException(400, "Bad Request");

    headerKey = line.substr(0, colonPosition);

    std::size_t valueStart = colonPosition + 1;
    while (valueStart < line.size() && line[valueStart] == ' ')
        valueStart++;

    if (valueStart < line.size())
        headerValue = line.substr(valueStart);
    else
        headerValue = "";
}

void RequestParser::validateHostHeader(const std::string& headerValue, bool& hasHostHeader)
{
    // Erreur: "Host:\r\n" ou "Host:   \r\n"  (valeur vide ou whitespace-only)
    if (headerValue.empty())
        throw ParseException(400, "Bad Request");

    // Erreur: double Host header
    if (hasHostHeader)
        throw ParseException(400, "Bad Request");

    hasHostHeader = true;
}

void RequestParser::parseHeaders(const std::string& rawRequest, HttpRequest& request,
                                  std::size_t firstLineEnd, std::size_t headerBodySeparator)
{
    bool        hasHostHeader          = false;
    bool        hasContentLengthHeader = false;
    std::size_t currentPosition        = firstLineEnd + 2;

    while (currentPosition < headerBodySeparator)
    {
        std::size_t lineEndPosition = rawRequest.find("\r\n", currentPosition);
        std::string currentLine     = rawRequest.substr(currentPosition, lineEndPosition - currentPosition);

        std::string headerKey;
        std::string headerValue;
        RequestParser::parseHeaderLine(currentLine, headerKey, headerValue);

        if (!headerKey.empty())
        {
            request.headers[headerKey] = headerValue;

            // les noms de headers sont case-insensitive : host/HOST/HoSt sont tous valides pour NGINX
            std::string headerKeyLower = headerKey;
            for (std::size_t i = 0; i < headerKeyLower.size(); i++)
                headerKeyLower[i] = std::tolower(headerKeyLower[i]);
            if (headerKeyLower == "host")
                RequestParser::validateHostHeader(headerValue, hasHostHeader);

            // Erreur: double Content-Length
            if (headerKeyLower == "content-length")
            {
                if (hasContentLengthHeader)
                    throw ParseException(400, "Bad Request");
                hasContentLengthHeader = true;
            }
        }
        currentPosition = lineEndPosition + 2;
    }

    // Erreur: "GET / HTTP/1.1\r\nContent-Type: text/html\r\n\r\n"  (pas de Host, obligatoire en HTTP/1.1)
    if (request.version == "HTTP/1.1" && !hasHostHeader)
        throw ParseException(400, "Bad Request");
}

void RequestParser::parseBody(const std::string& rawRequest, HttpRequest& request,
                               std::size_t headerBodySeparator)
{
    std::string rawBody = rawRequest.substr(headerBodySeparator + 4);

    std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Length");
    if (it != request.headers.end())
    {
        const std::string& clValue = it->second;

        // Erreur: "Content-Length: abc" ou "Content-Length: -1"  (pas un entier positif)
        for (std::size_t i = 0; i < clValue.size(); i++)
            if (clValue[i] < '0' || clValue[i] > '9')
                throw ParseException(400, "Bad Request");

        std::istringstream iss(clValue);
        std::size_t        contentLength = 0;
        iss >> contentLength;
        request.body = rawBody.substr(0, contentLength);
    }
    else
        request.body = rawBody;
}

HttpRequest RequestParser::parse(const std::string& rawRequest)
{
    std::size_t headerBodySeparator = rawRequest.find("\r\n\r\n");

    // Erreur: "GET / HTTP/1.1\r\nHost: localhost\r\n"  (headers jamais fermés)
    if (headerBodySeparator == std::string::npos)
        throw ParseException(400, "Bad Request");

    HttpRequest request;
    std::size_t firstLineEnd = rawRequest.find("\r\n");

    RequestParser::parseRequestLine(rawRequest.substr(0, firstLineEnd), request);
    RequestParser::parseHeaders(rawRequest, request, firstLineEnd, headerBodySeparator);
    RequestParser::parseBody(rawRequest, request, headerBodySeparator);

    return request;
}
