#include "../../include/config/ConfigMock.hpp"

// Reproduit en dur le contenu d'un fichier .conf (aucun fichier lu, aucun parsing).
// À terme, le parser produira ce Config à partir d'un fichier ; ici la
// configuration est construite via des appels successifs aux setters.

Config createMockConfig()
{
    Config config;
    ServerConfig server;

    // --- niveau server : où j'écoute + mes limites globales ---
    server.setPort(8080);
    server.setHost("127.0.0.1");
    server.setServerName("webserv.com");
    server.setMaxBodySize(1000000);                 // "1m" = 1 000 000 octets
    server.addErrorPage(404, "/errors/404.html");

    // --- location "/" : la route par défaut (filet de sécurité) ---
    LocationConfig root;
    root.setPath("/");
    root.setRoot("/var/www");
    root.setIndex("index.html");
    root.addMethod("GET");
    root.addMethod("POST");
    root.setAutoindex(false);
    server.addLocation(root);

    // --- location "/uploads" : route de réception de fichiers ---
    LocationConfig uploads;
    uploads.setPath("/uploads");
    uploads.setRoot("/var/www/uploads");
    uploads.addMethod("POST");
    uploads.addMethod("DELETE");
    uploads.setUploadPath("/var/www/uploads");
    server.addLocation(uploads);

    // --- location "/cgi-bin" : route qui EXÉCUTE au lieu de servir ---
    LocationConfig cgi;
    cgi.setPath("/cgi-bin");
    cgi.setCgiExtension(".php");
    cgi.setCgiPath("/usr/bin/php-cgi");             // interpréteur CGI associé à l'extension
    server.addLocation(cgi);

    // --- location "/old" : exemple de REDIRECTION (nouveau champ) ---
    LocationConfig redirect;
    redirect.setPath("/old");
    redirect.setRedirectUrl("https://webserv.com/new");   // URL cible de la redirection
    server.addLocation(redirect);

    config.addServer(server);
    return config;
}
