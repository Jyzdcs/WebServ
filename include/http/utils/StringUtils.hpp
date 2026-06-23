#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>

std::string urlDecode(const std::string& encoded);
bool        hasPathTraversal(const std::string& uri);
std::string extractUriPath(const std::string& uri);
std::string extractQueryString(const std::string& uri);
std::string extractFilename(const std::string& uri);

#endif
