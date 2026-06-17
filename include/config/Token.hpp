#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum TokenType {
    WORD,
    NUMBER,
    LBRACE,
    RBRACE,
    SEMICOLON,
    END_OF_FILE
};

struct Token {
    TokenType    type;
    std::string  value;
    int          line;
};

#endif
