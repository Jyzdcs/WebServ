#include "../../../include/http/MethodHandler.hpp"
#include <iostream>
#include <fstream>

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

static ServerConfig buildServer(const std::string& root)
{
    ServerConfig server;

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot(root);
    loc.setIndex("index.html");
    loc.addMethod("GET");
    server.addLocation(loc);

    LocationConfig autoLoc;
    autoLoc.setPath("/auto");
    autoLoc.setRoot(root);
    autoLoc.setAutoindex(true);
    autoLoc.addMethod("GET");
    server.addLocation(autoLoc);

    LocationConfig noMethod;
    noMethod.setPath("/readonly");
    noMethod.setRoot(root);
    noMethod.addMethod("POST");
    server.addLocation(noMethod);

    LocationConfig redirect;
    redirect.setPath("/old");
    redirect.setRedirectUrl("https://new-site.com");
    redirect.addMethod("GET");
    server.addLocation(redirect);

    return server;
}

static HttpRequest makeGet(const std::string& uri)
{
    HttpRequest req;
    req.method  = "GET";
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost";
    return req;
}

static void test_get_existing_file()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/index.html"), loc, server);
    check("GET existing file: 200",          res.status_code == 200);
    check("GET existing file: body non vide", !res.body.empty());
    check("GET existing file: Content-Type text/html", res.headers["Content-Type"] == "text/html");
    check("GET existing file: Content-Length non nul", res.headers["Content-Length"] != "0");
}

static void test_get_not_found()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/nonexistent.html"), loc, server);
    check("GET 404: status 404", res.status_code == 404);
}

static void test_get_method_not_allowed()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/readonly");
    loc.setRoot("www");
    loc.addMethod("POST");

    HttpResponse res = handler.handle(makeGet("/readonly/file.html"), loc, server);
    check("GET method not allowed: 405", res.status_code == 405);
}

static void test_get_redirect()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/old");
    loc.setRedirectUrl("https://new-site.com");
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/old"), loc, server);
    check("GET redirect: 301",             res.status_code == 301);
    check("GET redirect: Location header", res.headers["Location"] == "https://new-site.com");
}

static void test_get_directory_with_index()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.setIndex("index.html");
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/"), loc, server);
    check("GET dir with index: 200",          res.status_code == 200);
    check("GET dir with index: body non vide", !res.body.empty());
}

static void test_get_directory_no_index_no_autoindex()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.setAutoindex(false);
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/"), loc, server);
    check("GET dir no index no autoindex: 403", res.status_code == 403);
}

static void test_get_directory_autoindex()
{
    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.setAutoindex(true);
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/"), loc, server);
    check("GET dir autoindex: 200",             res.status_code == 200);
    check("GET dir autoindex: body contient ul", res.body.find("<ul>") != std::string::npos);
}

static void test_get_css_content_type()
{
    std::ofstream f("www/style.css");
    f << "body { color: red; }";
    f.close();

    MethodHandler handler;
    ServerConfig server = buildServer("www");

    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot("www");
    loc.addMethod("GET");

    HttpResponse res = handler.handle(makeGet("/style.css"), loc, server);
    check("GET css: 200",              res.status_code == 200);
    check("GET css: Content-Type css", res.headers["Content-Type"] == "text/css");

    std::remove("www/style.css");
}

int main()
{
    test_get_existing_file();
    test_get_not_found();
    test_get_method_not_allowed();
    test_get_redirect();
    test_get_directory_with_index();
    test_get_directory_no_index_no_autoindex();
    test_get_directory_autoindex();
    test_get_css_content_type();

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0 ? 0 : 1;
}
