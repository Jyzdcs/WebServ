#ifndef HTTP_UTILS_HPP
#define HTTP_UTILS_HPP

#include <string>

bool        writeFdFromString(int fd, const std::string& data);
bool        readFdToString(int fd, std::string& body);
std::string getContentType(const std::string& filePath);

#endif
