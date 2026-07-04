#include "../../include/config/ConfigParser.hpp"
#include "../../include/config/ConfigValidator.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";

    try
    {
        ConfigParser parser;
        ConfigValidator validator;

        Config config = parser.parse(path);
        validator.validate(config);
        std::cout << "OK: " << path << " (" << config.getServers().size()
                  << " server(s))" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "REJECTED: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
