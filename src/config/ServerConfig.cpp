#include "config/ServerConfig.hpp"

// Types primitifs (int, size_t) initialisés à 0 pour éviter des valeurs indéterminées.
ServerConfig::ServerConfig() : _port(0), _max_body_size(0) {}

ServerConfig::~ServerConfig() {}

// ---------- Getters (lecture seule) ----------
int ServerConfig::getPort() const { return _port; }
const std::string& ServerConfig::getHost() const { return _host; }
const std::string& ServerConfig::getServerName() const { return _server_name; }
size_t ServerConfig::getMaxBodySize() const { return _max_body_size; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _error_pages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }

// ---------- Setters (remplissage par le parser/mock) ----------
void ServerConfig::setPort(int port) { _port = port; }
void ServerConfig::setHost(const std::string& host) { _host = host; }
void ServerConfig::setServerName(const std::string& name) { _server_name = name; }
void ServerConfig::setMaxBodySize(size_t size) { _max_body_size = size; }
void ServerConfig::addErrorPage(int code, const std::string& path) { _error_pages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location) { _locations.push_back(location); }
