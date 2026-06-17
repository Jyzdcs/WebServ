#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "Token.hpp"
#include "Config.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class ConfigParser {
private:
    std::vector<Token>  _tokens;
    size_t              _pos;
    const Token         &current();
    void                advance();
    void                expect(TokenType type);
    ServerConfig        parseServer();
    LocationConfig      parseLocaton();
     

public:
    Config parse(const std::string& path);
};

#endif
