#include "../../include/config/Tokenizer.hpp"
#include <iostream>

static const char* typeName(TokenType t)
{
    switch (t)
    {
        case WORD:        return "WORD";
        case NUMBER:      return "NUMBER";
        case LBRACE:      return "LBRACE";
        case RBRACE:      return "RBRACE";
        case SEMICOLON:   return "SEMICOLON";
        case END_OF_FILE: return "EOF";
    }
    return "?";
}

int main()
{
    std::string sample =
        "server {\n"
        "    listen 8080;\n"
        "    # ceci est un commentaire a ignorer\n"
        "    server_name webserv.com;\n"
        "    location / {\n"
        "        methods GET POST;\n"
        "    }\n"
        "}\n";

    Tokenizer tk;
    std::vector<Token> tokens = tk.tokenize(sample);

    std::cout << "Nombre de tokens : " << tokens.size() << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i)
        std::cout << "[L" << tokens[i].line << "] "
                  << typeName(tokens[i].type) << " '" << tokens[i].value << "'"
                  << std::endl;
    return 0;
}
