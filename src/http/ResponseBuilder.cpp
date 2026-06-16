#include "../../include/http/ResponseBuilder.hpp"
#include <sstream>

std::string ResponseBuilder::build(const HttpResponse& res)
{
    std::ostringstream out;

    out << "HTTP/1.1 " << res.status_code << " " << res.status_msg << "\r\n";

    std::map<std::string, std::string>::const_iterator it;
    for (it = res.headers.begin(); it != res.headers.end(); ++it)
        out << it->first << ": " << it->second << "\r\n";

    out << "\r\n";
    out << res.body;

    return out.str();
}
