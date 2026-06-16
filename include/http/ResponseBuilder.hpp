#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include "HttpResponse.hpp"
#include <string>

class ResponseBuilder {
public:
    std::string build(const HttpResponse& res);
};

#endif
