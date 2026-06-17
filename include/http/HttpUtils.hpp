#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP

#include "HttpResponse.hpp"
#include <string>

HttpResponse buildHttpError(int statusCode, const std::string& statusMessage);

#endif
