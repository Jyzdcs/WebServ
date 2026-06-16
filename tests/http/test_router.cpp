#include "../../include/http/Router.hpp"
#include "../../include/config/ServerConfig.hpp"
#include "../../include/config/LocationConfig.hpp"
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

static ServerConfig buildServer()
{
    ServerConfig server;
    server.setPort(8080);
    server.setHost("127.0.0.1");

    LocationConfig root;
    root.setPath("/");
    root.setRoot("/var/www");
    root.addMethod("GET");
    root.addMethod("POST");
    server.addLocation(root);

    LocationConfig uploads;
    uploads.setPath("/uploads");
    uploads.setRoot("/var/www/uploads");
    uploads.addMethod("POST");
    uploads.addMethod("DELETE");
    server.addLocation(uploads);

    LocationConfig cgi;
    cgi.setPath("/cgi-bin");
    cgi.setCgiExtension(".php");
    cgi.setCgiPath("/usr/bin/php-cgi");
    server.addLocation(cgi);

    LocationConfig redirect;
    redirect.setPath("/old");
    redirect.setRedirectUrl("https://webserv.com/new");
    server.addLocation(redirect);

    LocationConfig deep;
    deep.setPath("/uploads/images");
    deep.setRoot("/var/www/uploads/images");
    deep.addMethod("GET");
    server.addLocation(deep);

    return server;
}

static HttpRequest makeReq(const std::string& uri)
{
    HttpRequest req;
    req.method  = "GET";
    req.uri     = uri;
    req.version = "HTTP/1.1";
    return req;
}

static void test_root_match()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/index.html"), server);
    check("root match /index.html → /", loc.getPath() == "/");
}

static void test_exact_match()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/uploads"), server);
    check("exact match /uploads → /uploads", loc.getPath() == "/uploads");
}

static void test_prefix_match()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/uploads/photo.jpg"), server);
    check("prefix match /uploads/photo.jpg → /uploads", loc.getPath() == "/uploads");
}

static void test_longest_prefix_wins()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/uploads/images/2024/foo.png"), server);
    check("longest prefix /uploads/images/... → /uploads/images", loc.getPath() == "/uploads/images");
}

static void test_no_partial_prefix()
{
    Router router;
    ServerConfig server = buildServer();

    // /uploadsfoo ne doit PAS matcher /uploads
    LocationConfig loc = router.route(makeReq("/uploadsfoo"), server);
    check("no partial prefix /uploadsfoo → /", loc.getPath() == "/");
}

static void test_cgi_match()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/cgi-bin/script.php"), server);
    check("cgi match /cgi-bin/script.php → /cgi-bin", loc.getPath() == "/cgi-bin");
}

static void test_redirect_match()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/old"), server);
    check("redirect match /old → /old", loc.getPath() == "/old");
    check("redirect url correct", loc.getRedirectUrl() == "https://webserv.com/new");
}

static void test_root_uri()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/"), server);
    check("URI / → /", loc.getPath() == "/");
}

static void test_no_match_returns_empty()
{
    Router router;
    ServerConfig server;

    LocationConfig uploads;
    uploads.setPath("/uploads");
    server.addLocation(uploads);

    LocationConfig loc = router.route(makeReq("/about"), server);
    check("no match → empty path", loc.getPath() == "");
}

static void test_uploads_root_correct()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/uploads/video.mp4"), server);
    check("uploads root /var/www/uploads", loc.getRoot() == "/var/www/uploads");
}

static void test_uploads_methods()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/uploads/file.txt"), server);
    const std::vector<std::string>& methods = loc.getAllowedMethods();
    check("uploads methods size = 2", methods.size() == 2);
    check("uploads method POST",   methods[0] == "POST");
    check("uploads method DELETE", methods[1] == "DELETE");
}

static void test_cgi_extension()
{
    Router router;
    ServerConfig server = buildServer();

    LocationConfig loc = router.route(makeReq("/cgi-bin/test.php"), server);
    check("cgi extension .php", loc.getCgiExtension() == ".php");
    check("cgi path correct",   loc.getCgiPath() == "/usr/bin/php-cgi");
}

static void test_deep_path_over_shallow()
{
    Router router;
    ServerConfig server = buildServer();

    // /uploads/images doit gagner sur /uploads pour /uploads/images/foo
    LocationConfig loc = router.route(makeReq("/uploads/images/foo.jpg"), server);
    check("deep path /uploads/images wins over /uploads", loc.getPath() == "/uploads/images");
    check("deep path root correct", loc.getRoot() == "/var/www/uploads/images");
}

int main()
{
    test_root_match();
    test_exact_match();
    test_prefix_match();
    test_longest_prefix_wins();
    test_no_partial_prefix();
    test_cgi_match();
    test_redirect_match();
    test_root_uri();
    test_no_match_returns_empty();
    test_uploads_root_correct();
    test_uploads_methods();
    test_cgi_extension();
    test_deep_path_over_shallow();

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0 ? 0 : 1;
}
