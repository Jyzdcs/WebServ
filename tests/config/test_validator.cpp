#include "../../include/config/ConfigValidator.hpp"
#include <iostream>

static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool condition)
{
    if (condition)
    {
        std::cout << "[OK] " << label << std::endl;
        passed++;
    }
    else
    {
        std::cout << "[KO] " << label << std::endl;
        failed++;
    }
}

static bool rejects(const Config& config)
{
    ConfigValidator validator;
    try
    {
        validator.validate(config);
    }
    catch (const std::exception& e)
    {
        return true;
    }
    return false;
}

static ServerConfig buildValidServer()
{
    ServerConfig server;
    server.setPort(8080);
    server.setHost("127.0.0.1");

    LocationConfig root;
    root.setPath("/");
    root.setRoot("www");
    root.addMethod("GET");
    server.addLocation(root);

    return server;
}

static Config wrap(const ServerConfig& server)
{
    Config config;
    config.addServer(server);
    return config;
}

int main()
{
    check("config minimale valide acceptée", !rejects(wrap(buildValidServer())));

    {
        Config config;
        ServerConfig a = buildValidServer();
        ServerConfig b = buildValidServer();
        b.setHost("0.0.0.0");
        config.addServer(a);
        config.addServer(b);
        check("même port, hosts différents accepté", !rejects(config));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig old;
        old.setPath("/old");
        old.setRedirectUrl("http://127.0.0.1:8080/");
        server.addLocation(old);
        check("location redirect sans root acceptée", !rejects(wrap(server)));
    }

    {
        Config empty;
        check("config sans server rejetée", rejects(empty));
    }

    {
        ServerConfig server = buildValidServer();
        server.setPort(0);
        check("listen manquant rejeté", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        server.setPort(70000);
        check("port > 65535 rejeté", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        server.setHost("");
        check("host manquant rejeté", rejects(wrap(server)));
    }

    {
        ServerConfig server;
        server.setPort(8080);
        server.setHost("127.0.0.1");
        check("server sans location rejeté", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        server.addErrorPage(999, "www/errors/404.html");
        check("error_page 999 rejetée", rejects(wrap(server)));
    }

    {
        Config config;
        config.addServer(buildValidServer());
        config.addServer(buildValidServer());
        check("host:port dupliqué sans server_name rejeté", rejects(config));
    }

    {
        Config config;
        ServerConfig a = buildValidServer();
        ServerConfig b = buildValidServer();
        a.setServerName("site.com");
        b.setServerName("site.com");
        config.addServer(a);
        config.addServer(b);
        check("host:port dupliqué avec même server_name rejeté", rejects(config));
    }

    {
        Config config;
        ServerConfig a = buildValidServer();
        ServerConfig b = buildValidServer();
        a.setServerName("site-a.com");
        b.setServerName("site-b.com");
        config.addServer(a);
        config.addServer(b);
        check("virtual hosts (même host:port, noms différents) accepté", !rejects(config));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig bad;
        bad.setPath("uploads");
        bad.setRoot("www");
        server.addLocation(bad);
        check("path sans '/' initial rejeté", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig bad;
        bad.setPath("/api");
        bad.setRoot("www");
        bad.addMethod("PATCH");
        server.addLocation(bad);
        check("méthode PATCH rejetée", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig bad;
        bad.setPath("/api");
        bad.setRoot("www");
        bad.addMethod("GET");
        bad.addMethod("GET");
        server.addLocation(bad);
        check("méthode dupliquée rejetée", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig bad;
        bad.setPath("/vide");
        bad.addMethod("GET");
        server.addLocation(bad);
        check("location sans action rejetée", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig dup;
        dup.setPath("/");
        dup.setRoot("www");
        server.addLocation(dup);
        check("location dupliquée rejetée", rejects(wrap(server)));
    }

    {
        ServerConfig server = buildValidServer();
        LocationConfig bad;
        bad.setPath("/uploads");
        bad.setRoot("www/uploads");
        bad.addMethod("GET");
        bad.setUploadPath("www/uploads");
        server.addLocation(bad);
        check("upload_store sans POST rejeté", rejects(wrap(server)));
    }

    std::cout << "\nResultat : " << passed << " OK, " << failed << " KO" << std::endl;
    return (failed == 0) ? 0 : 1;
}
