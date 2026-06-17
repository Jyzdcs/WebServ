#include "../../../include/http/Router.hpp"

static bool matchesLocation(const std::string& locationPath, const std::string& requestUri)
{
    if (locationPath.size() > requestUri.size())
        return false;
    if (requestUri.substr(0, locationPath.size()) != locationPath)
        return false;

    bool uriContinuesAfterPrefix = locationPath.size() < requestUri.size();
    bool nextCharIsSlash         = requestUri[locationPath.size()] == '/';
    bool locationIsRoot          = locationPath == "/";

    if (uriContinuesAfterPrefix && !nextCharIsSlash && !locationIsRoot)
        return false;

    return true;
}

LocationConfig Router::route(const HttpRequest& request, const ServerConfig& server)
{
    const std::vector<LocationConfig>& locations = server.getLocations();

    LocationConfig bestMatch;
    std::size_t    longestMatchLength = 0;

    for (std::vector<LocationConfig>::const_iterator locationIt = locations.begin();
         locationIt != locations.end(); ++locationIt)
    {
        const std::string& locationPath = locationIt->getPath();

        if (matchesLocation(locationPath, request.uri) && locationPath.size() > longestMatchLength)
        {
            bestMatch           = *locationIt;
            longestMatchLength  = locationPath.size();
        }
    }
    return bestMatch;
}
