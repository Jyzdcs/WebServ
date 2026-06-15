#include "Config.hpp"

// Constructeur vide : _servers (un std::vector) se construit tout seul, vide.
Config::Config() {}

// Destructeur vide : _servers libère sa propre mémoire automatiquement (RAII).
Config::~Config() {}

void Config::addServer(const ServerConfig& server)
{
    _servers.push_back(server);   // ajoute une COPIE du server dans le vector.
}

const std::vector<ServerConfig>& Config::getServers() const
{
    return _servers;
}
