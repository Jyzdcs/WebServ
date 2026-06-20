#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/config/ServerConfig.hpp"
#include "../../../include/config/LocationConfig.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

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

static HttpRequest makeReq(const std::string& method, const std::string& uri)
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    return req;
}

// location SANS root — simule /cgi-bin ou /old
static LocationConfig makeLocationNoRoot()
{
    LocationConfig loc;
    loc.setPath("/cgi-bin");
    // pas de root, pas de méthodes → tout appel → 405
    return loc;
}

// location AVEC root — cas normal
static LocationConfig makeLocationWithRoot()
{
    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("/tmp/www");   // root = /tmp/www
    loc.addMethod("GET");
    return loc;
}

static ServerConfig makeServerWithCustomErrorPage()
{
    ServerConfig server;
    server.setPort(8080);
    server.setHost("127.0.0.1");
    // filePath = root + path = "/tmp/www" + "/errors/404.html" = "/tmp/www/errors/404.html"
    server.addErrorPage(405, "/errors/404.html");
    return server;
}

// TEST 1 : location sans root → avec le guard actuel, la custom error page est ignorée
static void test_no_root_location_skips_error_page()
{
    MethodHandler  handler;
    ServerConfig   server   = makeServerWithCustomErrorPage();
    LocationConfig location = makeLocationNoRoot();

    HttpResponse response = handler.handle(makeReq("GET", "/cgi-bin/script"), location, server);

    std::cout << "\n[TEST] location sans root → status=" << response.status_code
              << " body='" << response.body.substr(0, 30) << "'" << std::endl;

    check("status 405", response.status_code == 405);
    // avec le guard location.getRoot().empty() → body = default error body
    // sans le guard → body = contenu de /tmp/test_custom_404.html
    bool usedCustomPage = response.body.find("Custom 404") != std::string::npos;
    std::cout << "      custom error page utilisée : " << (usedCustomPage ? "OUI" : "NON") << std::endl;
}

// TEST 2 : location avec root → la custom error page doit être utilisée
static void test_with_root_location_uses_error_page()
{
    MethodHandler  handler;
    ServerConfig   server   = makeServerWithCustomErrorPage();
    LocationConfig location = makeLocationWithRoot();

    // on force une 405 en envoyant DELETE sur une location qui n'autorise que GET
    HttpResponse response = handler.handle(makeReq("DELETE", "/"), location, server);

    std::cout << "\n[TEST] location avec root → status=" << response.status_code
              << " body='" << response.body.substr(0, 30) << "'" << std::endl;

    check("status 405", response.status_code == 405);
    bool usedCustomPage = response.body.find("Custom 404") != std::string::npos;
    check("custom error page utilisée", usedCustomPage);
}

int main()
{
    // crée /tmp/www/errors/404.html avec le contenu attendu par les tests
    ::mkdir("/tmp/www", 0755);
    ::mkdir("/tmp/www/errors", 0755);
    std::ofstream f("/tmp/www/errors/404.html");
    f << "<html><body>Custom 404</body></html>\n";
    f.close();

    test_no_root_location_skips_error_page();
    test_with_root_location_uses_error_page();

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0 ? 0 : 1;
}
