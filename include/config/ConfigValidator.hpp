#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include <string>
#include <vector>
#include "Config.hpp"

class ConfigValidator {
private:
    void validateServer(const ServerConfig& server, size_t index);
    void validateLocation(const LocationConfig& location, size_t serverIndex);
    void checkDuplicateHostPort(const std::vector<ServerConfig>& servers);
    void checkDuplicateLocationPath(const ServerConfig& server, size_t index);
    bool isAllowedMethod(const std::string& method);
    std::string where(size_t serverIndex);

public:
    void validate(const Config& config);
};

#endif
