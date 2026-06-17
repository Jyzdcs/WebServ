#include "../../../include/http/CgiHandler.hpp"
#include <sstream>

static std::string getQueryString(const std::string& uri)
{
    std::size_t q = uri.find('?');
    if (q == std::string::npos)
        return "";
    return uri.substr(q + 1);
}

std::vector<std::string> CgiHandler::buildEnv(const HttpRequest& req, const std::string& scriptPath)
{
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + req.method);
    env.push_back("QUERY_STRING=" + getQueryString(req.uri));
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("PATH_INFO=" + req.uri);

    std::map<std::string, std::string>::const_iterator it;

    it = req.headers.find("Content-Type");
    if (it != req.headers.end())
        env.push_back("CONTENT_TYPE=" + it->second);
    else
        env.push_back("CONTENT_TYPE=");

    if (!req.body.empty())
    {
        std::ostringstream ss;
        ss << req.body.size();
        env.push_back("CONTENT_LENGTH=" + ss.str());
    }
    else
        env.push_back("CONTENT_LENGTH=0");

    it = req.headers.find("Host");
    if (it != req.headers.end())
        env.push_back("HTTP_HOST=" + it->second);

    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");

    return env;
}
