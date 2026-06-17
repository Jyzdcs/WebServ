#include "../../../include/http/Router.hpp"

static bool matches(const std::string& locationPath, const std::string& uri)
{
    if (locationPath.size() > uri.size())
        return false;
    if (uri.substr(0, locationPath.size()) != locationPath)
        return false;
    if (locationPath.size() < uri.size() && uri[locationPath.size()] != '/' && locationPath != "/")
        return false;
    return true;
}

LocationConfig Router::route(const HttpRequest& req, const ServerConfig& server)
{
    const std::vector<LocationConfig>& locs = server.getLocations();

    LocationConfig best;
    std::size_t    bestLen = 0;

    for (std::vector<LocationConfig>::const_iterator it = locs.begin(); it != locs.end(); ++it)
    {
        const std::string& path = it->getPath();

        if (matches(path, req.uri) && path.size() > bestLen)
        {
            best    = *it;
            bestLen = path.size();
        }
    }
    return best;
}
