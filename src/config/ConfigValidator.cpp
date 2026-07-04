#include "../../include/config/ConfigValidator.hpp"
#include <sstream>
#include <stdexcept>

void ConfigValidator::validate(const Config& config)
{
    const std::vector<ServerConfig>& servers = config.getServers();

    if (servers.empty())
        throw std::runtime_error("config: no server block found");

    for (size_t i = 0; i < servers.size(); ++i)
        validateServer(servers[i], i);

    checkDuplicateHostPort(servers);
}

std::string ConfigValidator::where(size_t serverIndex)
{
    std::ostringstream oss;
    oss << "server[" << serverIndex << "]: ";
    return oss.str();
}

void ConfigValidator::validateServer(const ServerConfig& server, size_t index)
{
    if (server.getPort() == 0)
        throw std::runtime_error(where(index) + "missing 'listen' directive");
    if (server.getPort() < 1 || server.getPort() > 65535)
    {
        std::ostringstream oss;
        oss << where(index) << "invalid port " << server.getPort()
            << " (must be between 1 and 65535)";
        throw std::runtime_error(oss.str());
    }

    if (server.getHost().empty())
        throw std::runtime_error(where(index) + "missing 'host' directive");

    if (server.getLocations().empty())
        throw std::runtime_error(where(index) + "must have at least one location block");

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

    if (path.empty() || path[0] != '/')
        throw std::runtime_error(where(serverIndex)
            + "location path '" + path + "' must start with '/'");

    const std::vector<std::string>& methods = location.getAllowedMethods();
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (!isAllowedMethod(methods[i]))
            throw std::runtime_error(where(serverIndex) + "location '" + path
                + "': unknown method '" + methods[i] + "' (allowed: GET POST DELETE)");
        for (size_t j = i + 1; j < methods.size(); ++j)
        {
            if (methods[i] == methods[j])
                throw std::runtime_error(where(serverIndex) + "location '" + path
                    + "': duplicate method '" + methods[i] + "'");
        }
    }

    bool hasRoot = !location.getRoot().empty();
    bool hasRedirect = !location.getRedirectUrl().empty();
    bool hasCgi = !location.getCgiExtension().empty();
    if (!hasRoot && !hasRedirect && !hasCgi)
        throw std::runtime_error(where(serverIndex) + "location '" + path
            + "' has no 'root', 'redirect' or 'cgi_extensions' (nothing to do)");

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
                continue;

            std::ostringstream oss;
            oss << "server[" << j << "] duplicates host:port "
                << servers[i].getHost() << ":" << servers[i].getPort()
                << " of server[" << i << "]"
                << " (add distinct server_name to both for virtual hosting)";
            throw std::runtime_error(oss.str());
        }
    }
}

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

bool ConfigValidator::isAllowedMethod(const std::string& method)
{
    return (method == "GET" || method == "POST" || method == "DELETE");
}
