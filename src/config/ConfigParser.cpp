#include "../../include/config/ConfigParser.hpp"
#include "../../include/config/Tokenizer.hpp"
#include <cstdlib>
#include <cctype>


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

    while (current().type != END_OF_FILE)
    {
        ServerConfig server = parseServer();
        config.addServer(server);
    }

    return config;
}

const Token &ConfigParser::current()
{
    return (_tokens[_pos]);
}
void    ConfigParser::advance()
{
    if (_pos + 1 < _tokens.size())
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
        if (current().type == WORD && current().value == "location")
            server.addLocation(parseLocation());
        else
            parseServerDirective(server);
    }
    expect(RBRACE);
    return server;
}

void ConfigParser::parseServerDirective(ServerConfig& server)
{
    std::string name = current().value;
    advance();

    if (name == "listen")
    {
        if (current().type != NUMBER)
            throw std::runtime_error("listen expects a number");
        server.setPort(std::atoi(current().value.c_str()));
        advance();
    }
    else if (name == "host")
    {
        server.setHost(current().value);
        advance();
    }
    else if (name == "server_name")
    {
        server.setServerName(current().value);
        advance();
    }
    else if (name == "client_max_body_size")
    {
        server.setMaxBodySize(parseSize(current().value));
        advance();
    }
    else if (name == "error_page")
    {
        int code = std::atoi(current().value.c_str());
        advance();
        server.addErrorPage(code, current().value);
        advance();
    }
    else
        throw std::runtime_error("Unknown server directive: " + name);
    expect(SEMICOLON);
}

LocationConfig ConfigParser::parseLocation()
{
    LocationConfig location;

    advance();
    location.setPath(current().value);
    advance();
    expect(LBRACE);

    while (current().type != RBRACE && current().type != END_OF_FILE)
        parseLocationDirective(location);
    expect(RBRACE);
    return location;
}

void ConfigParser::parseLocationDirective(LocationConfig &location)
{
    std::string name = current().value;
    advance();

    if (name == "root")
    {
        location.setRoot(current().value);
        advance();
    }
    else if (name == "index")
    {
        location.setIndex(current().value);
        advance();
    }
    else if (name == "autoindex")
    {
        if (current().value == "off")
            location.setAutoindex(false);
        else if (current().value == "on")
            location.setAutoindex(true);
        else
            throw std::runtime_error("Unknown value: " + name + current().value);
        advance();
    }
    else if (name == "methods")
    {
        while (current().type != SEMICOLON && current().type != END_OF_FILE)
        {
            location.addMethod(current().value);
            advance();
        }
    }
    else if (name == "upload_store")
    {
        location.setUploadPath(current().value);
        advance();
    }
    else if (name == "cgi_extensions")
    {
        location.setCgiExtension(current().value);
        advance();
        location.setCgiPath(current().value);
        advance();
    }
    else
        throw std::runtime_error("Unknown location directive: " + name);
    expect(SEMICOLON);
}

size_t ConfigParser::parseSize(const std::string& value)
{
    if (value.empty())
        throw std::runtime_error("client_max_body_size: empty value");

    size_t i = 0;
    while (i < value.size() && std::isdigit(static_cast<unsigned char>(value[i])))
        ++i;
    if (i == 0)
        throw std::runtime_error("client_max_body_size: expected a number, got '" + value + "'");

    size_t bytes = std::strtoul(value.substr(0, i).c_str(), NULL, 10);

    if (i == value.size())
        return bytes;
    if (value.size() - i != 1)
        throw std::runtime_error("client_max_body_size: invalid suffix in '" + value + "'");

    size_t multiplier;
    switch (value[i])
    {
        case 'k': case 'K': multiplier = 1024UL; break;
        case 'm': case 'M': multiplier = 1024UL * 1024UL; break;
        case 'g': case 'G': multiplier = 
        1024UL * 1024UL * 1024UL; break;
        default:
            throw std::runtime_error("client_max_body_size: unknown suffix in '" + value + "'");
    }
    if (bytes != 0 && multiplier > (static_cast<size_t>(-1) / bytes))
        throw std::runtime_error("client_max_body_size: value too large '" + value + "'");

    return bytes * multiplier;
}
