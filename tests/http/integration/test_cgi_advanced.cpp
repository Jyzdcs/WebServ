#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/config/LocationConfig.hpp"
#include "../../../include/config/ServerConfig.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

// ── Counters ──────────────────────────────────────────────────────────────────
static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool cond)
{
    if (cond) { std::cout << "[OK] " << label << "\n"; ++passed; }
    else       { std::cout << "[KO] " << label << "\n"; ++failed; }
}

// ── Helpers ───────────────────────────────────────────────────────────────────
static LocationConfig makeCgiLoc(const std::string& ext = ".py",
                                  const std::string& interp = "/usr/bin/python3",
                                  const std::string& root = "www")
{
    LocationConfig loc;
    loc.setPath("/cgi-bin");
    loc.setRoot(root);
    loc.setCgiExtension(ext);
    loc.setCgiPath(interp);
    loc.addMethod("GET");
    loc.addMethod("POST");
    loc.addMethod("DELETE");
    return loc;
}

static HttpRequest makeGet(const std::string& uri, const std::string& host = "localhost")
{
    HttpRequest req;
    req.method  = "GET";
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["Host"] = host;
    return req;
}

static HttpRequest makePost(const std::string& uri,
                             const std::string& body,
                             const std::string& ctype = "application/x-www-form-urlencoded")
{
    HttpRequest req;
    req.method  = "POST";
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["Host"]         = "localhost";
    req.headers["Content-Type"] = ctype;
    req.body = body;
    return req;
}

