#include "../../include/config/ConfigValidator.hpp"
#include <sstream>
#include <stdexcept>

// Point d'entrée : vérifie toute la config, server par server.
// L'ordre des checks va du général (y a-t-il des servers ?)
// au particulier (chaque location de chaque server).
void ConfigValidator::validate(const Config& config)
{
    const std::vector<ServerConfig>& servers = config.getServers();

    if (servers.empty())
        throw std::runtime_error("config: no server block found");

    for (size_t i = 0; i < servers.size(); ++i)
        validateServer(servers[i], i);

    checkDuplicateHostPort(servers);
}

// Préfixe commun des messages d'erreur, ex: "server[0]: ".
// Permet à l'utilisateur de retrouver le bloc fautif dans son .conf.
std::string ConfigValidator::where(size_t serverIndex)
{
    std::ostringstream oss;
    oss << "server[" << serverIndex << "]: ";
    return oss.str();
}

void ConfigValidator::validateServer(const ServerConfig& server, size_t index)
{
    // Le port par défaut est 0 (constructeur) : si on le voit encore,
    // c'est que la directive 'listen' n'était pas dans le fichier.
    if (server.getPort() == 0)
        throw std::runtime_error(where(index) + "missing 'listen' directive");
    if (server.getPort() < 1 || server.getPort() > 65535)
    {
        std::ostringstream oss;
        oss << where(index) << "invalid port " << server.getPort()
            << " (must be between 1 and 65535)";
        throw std::runtime_error(oss.str());
    }

    // Même logique : host vide = directive absente.
    if (server.getHost().empty())
        throw std::runtime_error(where(index) + "missing 'host' directive");

    // Un server sans location ne peut répondre à aucune requête :
    // le Router fait sa correspondance sur les locations.
    if (server.getLocations().empty())
        throw std::runtime_error(where(index) + "must have at least one location block");

    // Les codes d'erreur HTTP valides vont de 300 à 599
    // (on autorise les 3xx pour les pages de redirection d'erreur).
    const std::map<int, std::string>& pages = server.getErrorPages();
    for (std::map<int, std::string>::const_iterator it = pages.begin(); it != pages.end(); ++it)
    {
        if (it->first < 300 || it->first > 599)
        {
            std::ostringstream oss;
            oss << where(index) << "invalid error_page code " << it->first;
            throw std::runtime_error(oss.str());
        }
    }

    const std::vector<LocationConfig>& locations = server.getLocations();
    for (size_t i = 0; i < locations.size(); ++i)
        validateLocation(locations[i], index);

    checkDuplicateLocationPath(server, index);
}

void ConfigValidator::validateLocation(const LocationConfig& location, size_t serverIndex)
{
    const std::string& path = location.getPath();

    // Un path de location est toujours absolu dans l'URI, donc commence par '/'.
    if (path.empty() || path[0] != '/')
        throw std::runtime_error(where(serverIndex)
            + "location path '" + path + "' must start with '/'");

    // On n'accepte que les méthodes que le serveur sait traiter.
    const std::vector<std::string>& methods = location.getAllowedMethods();
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (!isAllowedMethod(methods[i]))
            throw std::runtime_error(where(serverIndex) + "location '" + path
                + "': unknown method '" + methods[i] + "' (allowed: GET POST DELETE)");
        // Doublon de méthode = probablement une faute de frappe dans le .conf.
        for (size_t j = i + 1; j < methods.size(); ++j)
        {
            if (methods[i] == methods[j])
                throw std::runtime_error(where(serverIndex) + "location '" + path
                    + "': duplicate method '" + methods[i] + "'");
        }
    }

    // Une location doit savoir QUOI faire d'une requête :
    // servir des fichiers (root), rediriger (redirect) ou exécuter (cgi).
    // Sans aucun des trois, elle est inutilisable.
    bool hasRoot = !location.getRoot().empty();
    bool hasRedirect = !location.getRedirectUrl().empty();
    bool hasCgi = !location.getCgiExtension().empty();
    if (!hasRoot && !hasRedirect && !hasCgi)
        throw std::runtime_error(where(serverIndex) + "location '" + path
            + "' has no 'root', 'redirect' or 'cgi_extensions' (nothing to do)");

    // Accepter des uploads sans autoriser POST n'a pas de sens :
    // c'est POST qui transporte le fichier.
    if (!location.getUploadPath().empty())
    {
        bool hasPost = false;
        for (size_t i = 0; i < methods.size(); ++i)
        {
            if (methods[i] == "POST")
                hasPost = true;
        }
        if (!hasPost)
            throw std::runtime_error(where(serverIndex) + "location '" + path
                + "': 'upload_store' requires POST in 'methods'");
    }
}

// Deux servers sur le même host:port :
//   - AUTORISÉ si leurs server_name diffèrent -> virtual hosting par nom,
//     le server est choisi via le header Host (grille d'éval : "plusieurs
//     serveurs avec différents hostnames", curl --resolve).
//   - REFUSÉ sinon (même nom, ou pas de nom) : impossible de les départager,
//     c'est le cas "même port deux fois -> refus de démarrer" du sujet.
void ConfigValidator::checkDuplicateHostPort(const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        for (size_t j = i + 1; j < servers.size(); ++j)
        {
            if (servers[i].getHost() != servers[j].getHost()
                || servers[i].getPort() != servers[j].getPort())
                continue;

            const std::string& nameI = servers[i].getServerName();
            const std::string& nameJ = servers[j].getServerName();
            if (!nameI.empty() && !nameJ.empty() && nameI != nameJ)
                continue; // virtual hosts : le header Host les départagera

            std::ostringstream oss;
            oss << "server[" << j << "] duplicates host:port "
                << servers[i].getHost() << ":" << servers[i].getPort()
                << " of server[" << i << "]"
                << " (add distinct server_name to both for virtual hosting)";
            throw std::runtime_error(oss.str());
        }
    }
}

// Deux locations "/uploads" dans le même server : le Router ne saurait
// pas laquelle choisir, on refuse dès le démarrage.
void ConfigValidator::checkDuplicateLocationPath(const ServerConfig& server, size_t index)
{
    const std::vector<LocationConfig>& locations = server.getLocations();

    for (size_t i = 0; i < locations.size(); ++i)
    {
        for (size_t j = i + 1; j < locations.size(); ++j)
        {
            if (locations[i].getPath() == locations[j].getPath())
                throw std::runtime_error(where(index) + "duplicate location '"
                    + locations[i].getPath() + "'");
        }
    }
}

// La liste des méthodes que notre webserv implémente réellement.
bool ConfigValidator::isAllowedMethod(const std::string& method)
{
    return (method == "GET" || method == "POST" || method == "DELETE");
}
