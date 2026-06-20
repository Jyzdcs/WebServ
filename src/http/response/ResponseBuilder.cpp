#include "../../../include/http/ResponseBuilder.hpp"
#include <sstream>
#include <ctime>

std::string ResponseBuilder::build(const HttpResponse& response)
{
    std::ostringstream rawOutput;

    // ex: "HTTP/1.1 200 OK"  ou  "HTTP/1.1 404 Not Found"
    rawOutput << "HTTP/1.1 " << response.status_code << " " << response.status_msg << "\r\n";

    // headers fixes présents dans toutes les réponses
    rawOutput << "Server: webserv/1.0\r\n";

    // date au format HTTP : "Fri, 19 Jun 2026 14:42:00 GMT"
    time_t     now = time(NULL);
    struct tm* gmt = gmtime(&now);
    char       dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    rawOutput << "Date: " << dateBuf << "\r\n";

    // on ferme la connexion après chaque réponse
    rawOutput << "Connection: close\r\n";

    // headers variables : Content-Type, Content-Length, Location, etc.
    std::map<std::string, std::string>::const_iterator headerIt;
    for (headerIt = response.headers.begin(); headerIt != response.headers.end(); ++headerIt)
        rawOutput << headerIt->first << ": " << headerIt->second << "\r\n";

    // ligne vide obligatoire qui sépare les headers du body
    rawOutput << "\r\n";
    rawOutput << response.body;

    return rawOutput.str();
}
