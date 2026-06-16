#include "../../include/config/LocationConfig.hpp"

// _autoindex (bool) initialisé à false : un bool non initialisé a une valeur indéterminée.
LocationConfig::LocationConfig() : _autoindex(false) {}

LocationConfig::~LocationConfig() {}

// ---------- Getters (lecture seule) ----------
const std::string& LocationConfig::getPath() const { return _path; }
const std::vector<std::string>& LocationConfig::getAllowedMethods() const { return _allowed_methods; }
const std::string& LocationConfig::getRoot() const { return _root; }
bool LocationConfig::getAutoindex() const { return _autoindex; }
const std::string& LocationConfig::getIndex() const { return _index; }
const std::string& LocationConfig::getCgiExtension() const { return _cgi_extension; }
const std::string& LocationConfig::getCgiPath() const { return _cgi_path; }            // NOUVEAU : l'interpréteur
const std::string& LocationConfig::getUploadPath() const { return _upload_path; }
const std::string& LocationConfig::getRedirectUrl() const { return _redirect_url; }    // NOUVEAU : la redirection

// ---------- Setters (remplissage par le parser/mock) ----------
void LocationConfig::setPath(const std::string& path) { _path = path; }
void LocationConfig::addMethod(const std::string& method) { _allowed_methods.push_back(method); }
void LocationConfig::setRoot(const std::string& root) { _root = root; }
void LocationConfig::setAutoindex(bool value) { _autoindex = value; }
void LocationConfig::setIndex(const std::string& index) { _index = index; }
void LocationConfig::setCgiExtension(const std::string& ext) { _cgi_extension = ext; }
void LocationConfig::setCgiPath(const std::string& path) { _cgi_path = path; }         // NOUVEAU
void LocationConfig::setUploadPath(const std::string& path) { _upload_path = path; }
void LocationConfig::setRedirectUrl(const std::string& url) { _redirect_url = url; }   // NOUVEAU
