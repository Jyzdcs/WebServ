#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include <sstream>

std::vector<std::string> CgiHandler::buildEnv(const HttpRequest& request,
                                               const std::string& scriptPath)
{
    std::vector<std::string> envVars;

    envVars.push_back("REQUEST_METHOD=" + request.method);
    envVars.push_back("QUERY_STRING="   + extractQueryString(request.uri));
    envVars.push_back("SCRIPT_FILENAME=" + scriptPath);

    std::string pathWithoutQuery = request.uri;
    std::size_t queryPosition    = pathWithoutQuery.find('?');
    if (queryPosition != std::string::npos)
        pathWithoutQuery = pathWithoutQuery.substr(0, queryPosition);
    envVars.push_back("PATH_INFO=" + pathWithoutQuery);

    std::map<std::string, std::string>::const_iterator headerIt;

    headerIt = request.headers.find("Content-Type");
    if (headerIt != request.headers.end())
        envVars.push_back("CONTENT_TYPE=" + headerIt->second);
    else
        envVars.push_back("CONTENT_TYPE=");

    if (!request.body.empty())
    {
        std::ostringstream bodyLengthStream;
        bodyLengthStream << request.body.size();
        envVars.push_back("CONTENT_LENGTH=" + bodyLengthStream.str());
    }
    else
        envVars.push_back("CONTENT_LENGTH=0");

    headerIt = request.headers.find("Host");
    if (headerIt != request.headers.end())
        envVars.push_back("HTTP_HOST=" + headerIt->second);

    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");

    return envVars;
}
