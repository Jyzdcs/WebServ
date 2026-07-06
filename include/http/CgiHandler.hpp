#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../../include/config/LocationConfig.hpp"
#include "../../include/http/processHttp.hpp"
#include <string>
#include <vector>

class CgiHandler {
public:
    ProcessResult start(const HttpRequest& req, const LocationConfig& loc);
    HttpResponse  parseOutput(const std::string& raw);

private:
    std::vector<std::string> buildEnv(const HttpRequest& req, const std::string& scriptPath);
};

#endif
