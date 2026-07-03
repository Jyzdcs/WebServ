#include "include/config/ConfigParser.hpp"
#include "include/config/ConfigValidator.hpp"
#include <iostream>

// Point d'entrée de webserv.
// Usage : ./webserv [fichier_de_config]
// Sans argument, on retombe sur config/default.conf (exigence du sujet).
//
// Pour l'instant le main s'arrête après le chargement de la config :
// le branchement de la boucle serveur (PollManager + Socket) se fera
// au point d'intégration 1, quand le contrat d'interface sera figé.
int main(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
    std::string path = (argc == 2) ? argv[1] : "config/default.conf";

    Config config;
    try
    {
        ConfigParser parser;
        ConfigValidator validator;

        config = parser.parse(path);
        validator.validate(config);
    }
    catch (const std::exception& e)
    {
        // Config invalide = message clair et sortie propre, jamais de crash.
        std::cerr << "webserv: " << path << ": " << e.what() << std::endl;
        return 1;
    }

    const std::vector<ServerConfig>& servers = config.getServers();
    std::cout << "Config OK: " << servers.size() << " server(s)" << std::endl;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::cout << "  - " << servers[i].getHost() << ":" << servers[i].getPort()
                  << " (" << servers[i].getLocations().size() << " location(s))"
                  << std::endl;
    }

    // TODO(core-server): démarrer les Socket + PollManager avec cette config.
    return 0;
}
