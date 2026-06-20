#include "../../../include/http/RequestParser.hpp"
#include "../../../include/http/Router.hpp"
#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/ResponseBuilder.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool condition)
{
    if (condition) { std::cout << "[OK] " << label << std::endl; passed++; }
    else           { std::cout << "[KO] " << label << std::endl; failed++; }
}

static HttpRequest makeReq(const std::string& method, const std::string& uri,
                            const std::string& body = "", const std::string& host = "localhost")
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["host"] = host;
    if (!body.empty())
    {
        req.body = body;
        req.headers["content-type"] = "text/plain";
    }
    return req;
}

// wrapper qui absorbe ParseException — utilisé pour les tests de parsing invalide
static HttpRequest safeParse(const std::string& raw)
{
    RequestParser p;
    HttpRequest   r;
    try { r = p.parse(raw); } catch (const RequestParser::ParseException&) {}
    return r;
}

static LocationConfig makeGetLoc(const std::string& root = "www")
{
    LocationConfig loc;
    loc.setPath("/");
    loc.setRoot(root);
    loc.setIndex("index.html");
    loc.addMethod("GET");
    loc.addMethod("POST");
    loc.addMethod("DELETE");
    return loc;
}

static LocationConfig makeUploadLoc()
{
    LocationConfig loc;
    loc.setPath("/uploads");
    loc.setRoot("/tmp");
    loc.setUploadPath("/tmp");
    loc.addMethod("POST");
    loc.addMethod("DELETE");
    loc.addMethod("GET");
    return loc;
}

