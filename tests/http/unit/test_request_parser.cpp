#include "../../../include/http/RequestParser.hpp"
#include <iostream>
#include <cassert>

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

static void test_get_simple()
{
    RequestParser parser;
    std::string raw = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("GET simple: method",  req.method  == "GET");
    check("GET simple: uri",     req.uri     == "/index.html");
    check("GET simple: version", req.version == "HTTP/1.1");
    check("GET simple: host header", req.headers["Host"] == "localhost");
    check("GET simple: body vide", req.body == "");
}

static void test_post_with_body()
{
    RequestParser parser;
    std::string raw =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=toto&password=1234";
    HttpRequest req = parser.parse(raw);

    check("POST body: method",  req.method == "POST");
    check("POST body: uri",     req.uri    == "/upload");
    check("POST body: Content-Length header", req.headers["Content-Length"] == "27");
    check("POST body: body", req.body == "username=toto&password=1234");
}

static void test_invalid_no_separator()
{
    RequestParser parser;
    std::string raw = "GET /index.html HTTP/1.1\r\nHost: localhost";
    HttpRequest req = parser.parse(raw);

    check("Invalide sans separator: method vide", req.method.empty());
}

static void test_invalid_bad_version()
{
    RequestParser parser;
    std::string raw = "GET /index.html NOTHTTP\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Invalide mauvaise version: method vide", req.method.empty());
}

static void test_invalid_missing_uri()
{
    RequestParser parser;
    std::string raw = "GET\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Invalide uri manquante: method vide", req.method.empty());
}

static void test_multiple_headers()
{
    RequestParser parser;
    std::string raw =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Accept: text/html\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    HttpRequest req = parser.parse(raw);

    check("Multi-headers: Accept",     req.headers["Accept"]     == "text/html");
    check("Multi-headers: Connection", req.headers["Connection"] == "keep-alive");
}

static void test_query_string()
{
    RequestParser parser;
    std::string raw = "GET /search?q=foo&page=2 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Query string: method", req.method == "GET");
    check("Query string: uri complet", req.uri == "/search?q=foo&page=2");
}

static void test_header_value_with_colon()
{
    RequestParser parser;
    std::string raw =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Date: Mon, 16 Jun 2026 00:00:00 GMT\r\n"
        "\r\n";
    HttpRequest req = parser.parse(raw);

    check("Header avec colon: valeur correcte", req.headers["Date"] == "Mon, 16 Jun 2026 00:00:00 GMT");
}

static void test_post_empty_body()
{
    RequestParser parser;
    std::string raw =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    HttpRequest req = parser.parse(raw);

    check("POST body vide: method", req.method == "POST");
    check("POST body vide: body vide", req.body == "");
}

static void test_delete_method()
{
    RequestParser parser;
    std::string raw = "DELETE /resource/42 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("DELETE: method", req.method == "DELETE");
    check("DELETE: uri",    req.uri    == "/resource/42");
}

static void test_root_uri()
{
    RequestParser parser;
    std::string raw = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("URI root: uri", req.uri == "/");
}

static void test_invalid_empty()
{
    RequestParser parser;
    HttpRequest req = parser.parse("");

    check("Invalide string vide: method vide", req.method.empty());
}

static void test_invalid_extra_token_first_line()
{
    RequestParser parser;
    std::string raw = "GET /index.html HTTP/1.1 EXTRA\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Invalide token en trop: method vide", req.method.empty());
}
static void test_header_no_space_after_colon()
{
    RequestParser parser;
    std::string raw = "GET / HTTP/1.1\r\nHost:localhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Header sans espace: valeur correcte", req.headers["Host"] == "localhost");
}

static void test_header_tab_after_colon()
{
    RequestParser parser;
    std::string raw = "GET / HTTP/1.1\r\nHost:\tlocalhost\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("Header avec tab: rejeté comme nginx", req.method.empty());
}

static void test_http11_without_host()
{
    RequestParser parser;
    std::string raw = "GET / HTTP/1.1\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("HTTP/1.1 sans Host: rejeté comme nginx", req.method.empty());
}

static void test_http10_without_host()
{
    RequestParser parser;
    std::string raw = "GET / HTTP/1.0\r\n\r\n";
    HttpRequest req = parser.parse(raw);

    check("HTTP/1.0 sans Host: accepté comme nginx", req.method == "GET");
}

int main()
{
    test_get_simple();
    test_post_with_body();
    test_invalid_no_separator();
    test_invalid_bad_version();
    test_invalid_missing_uri();
    test_multiple_headers();

    test_query_string();
    test_header_value_with_colon();
    test_post_empty_body();
    test_delete_method();
    test_root_uri();
    test_invalid_empty();
    test_invalid_extra_token_first_line();

    test_header_no_space_after_colon();
    test_header_tab_after_colon();
    test_http11_without_host();
    test_http10_without_host();

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed == 0 ? 0 : 1;
}
