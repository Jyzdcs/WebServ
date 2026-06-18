#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>
#include <cctype>
#include "Token.hpp"

class Tokenizer {
private:
    Token makeSymbol(TokenType type, const std::string &value, int line);
    Token readWordOrNumber(const std::string &content, int line, size_t &i);

public:
    std::vector<Token> tokenize(const std::string& content);
};

#endif
