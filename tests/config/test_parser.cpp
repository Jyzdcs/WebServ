#include "../../include/config/ConfigParser.hpp"
#include <iostream>

// Banc de test du parser.
// Compiler :
//   c++ -Wall -Wextra -Werror -std=c++98 \
//       src/config/ConfigParser.cpp src/config/Tokenizer.cpp \
//       src/config/Config.cpp src/config/ServerConfig.cpp src/config/LocationConfig.cpp \
//       tests/config/test_parser.cpp -o test_parser
// Lancer :
//   ./test_parser config/default.conf

int main(int argc, char** argv)
{
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";

    try
    {
        ConfigParser parser;
        parser.parse(path);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
