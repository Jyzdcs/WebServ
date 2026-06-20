#ifndef HTTP_BUILDERS_HPP
#define HTTP_BUILDERS_HPP

#include "../HttpResponse.hpp"
#include <string>

HttpResponse buildHttpError(int statusCode, const std::string& statusMessage);
HttpResponse buildHttpOk(const std::string& body, const std::string& contentType);
HttpResponse buildHttpCreated(const std::string& body);
HttpResponse buildHttpNoContent();
HttpResponse buildRedirect(const std::string& url);

#endif
