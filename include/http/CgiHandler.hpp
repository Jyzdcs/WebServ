#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../../include/config/LocationConfig.hpp"
#include <string>
#include <vector>

class CgiHandler {
public:
    HttpResponse execute(const HttpRequest& req, const LocationConfig& loc);

private:
    std::vector<std::string> buildEnv(const HttpRequest& req, const std::string& scriptPath);
    HttpResponse             parseOutput(const std::string& raw);
};

#endif
