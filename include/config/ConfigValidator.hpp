#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include <string>
#include <vector>
#include "Config.hpp"

// Validation SÉMANTIQUE de la config, à lancer APRÈS le parsing.
// Le parser vérifie la grammaire ("est-ce bien écrit ?"),
// le validator vérifie le sens ("est-ce que ça veut dire quelque chose ?").
// Exemple : "listen 99999;" passe le parser (c'est un nombre) mais
// échoue ici (un port > 65535 n'existe pas).
// En cas de problème : throw std::runtime_error avec un message clair
// qui indique quel serveur / quelle location est en cause.
class ConfigValidator {
private:
    // Vérifie un bloc server : listen, host, error_pages, locations.
    void validateServer(const ServerConfig& server, size_t index);

    // Vérifie un bloc location : path, méthodes, présence d'une action.
    void validateLocation(const LocationConfig& location, size_t serverIndex);

    // Deux servers sur le même host:port -> on ne peut pas bind 2 fois.
    void checkDuplicateHostPort(const std::vector<ServerConfig>& servers);

    // Deux locations avec le même path dans un même server -> ambigu.
    void checkDuplicateLocationPath(const ServerConfig& server, size_t index);

    // Seules les méthodes que notre serveur implémente sont acceptées.
    bool isAllowedMethod(const std::string& method);

    // Construit le préfixe d'erreur "server[i]: " pour situer le problème.
    std::string where(size_t serverIndex);

public:
    void validate(const Config& config);
};

#endif