// ── Section helpers ───────────────────────────────────────────────────────────
static void section(const std::string& title)
{
    std::cout << "\n── " << title << " ";
    for (int i = (int)title.size(); i < 50; i++) std::cout << '-';
    std::cout << "\n";
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main()
{
    CgiHandler    cgi;
    MethodHandler handler;
    ServerConfig  server;
    LocationConfig loc = makeCgiLoc();
    server.addLocation(loc);

    // ══════════════════════════════════════════════════════════════════════════
    section("ENV VARS TRANSMISSION");

    // REQUEST_METHOD GET
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/env_dump.py"), loc);
        check("env: status 200", res.status_code == 200);
        check("env: REQUEST_METHOD=GET", res.body.find("REQUEST_METHOD=GET") != std::string::npos);
        check("env: GATEWAY_INTERFACE=CGI/1.1", res.body.find("GATEWAY_INTERFACE=CGI/1.1") != std::string::npos);
        check("env: SERVER_PROTOCOL=HTTP/1.1", res.body.find("SERVER_PROTOCOL=HTTP/1.1") != std::string::npos);
        check("env: SCRIPT_FILENAME non vide", res.body.find("SCRIPT_FILENAME=__MISSING__") == std::string::npos);
    }

    // REQUEST_METHOD POST
    {
        HttpResponse res = cgi.execute(makePost("/cgi-bin/env_dump.py", "x=1"), loc);
        check("env POST: REQUEST_METHOD=POST", res.body.find("REQUEST_METHOD=POST") != std::string::npos);
        check("env POST: CONTENT_LENGTH=3", res.body.find("CONTENT_LENGTH=3") != std::string::npos);
        check("env POST: CONTENT_TYPE present", res.body.find("CONTENT_TYPE=application/x-www-form-urlencoded") != std::string::npos);
    }

    // QUERY_STRING
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/env_dump.py?name=Byron&lang=cpp"), loc);
        check("env: QUERY_STRING=name=Byron&lang=cpp",
              res.body.find("QUERY_STRING=name=Byron&lang=cpp") != std::string::npos);
    }

    // HTTP_HOST
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/env_dump.py", "myserver:8080"), loc);
        check("env: HTTP_HOST=myserver:8080",
              res.body.find("HTTP_HOST=myserver:8080") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("POST BODY TRANSMISSION");

    // echo exact body
    {
        std::string body = "hello_webserv_42";
        HttpResponse res = cgi.execute(makePost("/cgi-bin/echo_post.py", body), loc);
        check("POST echo: status 200", res.status_code == 200);
        check("POST echo: body identique", res.body == body);
    }

    // body avec caracteres speciaux
    {
        std::string body = "data=hello%20world&value=42&special=!@#$";
        HttpResponse res = cgi.execute(makePost("/cgi-bin/echo_post.py", body), loc);
        check("POST echo: special chars transmis", res.body == body);
    }

    // body vide
    {
        HttpResponse res = cgi.execute(makePost("/cgi-bin/echo_post.py", ""), loc);
        check("POST echo: body vide → 200", res.status_code == 200);
        check("POST echo: body vide → response vide", res.body.empty());
    }

    // body large (10KB)
    {
        std::string body(10 * 1024, 'Z');
        HttpResponse res = cgi.execute(makePost("/cgi-bin/echo_post.py", body), loc);
        check("POST echo: 10KB body transmis", res.body.size() == body.size());
        check("POST echo: contenu correct", res.body == body);
    }

    // POST form parsing
    {
        HttpResponse res = cgi.execute(
            makePost("/cgi-bin/post_form.py", "username=alice&age=30&city=Paris"), loc);
        check("POST form: username=alice", res.body.find("username=alice") != std::string::npos);
        check("POST form: age=30",         res.body.find("age=30") != std::string::npos);
        check("POST form: city=Paris",     res.body.find("city=Paris") != std::string::npos);
        check("POST form: CONTENT_TYPE present",
              res.body.find("CONTENT_TYPE=application/x-www-form-urlencoded") != std::string::npos);
    }

    // POST avec Content-Type: application/json
    {
        std::string json = "{\"key\":\"value\",\"num\":42}";
        HttpResponse res = cgi.execute(makePost("/cgi-bin/echo_post.py", json, "application/json"), loc);
        check("POST json: body correct", res.body == json);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("QUERY STRING PARSING");

    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/hello.py?name=Byron"), loc);
        check("query: name dans body", res.body.find("Byron") != std::string::npos);
    }

    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/json_api.py?name=Alice%20Smith"), loc);
        check("query: URL-decoded name", res.body.find("Alice Smith") != std::string::npos);
        check("query: content-type JSON",
              res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
    }

    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/env_dump.py?"), loc);
        check("query vide: QUERY_STRING=", res.body.find("QUERY_STRING=\n") != std::string::npos ||
                                           res.body.find("QUERY_STRING=\r\n") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("CUSTOM STATUS CODES FROM CGI");

    // 302 redirect
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/redirect.py?to=https://42.fr"), loc);
        check("CGI 302: status 302", res.status_code == 302);
        check("CGI 302: Location header present", res.headers.count("Location") > 0);
        check("CGI 302: Location correct",
              res.headers.count("Location") > 0 &&
              res.headers.at("Location") == "https://42.fr");
    }

    // 404 from CGI
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/custom_404.py"), loc);
        check("CGI custom 404: status 404", res.status_code == 404);
        check("CGI custom 404: body present", res.body.find("Custom 404") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("CONTENT-TYPE PROPAGATION");

    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/json_api.py"), loc);
        check("JSON CT: application/json", res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
        check("JSON CT: body is JSON", res.body.find("{\"status\":\"ok\"") != std::string::npos);
    }

    // No Content-Type → server adds default text/html
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/no_content_type.py"), loc);
        check("no CT: status 200", res.status_code == 200);
        check("no CT: Content-Type added by server", res.headers.count("Content-Type") > 0);
        check("no CT: body present", res.body.find("body without") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("LARGE OUTPUT");

    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/large_output.py"), loc);
        check("large: status 200", res.status_code == 200);
        check("large: body = 100KB", res.body.size() == 100 * 1024);
        check("large: content correct", res.body.find_first_not_of('A') == std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("ERROR HANDLING");

    // script inexistant → 404
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/doesnotexist.py"), loc);
        check("404: script inexistant → 404", res.status_code == 404);
    }

    // script crash → 500
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/crash.py"), loc);
        check("500: script crash → 500", res.status_code == 500);
    }

    // script non executable → 403
    {
        std::ofstream f("www/cgi-bin/noperm.py");
        f << "#!/usr/bin/env python3\nprint('Content-Type: text/html\\n\\nhello')\n";
        f.close();
        chmod("www/cgi-bin/noperm.py", 0644);

        HttpResponse res = cgi.execute(makeGet("/cgi-bin/noperm.py"), loc);
        check("403: non executable → 403", res.status_code == 403);
        remove("www/cgi-bin/noperm.py");
    }

    // script qui ecrit sur stderr → pas de crash, 200
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/stderr_safe.py"), loc);
        check("stderr: pas de crash", res.status_code == 200);
        check("stderr: body correct", res.body.find("stderr_ok") != std::string::npos);
    }

    // script infinite (timeout) → 504
    {
        std::cout << "  [wait] timeout test (5s)..." << std::flush;
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/infinite.py"), loc);
        std::cout << "\n";
        check("timeout: 504 Gateway Timeout", res.status_code == 504);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("METHOD DISPATCH (via MethodHandler)");

    // GET CGI via MethodHandler
    {
        HttpResponse res = handler.handle(makeGet("/cgi-bin/hello.py"), loc, server);
        check("MH: GET CGI → 200", res.status_code == 200);
        check("MH: GET CGI body html", res.body.find("CGI fonctionne") != std::string::npos);
    }

    // POST CGI via MethodHandler
    {
        HttpResponse res = handler.handle(
            makePost("/cgi-bin/hello.py", "message=test42"), loc, server);
        check("MH: POST CGI → 200", res.status_code == 200);
        check("MH: POST CGI body recu", res.body.find("message=test42") != std::string::npos);
        check("MH: POST CGI methode", res.body.find("POST") != std::string::npos);
    }

    // DELETE non autorise sur route CGI → 405
    {
        // Build a location that explicitly allows only GET and POST (no DELETE)
        LocationConfig locGetPost;
        locGetPost.setPath("/cgi-bin");
        locGetPost.setRoot("www");
        locGetPost.setCgiExtension(".py");
        locGetPost.setCgiPath("/usr/bin/python3");
        locGetPost.addMethod("GET");
        locGetPost.addMethod("POST");
        ServerConfig srv2; srv2.addLocation(locGetPost);

        HttpRequest delReq;
        delReq.method  = "DELETE";
        delReq.uri     = "/cgi-bin/hello.py";
        delReq.version = "HTTP/1.1";
        delReq.headers["Host"] = "localhost";

        HttpResponse res = handler.handle(delReq, locGetPost, srv2);
        check("MH: DELETE non autorise → 405", res.status_code == 405);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("EVAL-STYLE END-TO-END");

    // GET → JSON API
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/json_api.py?name=Evaluator"), loc);
        check("e2e: JSON API status 200", res.status_code == 200);
        check("e2e: JSON content-type",
              res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
        check("e2e: JSON body wellformed",
              res.body.find("{") == 0 && res.body.rfind("}") == res.body.size() - 2);
        check("e2e: name dans JSON", res.body.find("Evaluator") != std::string::npos);
    }

    // POST → echo dans redirect (prove CGI chain works)
    {
        HttpResponse res = cgi.execute(makeGet("/cgi-bin/redirect.py"), loc);
        check("e2e: redirect default location",
              res.headers.count("Location") > 0 &&
              res.headers.at("Location") == "https://example.com");
    }

    // POST form-data round-trip
    {
        HttpResponse res = cgi.execute(
            makePost("/cgi-bin/post_form.py", "login=student&project=webserv"), loc);
        check("e2e: form login=student",  res.body.find("login=student") != std::string::npos);
        check("e2e: form project=webserv", res.body.find("project=webserv") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────────────────\n";
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
