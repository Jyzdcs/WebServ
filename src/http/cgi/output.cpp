#include "../../../include/http/CgiHandler.hpp"
#include <sstream>

HttpResponse CgiHandler::parseOutput(const std::string& raw)
{
    HttpResponse res;

    std::size_t sep = raw.find("\r\n\r\n");
    if (sep == std::string::npos)
        sep = raw.find("\n\n");
    if (sep == std::string::npos)
    {
        res.status_code = 200;
        res.status_msg  = "OK";
        res.body        = raw;
        res.headers["Content-Type"] = "text/html";
        std::ostringstream ss;
        ss << res.body.size();
        res.headers["Content-Length"] = ss.str();
        return res;
    }

    std::string headersPart = raw.substr(0, sep);
    std::size_t bodyStart   = sep + (raw[sep + 1] == '\n' ? 2 : 4);
    res.body = raw.substr(bodyStart);

    res.status_code = 200;
    res.status_msg  = "OK";

    std::istringstream stream(headersPart);
    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        std::size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key   = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        std::size_t vs = 0;
        while (vs < value.size() && value[vs] == ' ')
            vs++;
        value = value.substr(vs);

        if (key == "Status")
        {
            std::istringstream ss(value);
            ss >> res.status_code;
            std::size_t sp = value.find(' ');
            if (sp != std::string::npos)
                res.status_msg = value.substr(sp + 1);
        }
        else
            res.headers[key] = value;
    }

    if (res.headers.find("Content-Type") == res.headers.end())
        res.headers["Content-Type"] = "text/html";

    std::ostringstream ss;
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();

    return res;
}

HttpResponse CgiHandler::buildError(int code, const std::string& msg)
{
    HttpResponse       res;
    std::ostringstream ss;

    res.status_code = code;
    res.status_msg  = msg;
    res.body        = "<html><body><h1>" + msg + "</h1></body></html>";
    res.headers["Content-Type"] = "text/html";
    ss << res.body.size();
    res.headers["Content-Length"] = ss.str();
    return res;
}
