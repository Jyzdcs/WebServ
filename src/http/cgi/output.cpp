#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include <sstream>

HttpResponse CgiHandler::parseOutput(const std::string& cgiOutput)
{
    HttpResponse response;

    std::size_t separatorPos   = cgiOutput.find("\r\n\r\n");
    bool        usesDoubleCRLF = (separatorPos != std::string::npos);
    if (separatorPos == std::string::npos)
        separatorPos = cgiOutput.find("\n\n");

    // Pas de séparateur header/body → tout le output est le body
    if (separatorPos == std::string::npos)
        return buildHttpOk(cgiOutput, "text/html");

    std::size_t bodyStart = separatorPos + (usesDoubleCRLF ? 4 : 2);

    std::string headersSection = cgiOutput.substr(0, separatorPos);
    response.body              = cgiOutput.substr(bodyStart);
    response.status_code       = 200;
    response.status_msg        = "OK";

    std::istringstream headerStream(headersSection);
    std::string        headerLine;

    while (std::getline(headerStream, headerLine))
    {
        if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r')
            headerLine.erase(headerLine.size() - 1);
        if (headerLine.empty())
            continue;

        std::size_t colonPos = headerLine.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string headerKey   = headerLine.substr(0, colonPos);
        std::string headerValue = headerLine.substr(colonPos + 1);

        std::size_t valueStart = 0;
        while (valueStart < headerValue.size() && headerValue[valueStart] == ' ')
            valueStart++;
        headerValue = headerValue.substr(valueStart);

        if (headerKey == "Status")
        {
            std::istringstream statusStream(headerValue);
            statusStream >> response.status_code;

            std::size_t statusSpacePos = headerValue.find(' ');
            if (statusSpacePos != std::string::npos)
                response.status_msg = headerValue.substr(statusSpacePos + 1);
        }
        else
            response.headers[headerKey] = headerValue;
    }

    if (response.headers.find("Content-Type") == response.headers.end())
        response.headers["Content-Type"] = "text/html";

    std::ostringstream contentLengthStream;
    contentLengthStream << response.body.size();
    response.headers["Content-Length"] = contentLengthStream.str();

    return response;
}
