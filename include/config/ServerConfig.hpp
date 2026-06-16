#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig {
private:
    int                             _port;          // 8080
    std::string                     _host;          // "127.0.0.1" / "localhost"
    std::string                     _server_name;   // "monsite.com"
    size_t                          _max_body_size; // 1000000
    std::map<int, std::string>      _error_pages;   // {404: "/404.html"}
    std::vector<LocationConfig>     _locations;     // tous les blocs location {}

public:
    ServerConfig();
    ~ServerConfig();

    int                                     getPort() const;
    const std::string&                      getHost() const;
    const std::string&                      getServerName() const;
    size_t                                  getMaxBodySize() const;
    const std::map<int, std::string>&       getErrorPages() const;
    const std::vector<LocationConfig>&      getLocations() const;

    void setPort(int port);
    void setHost(const std::string& host);
    void setServerName(const std::string& name);
    void setMaxBodySize(size_t size);
    void addErrorPage(int code, const std::string& path);
    void addLocation(const LocationConfig& location);
};

#endif
