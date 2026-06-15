#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include "ServerConfig.hpp"   // Config contient des ServerConfig par valeur -> include requis.

// Conteneur de haut niveau : ne réalise aucun parsing, stocke les ServerConfig
// (produits par le parser ou construits manuellement) et donne accès à ceux-ci.
class Config {
private:
    std::vector<ServerConfig>  _servers;   // 1 ou N blocs server -> conteneur dynamique.

public:
    Config();
    ~Config();

    // Écriture : ajoute un serveur à la configuration.
    void addServer(const ServerConfig& server);

    // Lecture seule : retourne une référence (évite la copie du vector).
    const std::vector<ServerConfig>&  getServers() const;
};

#endif
