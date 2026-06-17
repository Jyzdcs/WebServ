#include "../../include/config/ConfigParser.hpp"
#include "../../include/config/Tokenizer.hpp"


Config ConfigParser::parse(const std::string &path)
{
    Config config;
    Tokenizer tokenizer;

    std::ifstream file(path.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    _tokens = tokenizer.tokenize(content);
    _pos = 0;
    // std::cout << "Tokens charges : " << _tokens.size() << std::endl;

    return config;
}

const Token &ConfigParser::current()
{
    return (_tokens[_pos]);
}
void    ConfigParser::advance()
{
    _pos++;
}

void ConfigParser::expect(TokenType type)
{
    if (current().type != type)
    {
        std::ostringstream oss;
        oss << "Unexpected token at line " << current().line;
        throw std::runtime_error(oss.str());
      }
    advance();
  }

ServerConfig ConfigParser::parseServer()
{
    ServerConfig server;

    if (current().type != WORD || current().value != "server")
        throw std::runtime_error("Expected 'server' keyword");
    advance();

    expect(LBRACE);

    while (current().type != RBRACE && current().type != END_OF_FILE)
    {
        // ... ici : si "location" -> parseLocation, sinon -> directive ...
        advance();
    }
    expect(RBRACE);
    return server;
}

