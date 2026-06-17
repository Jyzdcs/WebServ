#include "../../../include/http/ResponseBuilder.hpp"
#include <sstream>
#include <ctime>

std::string ResponseBuilder::build(const HttpResponse& response)
{
    std::ostringstream rawOutput;

    rawOutput << "HTTP/1.1 " << response.status_code << " " << response.status_msg << "\r\n";

    time_t     now = time(NULL);
    struct tm* gmt = gmtime(&now);
    char       dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    rawOutput << "Date: " << dateBuf << "\r\n";
    rawOutput << "Connection: close\r\n";

    std::map<std::string, std::string>::const_iterator headerIt;
    for (headerIt = response.headers.begin(); headerIt != response.headers.end(); ++headerIt)
        rawOutput << headerIt->first << ": " << headerIt->second << "\r\n";

    rawOutput << "\r\n";
    rawOutput << response.body;

    return rawOutput.str();
}
