#include "../../include/http/MethodHandler.hpp"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool condition)
{
    if (condition) { std::cout << "[OK] " << label << std::endl; passed++; }
    else           { std::cout << "[KO] " << label << std::endl; failed++; }
}

static LocationConfig makeCgiLoc()
{
    LocationConfig loc;
    loc.setPath("/cgi-bin");
    loc.setRoot("www");
    loc.setCgiExtension(".py");
    loc.setCgiPath("/usr/bin/python3");
    loc.addMethod("GET");
    loc.addMethod("POST");
    return loc;
}

static HttpRequest makeReq(const std::string& method, const std::string& uri, const std::string& body = "")
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost:8080";
    if (!body.empty())
    {
        req.body = body;
        req.headers["Content-Type"] = "text/plain";
    }
    return req;
}

int main()
{
    ServerConfig   server;
    LocationConfig loc = makeCgiLoc();
    server.addLocation(loc);
    MethodHandler  handler;

    // ── CAS 1 : GET simple → 200 + body HTML ────────────────────────
    {
        HttpResponse res = handler.handle(makeReq("GET", "/cgi-bin/hello.py"), loc, server);
        check("CGI GET: status 200", res.status_code == 200);
        check("CGI GET: body non vide", !res.body.empty());
        check("CGI GET: Content-Type present", res.headers.count("Content-Type") > 0);
    }

    // ── CAS 2 : GET avec query string → name dans le body ───────────
    {
        HttpResponse res = handler.handle(makeReq("GET", "/cgi-bin/hello.py?name=Byron"), loc, server);
        check("CGI GET query: 200", res.status_code == 200);
        check("CGI GET query: name dans body", res.body.find("Byron") != std::string::npos);
    }

    // ── CAS 3 : POST avec body → body recu dans la reponse ──────────
    {
        HttpResponse res = handler.handle(makeReq("POST", "/cgi-bin/hello.py", "message=bonjour"), loc, server);
        check("CGI POST: 200", res.status_code == 200);
        check("CGI POST: body recu", res.body.find("message=bonjour") != std::string::npos);
        check("CGI POST: methode correcte", res.body.find("POST") != std::string::npos);
    }

    // ── CAS 4 : script inexistant → 404 ─────────────────────────────
    {
        HttpResponse res = handler.handle(makeReq("GET", "/cgi-bin/notfound.py"), loc, server);
        check("CGI 404: status 404", res.status_code == 404);
    }

    // ── CAS 5 : methode non autorisee → 405 ─────────────────────────
    {
        LocationConfig locGet = makeCgiLoc();
        locGet.addMethod("GET");
        HttpResponse res = handler.handle(makeReq("DELETE", "/cgi-bin/hello.py"), locGet, server);
        check("CGI 405: methode non autorisee", res.status_code == 405);
    }

    // ── CAS 6 : fichier non executable → 403 ────────────────────────
    {
        // cree un script sans permission d'execution
        std::ofstream f("www/cgi-bin/noperm.py");
        f << "#!/usr/bin/env python3\nprint('Content-Type: text/html\\n\\nhello')\n";
        f.close();
        chmod("www/cgi-bin/noperm.py", 0644);

        HttpResponse res = handler.handle(makeReq("GET", "/cgi-bin/noperm.py"), loc, server);
        check("CGI 403: script non executable", res.status_code == 403);

        remove("www/cgi-bin/noperm.py");
    }

    // ── CAS 7 : URI sans extension CGI → pas de CGI ─────────────────
    {
        LocationConfig locGet;
        locGet.setPath("/");
        locGet.setRoot("www");
        locGet.setIndex("index.html");
        locGet.addMethod("GET");

        HttpResponse res = handler.handle(makeReq("GET", "/index.html"), locGet, server);
        check("Non-CGI: index.html servi normalement", res.status_code == 200);
        check("Non-CGI: pas de CGI pour .html", res.body.find("WebServ") != std::string::npos);
    }

    std::cout << std::endl << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
