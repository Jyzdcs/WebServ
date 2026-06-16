#include "../../include/http/Router.hpp"

LocationConfig Router::route(const HttpRequest& req, const ServerConfig& server)
{
    const std::vector<LocationConfig>& locs = server.getLocations();

    LocationConfig  best;
    std::size_t     bestLen = 0;

    for (std::vector<LocationConfig>::const_iterator it = locs.begin(); it != locs.end(); ++it)
    {
        const std::string& path = it->getPath();

        if (path.size() > req.uri.size())
            continue;

        if (req.uri.substr(0, path.size()) != path)
            continue;

        if (path.size() < req.uri.size() && req.uri[path.size()] != '/' && path != "/")
            continue;

        if (path.size() > bestLen)
        {
            best    = *it;
            bestLen = path.size();
        }
    }
    return best;
}
