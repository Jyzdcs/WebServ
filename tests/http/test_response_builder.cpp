#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/MethodHandler.hpp"
#include <iostream>

static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool condition)
{
    if (condition) { std::cout << "[OK] " << label << std::endl; passed++; }
    else           { std::cout << "[KO] " << label << std::endl; failed++; }
}

static bool startsWith(const std::string& s, const std::string& prefix)
{
    return s.substr(0, prefix.size()) == prefix;
}

static bool contains(const std::string& s, const std::string& sub)
{
    return s.find(sub) != std::string::npos;
}

int main()
{
    ResponseBuilder builder;

    // ── CAS 1 : 200 OK avec body ─────────────────────────────────────
    {
        HttpResponse res;
        res.status_code = 200;
        res.status_msg  = "OK";
        res.headers["Content-Type"]   = "text/html";
        res.headers["Content-Length"] = "13";
        res.body = "Hello World!\n";

        std::string raw = builder.build(res);
        check("200 OK: status line", startsWith(raw, "HTTP/1.1 200 OK\r\n"));
        check("200 OK: Content-Type header", contains(raw, "Content-Type: text/html\r\n"));
        check("200 OK: Content-Length header", contains(raw, "Content-Length: 13\r\n"));
        check("200 OK: separateur vide", contains(raw, "\r\n\r\n"));
        check("200 OK: body", contains(raw, "Hello World!\n"));
    }

    // ── CAS 2 : 404 Not Found ────────────────────────────────────────
    {
        HttpResponse res;
        res.status_code = 404;
        res.status_msg  = "Not Found";
        res.body        = "<html><body><h1>Not Found</h1></body></html>";
        res.headers["Content-Type"]   = "text/html";
        res.headers["Content-Length"] = "44";

        std::string raw = builder.build(res);
        check("404: status line", startsWith(raw, "HTTP/1.1 404 Not Found\r\n"));
        check("404: body present", contains(raw, "Not Found"));
    }

    // ── CAS 3 : 201 Created sans body ───────────────────────────────
    {
        HttpResponse res;
        res.status_code = 201;
        res.status_msg  = "Created";
        res.headers["Location"]       = "/uploads/file.txt";
        res.headers["Content-Length"] = "0";

        std::string raw = builder.build(res);
        check("201 Created: status line", startsWith(raw, "HTTP/1.1 201 Created\r\n"));
        check("201 Created: Location header", contains(raw, "Location: /uploads/file.txt\r\n"));
        check("201 Created: body vide apres separateur", raw.substr(raw.find("\r\n\r\n") + 4).empty());
    }

    // ── CAS 4 : 301 Redirect ────────────────────────────────────────
    {
        HttpResponse res;
        res.status_code = 301;
        res.status_msg  = "Moved Permanently";
        res.headers["Location"] = "https://example.com";

        std::string raw = builder.build(res);
        check("301: status line", startsWith(raw, "HTTP/1.1 301 Moved Permanently\r\n"));
        check("301: Location header", contains(raw, "Location: https://example.com\r\n"));
    }

    // ── CAS 5 : integration avec MethodHandler ───────────────────────
    {
        LocationConfig loc;
        loc.setPath("/");
        loc.setRoot("www");
        loc.setIndex("index.html");
        loc.addMethod("GET");

        ServerConfig server;
        server.addLocation(loc);

        HttpRequest req;
        req.method  = "GET";
        req.uri     = "/index.html";
        req.version = "HTTP/1.1";

        MethodHandler handler;
        HttpResponse  res = handler.handle(req, loc, server);
        std::string   raw = builder.build(res);

        check("integration GET: commence par HTTP/1.1", startsWith(raw, "HTTP/1.1 200 OK\r\n"));
        check("integration GET: contient body html", contains(raw, "<!DOCTYPE html>"));
    }

    std::cout << std::endl << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
