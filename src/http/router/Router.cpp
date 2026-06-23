#include "../../../include/http/Router.hpp"

// vérifie si une location de config matche l'URI de la requête
// ex: locationPath="/uploads", requestUri="/uploads/photo.jpg" → true
// ex: locationPath="/uploads", requestUri="/uploadsfoo"        → false (pas de / après le prefix)
static bool matchesLocation(const std::string& locationPath, const std::string& requestUri)
{
    // le prefix doit tenir dans l'URI
    if (locationPath.size() > requestUri.size())
        return false;

    // l'URI doit commencer par le prefix exact
    if (requestUri.substr(0, locationPath.size()) != locationPath)
        return false;

    bool uriContinuesAfterPrefix = locationPath.size() < requestUri.size();
    bool nextCharIsSlash         = requestUri[locationPath.size()] == '/';
    bool locationIsRoot          = locationPath == "/";

    // Erreur: "/uploadsfoo" ne doit pas matcher "/uploads" (le char suivant doit être /)
    if (uriContinuesAfterPrefix && !nextCharIsSlash && !locationIsRoot)
        return false;

    return true;
}

LocationConfig Router::route(const HttpRequest& request, const ServerConfig& server)
{
    const std::vector<LocationConfig>& locations = server.getLocations();

    // strip le query string pour ne comparer que le path
    // ex: "/uploads/photo.jpg?size=large" → "/uploads/photo.jpg"
    std::string uriPath  = request.uri;
    std::size_t queryPos = uriPath.find('?');
    if (queryPos != std::string::npos)
        uriPath = uriPath.substr(0, queryPos);

    LocationConfig bestMatch;
    std::size_t    longestMatchLength = 0;

    // longest prefix match : la location la plus spécifique gagne
    // ex: "/uploads/images" gagne sur "/uploads" pour "/uploads/images/foo.png"
    for (std::vector<LocationConfig>::const_iterator locationIt = locations.begin();
         locationIt != locations.end(); ++locationIt)
    {
        // strip le trailing slash de la location sauf si c'est "/"
        // ex: "/uploads/" → "/uploads"
        std::string locationPath = locationIt->getPath();
        if (locationPath.size() > 1 && locationPath[locationPath.size() - 1] == '/')
            locationPath = locationPath.substr(0, locationPath.size() - 1);

        if (matchesLocation(locationPath, uriPath) && locationPath.size() > longestMatchLength)
        {
            bestMatch          = *locationIt;
            longestMatchLength = locationPath.size();
        }
    }
    return bestMatch;
}
