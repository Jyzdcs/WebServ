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

// Consomme la valeur d'une directive (un WORD ou un NUMBER).
// Refuse les symboles : sans ce garde-fou, "root ;" stockerait ";"
// comme valeur au lieu de signaler la valeur manquante.
std::string ConfigParser::takeValue(const std::string& directive)
{
    if (current().type != WORD && current().type != NUMBER)
    {
        std::ostringstream oss;
        oss << directive << ": missing value at line " << current().line;
        throw std::runtime_error(oss.str());
    }
    std::string value = current().value;
    advance();
    return value;
}

// Consomme une valeur strictement numérique.
// Le tokenizer marque NUMBER dès que le 1er caractère est un chiffre,
// donc "8080abc" est un NUMBER : on re-vérifie chaque caractère ici,
// sinon atoi("8080abc") donnerait 8080 en silence.
int ConfigParser::takeNumber(const std::string& directive)
{
    if (current().type != NUMBER)
    {
        std::ostringstream oss;
        oss << directive << ": expected a number at line " << current().line;
        throw std::runtime_error(oss.str());
    }
    const std::string& value = current().value;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
        {
            std::ostringstream oss;
            oss << directive << ": '" << value << "' is not a valid number at line "
                << current().line;
            throw std::runtime_error(oss.str());
        }
    }
    // atoi déborde en silence au-delà de INT_MAX : "4294975376" (= 2^32 + 8080)
    // redonnerait 8080 et passerait la validation ! 9 chiffres max suffisent
    // largement pour un port ou un code HTTP, et tiennent toujours dans un int.
    if (value.size() > 9)
    {
        std::ostringstream oss;
        oss << directive << ": number '" << value << "' is too large at line "
            << current().line;
        throw std::runtime_error(oss.str());
    }
    advance();
    return std::atoi(value.c_str());
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
        server.setPort(takeNumber("listen"));
    else if (name == "host")
        server.setHost(takeValue("host"));
    else if (name == "server_name")
        server.setServerName(takeValue("server_name"));
    else if (name == "client_max_body_size")
        server.setMaxBodySize(parseSize(takeValue("client_max_body_size")));
    else if (name == "error_page")
    {
        int code = takeNumber("error_page");
        server.addErrorPage(code, takeValue("error_page"));
    }
    else
        throw std::runtime_error("Unknown server directive: " + name);
    expect(SEMICOLON);
}

LocationConfig ConfigParser::parseLocation()
{
    LocationConfig location;

    advance();
    location.setPath(takeValue("location"));
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
        location.setRoot(takeValue("root"));
    else if (name == "index")
        location.setIndex(takeValue("index"));
    else if (name == "autoindex")
    {
        std::string value = takeValue("autoindex");
        if (value == "off")
            location.setAutoindex(false);
        else if (value == "on")
            location.setAutoindex(true);
        else
            throw std::runtime_error("autoindex: expected 'on' or 'off', got '" + value + "'");
    }
    else if (name == "methods")
    {
        // Au moins une méthode : takeValue refuse un ';' immédiat,
        // puis on boucle tant qu'il reste des valeurs avant le ';'.
        location.addMethod(takeValue("methods"));
        while (current().type != SEMICOLON && current().type != END_OF_FILE)
            location.addMethod(takeValue("methods"));
    }
    else if (name == "upload_store")
        location.setUploadPath(takeValue("upload_store"));
    else if (name == "cgi_extensions")
    {
        location.setCgiExtension(takeValue("cgi_extensions"));
        location.setCgiPath(takeValue("cgi_extensions"));
    }
    else if (name == "redirect")
        location.setRedirectUrl(takeValue("redirect"));
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