int main()
{
    ServerConfig  server;
    MethodHandler handler;
    ResponseBuilder builder;

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── PARSING ATTACKS ─────────────────────────────────────\n";

    // requête vide
    {
        HttpRequest r = safeParse("");
        check("parse: string vide → method vide", r.method.empty());
    }

    // pas de \r\n\r\n
    {
        HttpRequest r = safeParse("GET / HTTP/1.1\r\nHost: x");
        check("parse: pas de separateur → rejeté", r.method.empty());
    }

    // méthode avec espace
    {
        HttpRequest r = safeParse("G ET / HTTP/1.1\r\nHost: x\r\n\r\n");
        check("parse: methode avec espace → rejeté", r.method.empty());
    }

    // trop de tokens sur la première ligne
    {
        HttpRequest r = safeParse("GET / HTTP/1.1 EXTRA\r\nHost: x\r\n\r\n");
        check("parse: trop de tokens ligne 1 → rejeté", r.method.empty());
    }

    // tab après colon (Nginx rejette)
    {
        HttpRequest r = safeParse("GET / HTTP/1.1\r\nHost:\tlocalhost\r\n\r\n");
        check("parse: tab apres colon → rejeté", r.method.empty());
    }

    // HTTP/1.1 sans Host
    {
        HttpRequest r = safeParse("GET / HTTP/1.1\r\nAccept: *\r\n\r\n");
        check("parse: HTTP/1.1 sans Host → rejeté", r.method.empty());
    }

    // version invalide
    {
        HttpRequest r = safeParse("GET / HTTPS/1.1\r\nHost: x\r\n\r\n");
        check("parse: version invalide → rejeté", r.method.empty());
    }

    // header avec null byte dans la valeur (survit au parsing sans crash)
    {
        // Use the length-based constructor to include the embedded null byte —
        // the const char* constructor would stop at the first \0.
        const char buffer[] = "GET / HTTP/1.1\r\nHost: local\x00host\r\n\r\n";
        std::string raw(buffer, sizeof(buffer) - 1);
        safeParse(raw);
        check("parse: header avec null → pas de crash", true);
    }

    // URI très longue (8KB)
    {
        std::string uri(8192, 'a');
        std::string raw = "GET /" + uri + " HTTP/1.1\r\nHost: x\r\n\r\n";
        safeParse(raw);
        check("parse: URI 8KB → pas de crash", true);
    }

    // beaucoup de headers
    {
        std::string raw = "GET / HTTP/1.1\r\nHost: x\r\n";
        for (int i = 0; i < 100; i++) {
            std::ostringstream ss; ss << i;
            raw += "X-Header-" + ss.str() + ": value\r\n";
        }
        raw += "\r\n";
        HttpRequest r = safeParse(raw);
        check("parse: 100 headers → pas de crash", !r.method.empty());
    }

    // body énorme (1MB)
    {
        std::string body(1024 * 1024, 'X');
        std::ostringstream cl; cl << body.size();
        std::string raw = "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: " + cl.str() + "\r\n\r\n" + body;
        HttpRequest r = safeParse(raw);
        check("parse: body 1MB → pas de crash", !r.method.empty());
        check("parse: body 1MB → body correct", r.body.size() == body.size());
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── PATH TRAVERSAL ATTACKS ──────────────────────────────\n";

    {
        LocationConfig loc = makeGetLoc();
        server.addLocation(loc);

        std::string traversals[] = {
            "/../etc/passwd",
            "/../../etc/shadow",
            "/../../../etc/hosts",
            "/uploads/../../etc/passwd",
            "/./././../etc/passwd",
            "/%2e%2e/etc/passwd",
            "/..%2fetc%2fpasswd",
            "/%2e%2e%2fetc%2fpasswd"
        };

        for (int i = 0; i < 8; i++)
        {
            HttpResponse res = handler.handle(makeReq("GET", traversals[i]), loc, server);
            check("GET traversal: " + traversals[i].substr(0, 30) + " → pas 200",
                  res.status_code != 200);
        }
    }

    // POST path traversal sur upload
    {
        LocationConfig loc = makeUploadLoc();
        std::string traversals[] = {
            "/uploads/../../../etc/cron",
            "/uploads/../../tmp/evil",
            "/uploads/../passwd"
        };
        for (int i = 0; i < 3; i++)
        {
            HttpResponse res = handler.handle(makeReq("POST", traversals[i], "evil"), loc, server);
            check("POST traversal: " + traversals[i].substr(0, 35) + " → 400",
                  res.status_code == 400);
        }
    }

    // DELETE path traversal
    {
        LocationConfig loc = makeGetLoc("/tmp");
        std::string traversals[] = {
            "/../etc/passwd",
            "/../../root/.ssh/authorized_keys",
            "/../tmp/../etc/hostname"
        };
        for (int i = 0; i < 3; i++)
        {
            HttpResponse res = handler.handle(makeReq("DELETE", traversals[i]), loc, server);
            check("DELETE traversal → 400", res.status_code == 400);
        }
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── METHOD ATTACKS ──────────────────────────────────────\n";

    {
        LocationConfig loc = makeGetLoc();
        std::string methods[] = { "PUT", "PATCH", "OPTIONS", "HEAD", "TRACE", "CONNECT", "FOOBAR" };
        for (int i = 0; i < 7; i++)
        {
            HttpResponse res = handler.handle(makeReq(methods[i], "/index.html"), loc, server);
            check("methode inconnue " + methods[i] + " → 405", res.status_code == 405);
        }
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── RESPONSE BUILDER ATTACKS ────────────────────────────\n";

    // headers avec valeurs vides
    {
        HttpResponse res;
        res.status_code = 200;
        res.status_msg  = "OK";
        res.headers["Content-Type"]   = "";
        res.headers["X-Empty"]        = "";
        res.body = "test";
        std::string raw = builder.build(res);
        check("builder: headers vides → pas de crash", !raw.empty());
        check("builder: contient status line", raw.find("HTTP/1.1 200") != std::string::npos);
    }

    // status message avec caractères spéciaux
    {
        HttpResponse res;
        res.status_code = 418;
        res.status_msg  = "I'm a teapot!";
        res.body = "";
        std::string raw = builder.build(res);
        check("builder: status message special → correct", raw.find("418") != std::string::npos);
    }

    // body avec null bytes
    {
        HttpResponse res;
        res.status_code = 200;
        res.status_msg  = "OK";
        res.body = std::string("hel\0lo", 6);
        res.headers["Content-Length"] = "6";
        std::string raw = builder.build(res);
        check("builder: body avec null bytes → taille correcte", raw.size() >= 6);
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── ROUTER EDGE CASES ───────────────────────────────────\n";

    {
        ServerConfig  srv;
        LocationConfig loc1; loc1.setPath("/api"); loc1.setRoot("/a"); loc1.addMethod("GET");
        LocationConfig loc2; loc2.setPath("/api/v1"); loc2.setRoot("/b"); loc2.addMethod("GET");
        LocationConfig loc3; loc3.setPath("/api/v1/users"); loc3.setRoot("/c"); loc3.addMethod("GET");
        srv.addLocation(loc1);
        srv.addLocation(loc2);
        srv.addLocation(loc3);

        Router router;

        HttpRequest r1; r1.uri = "/api/v1/users/123";
        check("router: /api/v1/users/123 → /api/v1/users", router.route(r1, srv).getRoot() == "/c");

        HttpRequest r2; r2.uri = "/api/v1/settings";
        check("router: /api/v1/settings → /api/v1", router.route(r2, srv).getRoot() == "/b");

        HttpRequest r3; r3.uri = "/apiv2/test";
        check("router: /apiv2 ne match pas /api → vide", router.route(r3, srv).getPath().empty());

        HttpRequest r4; r4.uri = "/API/v1";
        check("router: case sensitive → vide", router.route(r4, srv).getPath().empty());
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n── FULL PIPELINE FUZZ ──────────────────────────────────\n";

    // requêtes HTTP/1.0 (sans Host) passent au GET
    {
        RequestParser p;
        HttpRequest r = p.parse("GET /index.html HTTP/1.0\r\n\r\n");
        check("HTTP/1.0 sans Host → accepté", !r.method.empty());

        LocationConfig loc = makeGetLoc();
        ServerConfig   srv; srv.addLocation(loc);
        HttpResponse res = handler.handle(r, loc, srv);
        check("HTTP/1.0 GET index → 200", res.status_code == 200);
    }

    // pipeline GET → ResponseBuilder → format correct
    {
        RequestParser p;
        HttpRequest r = p.parse("GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
        LocationConfig loc = makeGetLoc();
        ServerConfig   srv; srv.addLocation(loc);
        HttpResponse res = handler.handle(r, loc, srv);
        std::string raw = builder.build(res);

        check("pipeline: commence par HTTP/1.1", raw.substr(0, 8) == "HTTP/1.1");
        check("pipeline: contient \\r\\n\\r\\n", raw.find("\r\n\r\n") != std::string::npos);
        check("pipeline: body apres separateur", raw.find("\r\n\r\n<!DOCTYPE") != std::string::npos);
    }

    // POST sur route GET-only → 405
    {
        LocationConfig loc; loc.setPath("/"); loc.setRoot("www"); loc.addMethod("GET");
        ServerConfig   srv; srv.addLocation(loc);
        HttpResponse res = handler.handle(makeReq("POST", "/index.html", "data"), loc, srv);
        check("POST sur GET-only → 405", res.status_code == 405);
        std::string raw = builder.build(res);
        check("405 sérialisé correctement", raw.find("405") != std::string::npos);
    }

    // ════════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────────────\n";
    std::cout << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
