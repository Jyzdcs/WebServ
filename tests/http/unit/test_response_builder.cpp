#include "../../../include/http/ResponseBuilder.hpp"
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
        check("200 OK: status line",           startsWith(raw, "HTTP/1.1 200 OK\r\n"));
        check("200 OK: Content-Type header",   contains(raw, "Content-Type: text/html\r\n"));
        check("200 OK: Content-Length header", contains(raw, "Content-Length: 13\r\n"));
        check("200 OK: separateur vide",       contains(raw, "\r\n\r\n"));
        check("200 OK: body",                  contains(raw, "Hello World!\n"));
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
        check("404: status line",   startsWith(raw, "HTTP/1.1 404 Not Found\r\n"));
        check("404: body present",  contains(raw, "Not Found"));
    }

    // ── CAS 3 : 201 Created sans body ───────────────────────────────
    {
        HttpResponse res;
        res.status_code = 201;
        res.status_msg  = "Created";
        res.headers["Location"]       = "/uploads/file.txt";
        res.headers["Content-Length"] = "0";

        std::string raw = builder.build(res);
        check("201 Created: status line",
              startsWith(raw, "HTTP/1.1 201 Created\r\n"));
        check("201 Created: Location header",
              contains(raw, "Location: /uploads/file.txt\r\n"));
        check("201 Created: body vide apres separateur",
              raw.substr(raw.find("\r\n\r\n") + 4).empty());
    }

    // ── CAS 4 : 301 Redirect ────────────────────────────────────────
    {
        HttpResponse res;
        res.status_code = 301;
        res.status_msg  = "Moved Permanently";
        res.headers["Location"] = "https://example.com";

        std::string raw = builder.build(res);
        check("301: status line",    startsWith(raw, "HTTP/1.1 301 Moved Permanently\r\n"));
        check("301: Location header", contains(raw, "Location: https://example.com\r\n"));
    }

    // ── CAS 5 : 405 Method Not Allowed ──────────────────────────────
    {
        HttpResponse res;
        res.status_code = 405;
        res.status_msg  = "Method Not Allowed";
        res.headers["Content-Type"]   = "text/html";
        res.headers["Content-Length"] = "0";

        std::string raw = builder.build(res);
        check("405: status line", startsWith(raw, "HTTP/1.1 405 Method Not Allowed\r\n"));
    }

    // ── CAS 6 : 500 Internal Server Error ───────────────────────────
    {
        HttpResponse res;
        res.status_code = 500;
        res.status_msg  = "Internal Server Error";
        res.body        = "<html><body><h1>Internal Server Error</h1></body></html>";
        res.headers["Content-Type"] = "text/html";

        std::string raw = builder.build(res);
        check("500: status line",  startsWith(raw, "HTTP/1.1 500 Internal Server Error\r\n"));
        check("500: body present", contains(raw, "Internal Server Error"));
    }

    // ── CAS 7 : body vide → pas de contenu apres separateur ─────────
    {
        HttpResponse res;
        res.status_code = 204;
        res.status_msg  = "No Content";

        std::string raw = builder.build(res);
        check("204: status line", startsWith(raw, "HTTP/1.1 204 No Content\r\n"));
        check("204: separateur present", contains(raw, "\r\n\r\n"));
        check("204: body vide", raw.substr(raw.find("\r\n\r\n") + 4).empty());
    }

    // ── CAS 8 : headers multiples ────────────────────────────────────
    {
        HttpResponse res;
        res.status_code = 200;
        res.status_msg  = "OK";
        res.headers["Content-Type"]   = "application/json";
        res.headers["Content-Length"] = "2";
        res.headers["X-Custom"]       = "webserv";
        res.body = "{}";

        std::string raw = builder.build(res);
        check("multi-headers: Content-Type JSON",
              contains(raw, "Content-Type: application/json\r\n"));
        check("multi-headers: X-Custom present",
              contains(raw, "X-Custom: webserv\r\n"));
        check("multi-headers: body JSON", contains(raw, "{}"));
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
