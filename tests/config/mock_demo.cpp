#include "../../include/config/ConfigMock.hpp"
#include <iostream>

// Démonstration de consommation : parcourt le Config via les getters, comme le
// feront les modules core server et http layer. Sert à valider l'interface.
// Entrée temporaire, à remplacer par le main() réel.
// C++98 : itération via const_iterator (le range-for est du C++11).
int main()
{
    Config config = createMockConfig();

    const std::vector<ServerConfig>& servers = config.getServers();

    for (std::vector<ServerConfig>::const_iterator s = servers.begin(); s != servers.end(); ++s)
    {
        std::cout << "Server " << s->getHost() << ":" << s->getPort()
                  << " (" << s->getServerName() << ")" << std::endl;
        std::cout << "  max body size: " << s->getMaxBodySize() << std::endl;

        const std::vector<LocationConfig>& locs = s->getLocations();
        for (std::vector<LocationConfig>::const_iterator l = locs.begin(); l != locs.end(); ++l)
        {
            std::cout << "  location " << l->getPath();
            if (!l->getRedirectUrl().empty())
                std::cout << " -> redirect " << l->getRedirectUrl();
            else if (!l->getCgiPath().empty())
                std::cout << " -> CGI " << l->getCgiExtension() << " via " << l->getCgiPath();
            else
                std::cout << " -> root " << l->getRoot();
            std::cout << std::endl;
        }
    }
    return 0;
}
