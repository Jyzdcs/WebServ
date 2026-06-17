#include "../../../include/http/ResponseBuilder.hpp"
#include <sstream>

std::string ResponseBuilder::build(const HttpResponse& response)
{
    std::ostringstream rawOutput;

    rawOutput << "HTTP/1.1 " << response.status_code << " " << response.status_msg << "\r\n";

    std::map<std::string, std::string>::const_iterator headerIt;
    for (headerIt = response.headers.begin(); headerIt != response.headers.end(); ++headerIt)
        rawOutput << headerIt->first << ": " << headerIt->second << "\r\n";

    rawOutput << "\r\n";
    rawOutput << response.body;

    return rawOutput.str();
}
