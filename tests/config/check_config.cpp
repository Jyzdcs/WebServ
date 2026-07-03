#include "../../include/config/ConfigParser.hpp"
#include "../../include/config/ConfigValidator.hpp"
#include <iostream>

// Mini-outil : parse + valide UN fichier de config et sort 0 (OK) ou 1 (KO).
// Utilisé par run_config_tests.sh pour vérifier tous les .conf du repo.
// (On ne réutilise pas ./webserv : lui finira par lancer la boucle serveur.)

int main(int argc, char** argv)
{
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";

    try
    {
        ConfigParser parser;
        ConfigValidator validator;

        Config config = parser.parse(path);
        validator.validate(config);
        std::cout << "OK: " << path << " (" << config.getServers().size()
                  << " server(s))" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "REJECTED: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
