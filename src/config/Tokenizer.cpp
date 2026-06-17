#include "../../include/config/Tokenizer.hpp"

std::vector<Token> Tokenizer::tokenize(const std::string &content)
{
    std::vector<Token> tokenList;
    int line = 1;
    size_t i = 0;

    while (content[i])
    {
        if (content[i] == '\r' || content[i] == ' ' || content[i] == '\t')
            i++;
        else if (content[i] == '\n')
        {
            line++;
            i++;
        }
        else if (content[i] == '{')
        {
            tokenList.push_back(makeSymbol(LBRACE, "{", line));
            i++;
        }
        else if (content[i] == '}')
        {
             tokenList.push_back(makeSymbol(RBRACE, "}", line));
            i++;
        }
        else if (content[i] == ';')
        {
            tokenList.push_back(makeSymbol(SEMICOLON, ";", line));
            i++;
        }
        else if (content[i] == '#')
        {
            while (content[i] && content[i] != '\n')
                 i++;
        }
        else
            tokenList.push_back(readWordOrNumber(content, line, i));
    }
    tokenList.push_back(makeSymbol(END_OF_FILE, "", line));
    return tokenList;
}

Token Tokenizer::makeSymbol(TokenType type, const std::string &value, int line)
{
    Token t;
    t.type = type;
    t.value = value;
    t.line = line;
    return t;
}

Token Tokenizer::readWordOrNumber(const std::string &content, int line, size_t &i)
{
    Token t;
    t.line = line;
    if (isdigit(content[i]))
        t.type = NUMBER;
    else
        t.type = WORD;
    while (content[i] && content[i] != ' ' && content[i] != '\t' && content[i] != '\r'
            && content[i] != '\n' && content[i] != '{' && content[i] != '}'
            && content[i] != ';' && content[i] != '#')
    {
        t.value += content[i];
        i++;
    }
    return t;
}