#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include "ServerConfig.hpp"   // Config CONTIENT des ServerConfig -> include obligatoire.

// Config = la "boîte à archives" : elle ne parse RIEN, elle range les ServerConfig
// parsés (ou ici, fabriqués en dur par le mock) et permet de les consulter.
class Config {
private:
    std::vector<ServerConfig>  _servers;   // 1 ou N blocs server -> conteneur dynamique.

public:
    Config();
    ~Config();

    // Écriture : appelé par le parser (ou le mock) pour remplir la boîte.
    void addServer(const ServerConfig& server);

    // Lecture : appelé par tes potes pour consulter la config (copie évitée, lecture seule).
    const std::vector<ServerConfig>&  getServers() const;
};

#endif
