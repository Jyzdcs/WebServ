#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

class Location {
private:
    std::string                 _path;            // "/" ou "/uploads"
    std::vector<std::string>    _allowed_methods; // ["GET", "POST", "DELETE"]
    std::string                 _root;            // "/var/www"
    bool                        _autoindex;       // true / false
    std::string                 _index;           // "index.html"
    std::string                 _cgi_extension;   // ".php"
    std::string                 _upload_path;     // "/var/www/uploads"

public:
    Location();
    ~Location();

    const std::string&              getPath() const;
    const std::vector<std::string>& getAllowedMethods() const;
    const std::string&              getRoot() const;
    bool                            getAutoindex() const;
    const std::string&              getIndex() const;
    const std::string&              getCgiExtension() const;
    const std::string&              getUploadPath() const;

    void setPath(const std::string& path);
    void addMethod(const std::string& method);
    void setRoot(const std::string& root);
    void setAutoindex(bool value);
    void setIndex(const std::string& index);
    void setCgiExtension(const std::string& ext);
    void setUploadPath(const std::string& path);
};

#endif