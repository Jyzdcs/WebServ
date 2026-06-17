#include "../../include/config/ConfigParser.hpp"
#include <iostream>

// Banc de test du parser : parse un .conf et affiche le Config obtenu.

int main(int argc, char** argv)
{
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";

    try
    {
        ConfigParser parser;
        Config config = parser.parse(path);

        const std::vector<ServerConfig>& servers = config.getServers();
        std::cout << "Servers parsed: " << servers.size() << std::endl;

        for (size_t i = 0; i < servers.size(); ++i)
        {
            const ServerConfig& s = servers[i];
            std::cout << "\n=== server[" << i << "] ===" << std::endl;
            std::cout << "  host          : " << s.getHost() << std::endl;
            std::cout << "  port          : " << s.getPort() << std::endl;
            std::cout << "  server_name   : " << s.getServerName() << std::endl;
            std::cout << "  max_body_size : " << s.getMaxBodySize() << std::endl;

            const std::map<int, std::string>& pages = s.getErrorPages();
            for (std::map<int, std::string>::const_iterator it = pages.begin(); it != pages.end(); ++it)
                std::cout << "  error_page    : " << it->first << " -> " << it->second << std::endl;

            const std::vector<LocationConfig>& locs = s.getLocations();
            for (size_t j = 0; j < locs.size(); ++j)
            {
                const LocationConfig& l = locs[j];
                std::cout << "  --- location " << l.getPath() << " ---" << std::endl;
                std::cout << "      root       : " << l.getRoot() << std::endl;
                std::cout << "      index      : " << l.getIndex() << std::endl;
                std::cout << "      autoindex  : " << (l.getAutoindex() ? "on" : "off") << std::endl;

                const std::vector<std::string>& methods = l.getAllowedMethods();
                std::cout << "      methods    :";
                for (size_t k = 0; k < methods.size(); ++k)
                    std::cout << " " << methods[k];
                std::cout << std::endl;

                if (!l.getUploadPath().empty())
                    std::cout << "      upload     : " << l.getUploadPath() << std::endl;
                if (!l.getCgiExtension().empty())
                    std::cout << "      cgi        : " << l.getCgiExtension() << " -> " << l.getCgiPath() << std::endl;
                if (!l.getRedirectUrl().empty())
                    std::cout << "      redirect   : " << l.getRedirectUrl() << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
