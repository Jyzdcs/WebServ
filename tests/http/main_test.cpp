#include "../../include/http/RequestParser.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/config/ConfigMock.hpp"
#include <iostream>

static void printRequest(const HttpRequest& req)
{
    std::cout << "=== REQUEST ===" << std::endl;
    std::cout << "method:  " << req.method  << std::endl;
    std::cout << "uri:     " << req.uri     << std::endl;
    std::cout << "version: " << req.version << std::endl;
    std::cout << "body:    " << req.body    << std::endl;

    std::map<std::string, std::string>::const_iterator it;
    for (it = req.headers.begin(); it != req.headers.end(); ++it)
        std::cout << "  [" << it->first << "] = " << it->second << std::endl;
}

static void printConfig(const Config& config)
{
    std::cout << "\n=== CONFIG ===" << std::endl;

    const std::vector<ServerConfig>& servers = config.getServers();
    for (std::vector<ServerConfig>::const_iterator s = servers.begin(); s != servers.end(); ++s)
    {
        std::cout << "server " << s->getHost() << ":" << s->getPort()
                  << " (" << s->getServerName() << ")" << std::endl;

        const std::vector<LocationConfig>& locs = s->getLocations();
        for (std::vector<LocationConfig>::const_iterator l = locs.begin(); l != locs.end(); ++l)
        {
            std::cout << "  location " << l->getPath() << std::endl;
            std::cout << "    root:     " << l->getRoot() << std::endl;
            std::cout << "    index:    " << l->getIndex() << std::endl;
            std::cout << "    redirect: " << l->getRedirectUrl() << std::endl;
            std::cout << "    cgi_ext:  " << l->getCgiExtension() << std::endl;

            const std::vector<std::string>& methods = l->getAllowedMethods();
            std::cout << "    methods:  ";
            for (std::vector<std::string>::const_iterator m = methods.begin(); m != methods.end(); ++m)
                std::cout << *m << " ";
            std::cout << std::endl;
        }
    }
}

int main()
{
    // --- parse une requête ---
    RequestParser parser;
    std::string raw =
        "POST /uploads/photo.jpg HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 4\r\n"
        "\r\n"
        "data";

    HttpRequest req = parser.parse(raw);
    std::cout << "=== RAW REQUEST ===" << std::endl << raw << std::endl;
    if (req.method.empty())
    {
        std::cout << "Invalid request" << std::endl;
        return 1;
    }
    printRequest(req);

    // --- charge la config mock ---
    Config config = createMockConfig();
    printConfig(config);

    // --- ce que le Router devra faire : trouver quelle location matche req.uri ---
    std::cout << "\n=== ROUTER ===" << std::endl;
    Router router;
    LocationConfig matched = router.route(req, config.getServers()[0]);
    std::cout << "URI             : " << req.uri << std::endl;
    std::cout << "Location matchee: " << matched.getPath() << std::endl;
    std::cout << "Root            : " << matched.getRoot() << std::endl;

    return 0;
}
