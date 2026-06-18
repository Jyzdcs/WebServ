/*
** BUG HUNTER — HTTP Layer
** Compile:
**   c++ -Wall -Wextra -std=c++98 -I include \
**     tests/http/unit/test_bug_hunter.cpp \
**     src/http/handlers/MethodHandler.cpp \
**     src/http/handlers/GetHandler.cpp \
**     src/http/handlers/PostHandler.cpp \
**     src/http/handlers/DeleteHandler.cpp \
**     src/http/router/Router.cpp \
**     src/http/parser/RequestParser.cpp \
**     src/http/response/ResponseBuilder.cpp \
**     src/http/utils/HttpUtils.cpp \
**     src/http/utils/StringUtils.cpp \
**     src/http/cgi/env.cpp \
**     src/http/cgi/output.cpp \
**     src/http/cgi/execute.cpp \
**     src/config/LocationConfig.cpp \
**     src/config/ServerConfig.cpp \
**     src/config/Config.cpp \
**     -o /tmp/test_bug_hunter && /tmp/test_bug_hunter
*/

#include "../../../include/http/RequestParser.hpp"
#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/Router.hpp"
#include "../../../include/http/ResponseBuilder.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include "../../../include/config/LocationConfig.hpp"
#include "../../../include/config/ServerConfig.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static int g_passed = 0;
static int g_failed = 0;

static void proveStr(const std::string& label,
                     const std::string& expected,
                     const std::string& actual)
{
    if (actual == expected)
    {
        std::cout << "  [OK]  " << label << std::endl;
        ++g_passed;
    }
    else
    {
        std::cout << "  [BUG] " << label << std::endl;
        std::cout << "        expected : \"" << expected << "\"" << std::endl;
        std::cout << "        got      : \"" << actual   << "\"" << std::endl;
        ++g_failed;
    }
}

static void proveB(const std::string& label, bool expected, bool actual)
{
    proveStr(label,
             expected ? "true" : "false",
             actual   ? "true" : "false");
}

static void proveInt(const std::string& label, int expected, int actual)
{
    std::ostringstream e, a;
    e << expected; a << actual;
    proveStr(label, e.str(), a.str());
}

/* Build a minimal server config */
static ServerConfig makeServer(size_t maxBody = 0)
{
    ServerConfig s;
    s.setPort(8080);
    s.setHost("localhost");
    s.setMaxBodySize(maxBody);
    return s;
}

/* Build a minimal location config with comma-separated methods */
static LocationConfig makeLocation(const std::string& path,
                                    const std::string& root,
                                    const std::string& methods = "GET")
{
    LocationConfig loc;
    loc.setPath(path);
    loc.setRoot(root);

    std::string m;
    for (std::size_t i = 0; i <= methods.size(); ++i)
    {
        if (i == methods.size() || methods[i] == ',')
        {
            if (!m.empty()) { loc.addMethod(m); m.clear(); }
        }
        else
            m += methods[i];
    }
    return loc;
}

/* ================================================================== */
/*  BUG #1                                                             */
/*  Host header case-sensitivity                                        */
/*                                                                      */
/*  RFC 7230 §3.2: header field names are case-insensitive.            */
/*  isValid() only matches the literal string "Host" (capital H).      */
/*  A browser sending "host: localhost" has its entire HTTP/1.1        */
/*  request rejected → parse() returns an empty HttpRequest.           */
/*  The client gets a 404 instead of the actual page.                  */
/* ================================================================== */
static void bug1_host_case_sensitivity()
{
    std::cout << "\n[BUG #1] Host header case sensitivity\n";
    std::cout << "  RFC 7230 §3.2: header names MUST be treated case-insensitively.\n";
    std::cout << "  isValid() only accepts literal \"Host\" — lowercase \"host:\" is rejected.\n\n";

    RequestParser parser;

    std::string rawLower =
        "GET / HTTP/1.1\r\n"
        "host: localhost\r\n"       /* valid per RFC, lowercase */
        "\r\n";

    std::string rawCanon =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"       /* canonical form */
        "\r\n";

    HttpRequest reqCanon = parser.parse(rawCanon);
    HttpRequest reqLower = parser.parse(rawLower);

    proveStr("Canonical 'Host:' → method parsed",    "GET", reqCanon.method);
    proveStr("Lowercase 'host:' → method parsed",    "GET", reqLower.method);  /* FAIL */

    std::cout << "  PROOF: RequestParser.cpp line 40:\n";
    std::cout << "    if (currentLine.substr(0, colonPosition) == \"Host\") hasHostHeader = true;\n";
    std::cout << "  \"host\" != \"Host\" → hasHostHeader stays false → isValid() returns false.\n";
}

/* ================================================================== */
/*  BUG #2                                                             */
/*  POST with no upload_path and no CGI → 500 Internal Server Error    */
/*                                                                      */
/*  handlePost() first checks `if (location.getUploadPath().empty())`  */
/*  and immediately returns 500 if no upload_path is configured.       */
/*  Any non-CGI, non-upload POST (e.g. REST endpoint that relies on    */
/*  CGI not being in a separate route) gets 500 instead of a          */
/*  meaningful error.  The evaluator sees "server error" for a         */
/*  perfectly valid client request.                                     */
/* ================================================================== */
static void bug2_post_no_upload_path_returns_500()
{
    std::cout << "\n[BUG #2] POST without upload_path → 500 Internal Server Error\n";
    std::cout << "  handlePost() returns 500 when upload_path is empty.\n";
    std::cout << "  Evaluator reads '500 Internal Server Error' and deducts points.\n\n";

    ServerConfig   server = makeServer();
    LocationConfig loc    = makeLocation("/api", "www", "GET,POST");
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "POST";
    req.uri     = "/api/data";
    req.version = "HTTP/1.1";
    req.body    = "name=test&value=42";

    MethodHandler handler;
    HttpResponse  resp = handler.handle(req, loc, server);

    std::cout << "  Scenario: POST /api/data — POST allowed, no upload_path, no CGI\n";
    std::cout << "  Got: " << resp.status_code << " " << resp.status_msg << "\n\n";

    proveB("Expected: NOT 500 (should be 405/501/200, never a server error)",
           false,
           resp.status_code == 500);   /* FAIL */

    std::cout << "  PROOF: PostHandler.cpp lines 11-12:\n";
    std::cout << "    if (location.getUploadPath().empty())\n";
    std::cout << "        return buildHttpError(500, \"Internal Server Error\");\n";
}

/* ================================================================== */
/*  BUG #3                                                             */
/*  POST with empty body returns 400 Bad Request                       */
/*                                                                      */
/*  RFC 7230 §3.3: a POST MUST accept an empty message body.           */
/*  Content-Length: 0 is a perfectly valid request (trigger endpoints, */
/*  idempotent operations).  handlePost() returns 400 unconditionally  */
/*  when body is empty — this is a protocol violation.                 */
/* ================================================================== */
static void bug3_post_empty_body_returns_400()
{
    std::cout << "\n[BUG #3] POST with empty body → 400 Bad Request (RFC violation)\n";
    std::cout << "  RFC 7230 §3.3: Content-Length: 0 is valid — empty body MUST be accepted.\n\n";

    ServerConfig   server = makeServer();
    LocationConfig loc    = makeLocation("/upload", "www", "POST");
    loc.setUploadPath("/tmp");
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "POST";
    req.uri     = "/upload/trigger.txt";
    req.version = "HTTP/1.1";
    req.body    = "";   /* Content-Length: 0 scenario */

    MethodHandler handler;
    HttpResponse  resp = handler.handle(req, loc, server);

    std::cout << "  Scenario: POST /upload/trigger.txt with empty body (Content-Length: 0)\n";
    std::cout << "  Got: " << resp.status_code << " " << resp.status_msg << "\n\n";

    proveB("Expected: NOT 400 (empty body is valid per RFC 7230)",
           false,
           resp.status_code == 400);   /* FAIL */

    std::cout << "  PROOF: PostHandler.cpp lines 14-15:\n";
    std::cout << "    if (request.body.empty())\n";
    std::cout << "        return buildHttpError(400, \"Bad Request\");\n";
}

/* ================================================================== */
/*  BUG #4                                                             */
/*  HttpResponse has no default constructor → status_code uninitialized*/
/*                                                                      */
/*  `struct HttpResponse { int status_code; ... };`                   */
/*  No constructor → status_code is an indeterminate int on the stack. */
/*  If any code path exits without writing status_code, the client    */
/*  receives a garbled response like "HTTP/1.1 -13812733 ".            */
/*  The ResponseBuilder blindly writes whatever value is in the struct. */
/* ================================================================== */
static void bug4_httpresponse_uninitialized_status_code()
{
    std::cout << "\n[BUG #4] HttpResponse::status_code is UNINITIALIZED by default\n";
    std::cout << "  No constructor in HttpResponse.hpp → status_code = indeterminate int.\n\n";

    HttpResponse r;

    std::cout << "  HttpResponse r;  →  r.status_code = " << r.status_code << "\n";
    std::cout << "  (value is indeterminate; any non-zero value proves the bug)\n\n";

    /* The status_msg std::string IS default-constructed to empty */
    proveStr("status_msg default-constructed to empty string", "", r.status_msg);

    /* status_code is NOT 0 in many stack contexts (indeterminate) */
    /* We just flag that the struct has no constructor to guarantee 0 */
    proveInt("status_code default-initialized to 0 (needs constructor)",
             0, r.status_code);   /* May FAIL depending on stack state */

    /* Show what ResponseBuilder emits with a default-constructed response */
    ResponseBuilder rb;
    HttpResponse    uninitialized;
    std::string     raw = rb.build(uninitialized);
    std::cout << "  ResponseBuilder::build() output with uninitialized HttpResponse:\n";
    std::cout << "    " << raw.substr(0, raw.find("\r\n")) << "\n\n";
    std::cout << "  PROOF: HttpResponse.hpp has no constructor.\n";
    std::cout << "  Even if it currently happens to be 0 in debug builds, release builds\n";
    std::cout << "  with optimizations may produce garbage — undefined behavior.\n";
}

/* ================================================================== */
/*  BUG #5                                                             */
/*  Chunked Transfer-Encoding body is NOT decoded                      */
/*                                                                      */
/*  Subject §IV.3: "Chunked requests must be un-chunked before        */
/*  passing to CGI (EOF = end of body)".                               */
/*  parseBody() does rawRequest.substr(headerBodySeparator + 4).       */
/*  The raw chunked wire format (size\r\ndata\r\n...0\r\n\r\n) is     */
/*  passed verbatim to the body. CGI and POST handlers receive garbage.*/
/* ================================================================== */
static void bug5_chunked_body_not_decoded()
{
    std::cout << "\n[BUG #5] Chunked Transfer-Encoding body is NOT decoded\n";
    std::cout << "  Subject §IV.3 explicitly requires de-chunking before CGI.\n\n";

    /*
    ** Wire format for chunked body "Hello":
    **   5\r\n        <- chunk size in hex
    **   Hello\r\n   <- chunk data + CRLF
    **   0\r\n        <- terminating chunk
    **   \r\n         <- final empty trailer
    */
    std::string raw =
        "POST /upload/data.txt HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "Hello\r\n"
        "0\r\n"
        "\r\n";

    RequestParser parser;
    HttpRequest   req = parser.parse(raw);

    std::string expectedBody = "Hello";
    std::string actualBody   = req.body;

    std::cout << "  Request has Transfer-Encoding: chunked with payload 'Hello'.\n";
    std::cout << "  Expected body: \"" << expectedBody << "\"\n";
    std::cout << "  Actual body:   \"" << actualBody   << "\"\n\n";

    proveStr("Chunked body decoded to actual content (no chunk-size lines)",
             expectedBody, actualBody);    /* FAIL */

    std::cout << "  PROOF: RequestParser.cpp line 90:\n";
    std::cout << "    request.body = rawRequest.substr(headerBodySeparator + 4);\n";
    std::cout << "  No chunked decoding at all. CGI receives '5\\r\\nHello\\r\\n0\\r\\n\\r\\n'.\n";
}

/* ================================================================== */
/*  BUG #6                                                             */
/*  Missing Connection: close header → HTTP/1.1 keep-alive hangs       */
/*                                                                      */
/*  HTTP/1.1 default is persistent connections (keep-alive).           */
/*  The server never sends Connection: close.  HTTP/1.1 clients will  */
/*  keep the TCP socket open after the response, waiting for more.     */
/*  Evaluation §7 "Pas de connexions pendantes (hanging connections)". */
/* ================================================================== */
static void bug6_missing_connection_close_header()
{
    std::cout << "\n[BUG #6] ResponseBuilder never adds 'Connection: close'\n";
    std::cout << "  HTTP/1.1 clients assume keep-alive by default → hanging connections.\n";
    std::cout << "  Evaluation §7: 'Pas de connexions pendantes'.\n\n";

    HttpResponse resp;
    resp.status_code = 200;
    resp.status_msg  = "OK";
    resp.body        = "hello";
    resp.headers["Content-Type"]   = "text/plain";
    resp.headers["Content-Length"] = "5";

    ResponseBuilder rb;
    std::string     raw = rb.build(resp);

    bool hasConnectionHeader = (raw.find("Connection:") != std::string::npos);

    std::cout << "  Built response headers:\n";
    std::size_t sep = raw.find("\r\n\r\n");
    if (sep != std::string::npos)
        std::cout << "    " << raw.substr(0, sep) << "\n\n";

    proveB("Response contains Connection header",
           true, hasConnectionHeader);    /* FAIL */

    std::cout << "  PROOF: ResponseBuilder.cpp — no Connection header ever written.\n";
    std::cout << "  Without Connection: close, HTTP/1.1 clients hold the socket open forever.\n";
}

/* ================================================================== */
/*  BUG #7                                                             */
/*  CGI env missing required CGI/1.1 variables                         */
/*                                                                      */
/*  RFC 3875 §4.1 defines REQUIRED meta-variables:                     */
/*    SERVER_NAME, SERVER_PORT, SCRIPT_NAME, REMOTE_ADDR, AUTH_TYPE   */
/*  buildEnv() sets: REQUEST_METHOD, QUERY_STRING, SCRIPT_FILENAME,   */
/*  PATH_INFO, CONTENT_TYPE, CONTENT_LENGTH, HTTP_HOST,                */
/*  SERVER_PROTOCOL, GATEWAY_INTERFACE.                                 */
/*  Missing: SERVER_NAME, SERVER_PORT, SCRIPT_NAME, REMOTE_ADDR.       */
/*  Any Python CGI using os.environ['SERVER_NAME'] raises KeyError.   */
/*                                                                      */
/*  We prove this by running a real CGI script that dumps its env.     */
/* ================================================================== */
static void bug7_cgi_env_missing_required_vars()
{
    std::cout << "\n[BUG #7] CGI env missing required CGI/1.1 variables\n";
    std::cout << "  RFC 3875 §4.1: SERVER_NAME, SERVER_PORT, SCRIPT_NAME are REQUIRED.\n\n";

    /*
    ** Use www/cgi-bin/env_required.py — a script that prints every
    ** RFC 3875 §4.1 required variable (or "__MISSING__" if absent).
    ** buildScriptPath() = cwd + "/" + root + uri
    **                   = cwd + "/" + "www" + "/cgi-bin/env_required.py"
    */
    LocationConfig loc;
    loc.setPath("/cgi-bin");
    loc.setRoot("www");
    loc.addMethod("GET");
    loc.setCgiExtension(".py");
    loc.setCgiPath("/Users/byronlove/.local/bin/python3");

    HttpRequest req;
    req.method  = "GET";
    req.uri     = "/cgi-bin/env_required.py";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost:8080";

    CgiHandler   handler;
    HttpResponse resp = handler.execute(req, loc);

    std::cout << "  CGI output (env dump):\n";
    /* Print each line prefixed */
    std::istringstream iss(resp.body);
    std::string        line;
    while (std::getline(iss, line))
        std::cout << "    " << line << "\n";
    std::cout << "\n";

    /* A variable is "missing" if the CGI script printed VAR=__MISSING__ */
    bool serverNameMissing  = (resp.body.find("SERVER_NAME=__MISSING__")  != std::string::npos);
    bool serverPortMissing  = (resp.body.find("SERVER_PORT=__MISSING__")  != std::string::npos);
    bool scriptNameMissing  = (resp.body.find("SCRIPT_NAME=__MISSING__")  != std::string::npos);
    bool remoteAddrMissing  = (resp.body.find("REMOTE_ADDR=__MISSING__")  != std::string::npos);
    bool gatewayPresent     = (resp.body.find("GATEWAY_INTERFACE=CGI/1.1") != std::string::npos);

    proveB("SERVER_NAME is NOT missing (RFC 3875 §4.1.14 REQUIRED)",
           false, serverNameMissing);   /* FAIL */
    proveB("SERVER_PORT is NOT missing (RFC 3875 §4.1.15 REQUIRED)",
           false, serverPortMissing);   /* FAIL */
    proveB("SCRIPT_NAME is NOT missing (RFC 3875 §4.1.13 REQUIRED)",
           false, scriptNameMissing);   /* FAIL */
    proveB("REMOTE_ADDR is NOT missing (RFC 3875 §4.1.8 REQUIRED)",
           false, remoteAddrMissing);   /* FAIL */
    proveB("GATEWAY_INTERFACE=CGI/1.1 is present (in buildEnv)",
           true, gatewayPresent);       /* OK */

    std::cout << "  PROOF: env.cpp buildEnv() — no SERVER_NAME / SERVER_PORT /\n";
    std::cout << "  SCRIPT_NAME / REMOTE_ADDR written.\n";
    std::cout << "  Python `os.environ['SERVER_NAME']` would raise KeyError.\n";
}

/* ================================================================== */
/*  BUG #8                                                             */
/*  Autoindex includes '.' and '..' entries with live links            */
/*                                                                      */
/*  buildAutoindex() iterates readdir() without filtering '.' or '..'.  */
/*  The browser sees clickable links to '.' (infinite loop) and '..'   */
/*  (navigates outside configured root).  Clicking '..' then clicks    */
/*  '/../' which hasPathTraversal() rejects with 400 — inconsistent    */
/*  UX — but the parent dir listing may still be accessible.           */
/* ================================================================== */
static void bug8_autoindex_includes_dot_entries()
{
    std::cout << "\n[BUG #8] Autoindex includes '.' and '..' entries\n";
    std::cout << "  readdir() output not filtered → live links to . and .. in browser.\n\n";

    ServerConfig   server = makeServer();
    LocationConfig loc    = makeLocation("/", "/tmp", "GET");
    loc.setAutoindex(true);
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "GET";
    req.uri     = "/";
    req.version = "HTTP/1.1";

    MethodHandler handler;
    HttpResponse  resp = handler.handle(req, loc, server);

    bool hasDotLink    = (resp.body.find(">.<")  != std::string::npos ||
                          resp.body.find("\"/.\"") != std::string::npos ||
                          resp.body.find("\"/./\"") != std::string::npos);
    bool hasDotDotLink = (resp.body.find(">..<") != std::string::npos ||
                          resp.body.find("\"/..\"") != std::string::npos ||
                          resp.body.find("\"/../\"") != std::string::npos);

    std::cout << "  Autoindex response body:\n  " << resp.body.substr(0, 500) << "\n\n";

    proveB("Autoindex does NOT contain a '.' entry link",
           false, hasDotLink);     /* FAIL */
    proveB("Autoindex does NOT contain a '..' entry link",
           false, hasDotDotLink);  /* FAIL */

    std::cout << "  PROOF: GetHandler.cpp buildAutoindex() — readdir() without filtering:\n";
    std::cout << "    while ((dirEntry = readdir(directory)) != NULL)\n";
    std::cout << "    { entryLink += entryName; ... }   // no check for . or ..\n";
}

/* ================================================================== */
/*  BUG #9                                                             */
/*  Path traversal detection: semicolon bypass & double-encode gap     */
/*                                                                      */
/*  hasPathTraversal() performs one pass of urlDecode() then checks:   */
/*    '/../', ending '/..', starting '../', exact '..'.                 */
/*  Case A: '/foo/%2e%2e'  → decoded '/foo/..' → ends with '/..' ✓    */
/*  Case B: '/foo/%252e%252e' → decoded '/foo/%2e%2e' (NOT further     */
/*    decoded) → does NOT match any check → MISSED.                    */
/*    On Linux the OS won't interpret %2e as dot so no real traversal, */
/*    but the server returns 404 instead of 400 — behaviour differs    */
/*    from NGINX which returns 400 for all encoded-dot sequences.      */
/*  Case C: '/foo/..;/secret' → NOT caught — semicolon trick bypasses  */
/*    the literal string checks entirely.                               */
/* ================================================================== */
static void bug9_path_traversal_gaps()
{
    std::cout << "\n[BUG #9] Path traversal detection gaps\n";
    std::cout << "  hasPathTraversal() misses double-encoded and semicolon-bypass patterns.\n\n";

    const std::string caseA = "/foo/%2e%2e";        /* single-encoded → should be caught */
    const std::string caseB = "/foo/%252e%252e";    /* double-encoded → slips through */
    const std::string caseC = "/foo/..;/secret";    /* semicolon trick */

    bool caughtA = hasPathTraversal(caseA);
    bool caughtB = hasPathTraversal(caseB);
    bool caughtC = hasPathTraversal(caseC);

    std::cout << "  Case A '" << caseA << "' → " << (caughtA ? "CAUGHT (400)" : "MISSED") << "\n";
    std::cout << "  Case B '" << caseB << "' → " << (caughtB ? "CAUGHT (400)" : "MISSED — returns 404, not 400") << "\n";
    std::cout << "  Case C '" << caseC << "' → " << (caughtC ? "CAUGHT (400)" : "MISSED — proceeds to file lookup") << "\n\n";

    proveB("Case A '/foo/%2e%2e' is detected",   true, caughtA);   /* OK */
    proveB("Case B '/foo/%252e%252e' is detected (double-encoded)", true, caughtB);   /* FAIL */
    proveB("Case C '/foo/..;/secret' is detected (semicolon)",      true, caughtC);   /* FAIL */

    std::cout << "  PROOF: StringUtils.cpp urlDecode() runs exactly one pass.\n";
    std::cout << "  '%252e' → first decode pass → '%2e' → stops. The '/..' check\n";
    std::cout << "  sees '%2e%2e' which contains neither '/..' nor '../'.\n";
}

/* ================================================================== */
/*  BUG #10                                                            */
/*  DELETE 204 response has no Content-Length header                   */
/*                                                                      */
/*  RFC 7230 §3.3: for 204 No Content, no message body is allowed     */
/*  and Content-Length SHOULD be absent.  That part is correct.        */
/*  BUT: the 204 response also has NO other length-framing mechanism.  */
/*  Strict HTTP/1.1 clients that need explicit framing may hang.       */
/*  More importantly: there is NO 'Server' header in ANY response     */
/*  (minor but visible during evaluation with browser devtools).       */
/*  Most critically: DELETE success returns 204 but the response has   */
/*  zero headers — no Content-Type, no Content-Length, no Server.     */
/* ================================================================== */
static void bug10_delete_response_has_no_headers()
{
    std::cout << "\n[BUG #10] DELETE 204 response has zero response headers\n";
    std::cout << "  handleDelete() returns a bare `HttpResponse` with no headers set.\n\n";

    /* Create a temp file to delete */
    const char* tmpFile = "/tmp/webserv_bug10_test.txt";
    {
        std::ofstream f(tmpFile);
        f << "delete me";
    }

    ServerConfig   server = makeServer();
    LocationConfig loc    = makeLocation("/", "/", "DELETE");
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "DELETE";
    req.uri     = tmpFile;
    req.version = "HTTP/1.1";

    MethodHandler handler;
    HttpResponse  resp = handler.handle(req, loc, server);

    std::cout << "  DELETE of '" << tmpFile << "'\n";
    std::cout << "  status_code = " << resp.status_code << " (" << resp.status_msg << ")\n";
    std::cout << "  headers set: " << resp.headers.size() << "\n";
    for (std::map<std::string,std::string>::const_iterator it = resp.headers.begin();
         it != resp.headers.end(); ++it)
        std::cout << "    " << it->first << ": " << it->second << "\n";
    std::cout << "\n";

    ResponseBuilder rb;
    std::string     built = rb.build(resp);
    bool hasServerHeader = (built.find("Server:") != std::string::npos);

    std::cout << "  Full built response:\n  "
              << built.substr(0, built.find("\r\n\r\n")) << "\n\n";

    proveInt("DELETE succeeds with 204", 204, resp.status_code);
    proveB("Response has at least 1 header (e.g. Server or Date)",
           true, resp.headers.size() > 0);   /* FAIL: headers map is empty */
    proveB("Response includes a Server header",
           true, hasServerHeader);   /* FAIL */

    std::cout << "  PROOF: DeleteHandler.cpp — success path sets only status_code/status_msg.\n";
    std::cout << "  No headers added at all. ResponseBuilder adds only Date.\n";
    std::cout << "  Browser devtools will show a very bare response.\n";
}

/* ================================================================== */
/*  BUG #11                                                            */
/*  CGI: read() on pipe called WITHOUT poll() — GRADE 0                */
/*                                                                      */
/*  Subject §IV.1: "Never do a read or write without going through    */
/*  poll() first → grade 0".                                           */
/*  execute.cpp readCgiOutputWithTimeout() sets the pipe O_NONBLOCK    */
/*  then calls read() in a busy-wait / usleep(5000) loop.              */
/*  Zero poll() calls. Automatic grade 0.                              */
/*  Bonus: the entire server event loop is blocked while CGI runs.     */
/*  No other client can be served during this time.                    */
/* ================================================================== */
static void bug11_cgi_read_without_poll()
{
    std::cout << "\n[BUG #11] [GRADE-0] CGI reads pipe WITHOUT poll()\n";
    std::cout << "  Subject §IV.1 + evaluation §1: grade 0 if read() without poll().\n\n";

    std::cout << "  execute.cpp readCgiOutputWithTimeout() — verbatim:\n";
    std::cout << "  --------------------------------------------------\n";
    std::cout << "  fcntl(pipeReadEnd, F_SETFL, O_NONBLOCK);\n";
    std::cout << "  time_t timeoutDeadline = time(NULL) + CGI_TIMEOUT_SEC;\n";
    std::cout << "  while (true) {\n";
    std::cout << "      ssize_t bytesRead = read(pipeReadEnd, ...);  // NO poll() before this\n";
    std::cout << "      if (bytesRead > 0)  { ... }\n";
    std::cout << "      else if (bytesRead == 0) { break; }\n";
    std::cout << "      else {\n";
    std::cout << "          if (time(NULL) >= timeoutDeadline) { kill(); return; }\n";
    std::cout << "          usleep(5000);  // spin-wait\n";
    std::cout << "      }\n";
    std::cout << "  }\n";
    std::cout << "  --------------------------------------------------\n\n";
    std::cout << "  Evaluation criterion §1 (note 0 if found):\n";
    std::cout << "  'La valeur de retour de read/recv/write/send est correctement verifiee'\n";
    std::cout << "  'Never do a read or write without going through poll() first'\n\n";

    /* Structural proof: read() in execute.cpp is never preceded by poll() */
    proveB("CGI pipe read goes through poll() [GRADE-0 violation]",
           true, false);  /* Provably false by code inspection */

    std::cout << "  PROOF: execute.cpp contains no poll()/select()/epoll_wait()/kevent() call.\n";
    std::cout << "  The single poll() lives in the main event loop (PollManager), but\n";
    std::cout << "  readCgiOutputWithTimeout() bypasses it entirely with its own loop.\n";
}

/* ================================================================== */
/*  BUG #12                                                            */
/*  Router: location path with trailing slash never matches            */
/*                                                                      */
/*  matchesLocation('/static/', '/static/file.txt') returns false.     */
/*  After prefix match, uriContinuesAfterPrefix=true, nextCharIsSlash  */
/*  checks requestUri[8] which is 'f' (not '/'), locationIsRoot=false.  */
/*  The condition `uriContinuesAfterPrefix && !nextCharIsSlash &&      */
/*  !locationIsRoot` is true → return false.  No file under a trailing-*/
/*  slash location ever matches.                                        */
/* ================================================================== */
static void bug12_location_trailing_slash_never_matches()
{
    std::cout << "\n[BUG #12] Location path with trailing slash never matches any URI\n";
    std::cout << "  matchesLocation('/static/', '/static/file.txt') → false (should be true)\n\n";

    ServerConfig   server = makeServer();
    LocationConfig loc;
    loc.setPath("/static/");   /* trailing slash */
    loc.setRoot("www");
    loc.addMethod("GET");
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "GET";
    req.uri     = "/static/file.txt";
    req.version = "HTTP/1.1";

    Router         router;
    LocationConfig matched = router.route(req, server);

    std::cout << "  Location path = '/static/'  Request URI = '/static/file.txt'\n";
    std::cout << "  Router returned location path: '" << matched.getPath() << "'\n\n";

    proveStr("Router matched '/static/' for '/static/file.txt'",
             "/static/", matched.getPath());    /* FAIL: returns "" */

    std::cout << "  PROOF: Router.cpp matchesLocation():\n";
    std::cout << "    uriContinuesAfterPrefix = (8 < 14) = true\n";
    std::cout << "    nextCharIsSlash = requestUri[8] = 'f' = false\n";
    std::cout << "    locationIsRoot  = ('/static/' == '/') = false\n";
    std::cout << "    → condition (true && true && true) = true → return false\n";
    std::cout << "  Any config with location /path/ will silently serve nothing.\n";
}

/* ================================================================== */
/*  BUG #13                                                            */
/*  GET directory + index configured but missing → 404, ignores        */
/*  autoindex fallback even when autoindex is enabled                  */
/*                                                                      */
/*  When a request hits a directory, handleGet() enters the            */
/*  `!location.getIndex().empty()` branch, appends the index filename, */
/*  then calls open(). If open() fails (ENOENT), it returns 404.       */
/*  Autoindex is never reconsidered.  If both index and autoindex are  */
/*  configured, and the index file is absent, the user gets 404         */
/*  instead of the directory listing.                                   */
/* ================================================================== */
static void bug13_missing_index_ignores_autoindex()
{
    std::cout << "\n[BUG #13] index file missing + autoindex=on → 404, not directory listing\n";
    std::cout << "  When index file is absent, autoindex fallback is never attempted.\n\n";

    ServerConfig   server = makeServer();
    LocationConfig loc    = makeLocation("/", "/tmp", "GET");
    loc.setIndex("DOES_NOT_EXIST_index.html");
    loc.setAutoindex(true);
    server.addLocation(loc);

    HttpRequest req;
    req.method  = "GET";
    req.uri     = "/";
    req.version = "HTTP/1.1";

    MethodHandler handler;
    HttpResponse  resp = handler.handle(req, loc, server);

    std::cout << "  root=/tmp, index=<missing>, autoindex=true\n";
    std::cout << "  Got: " << resp.status_code << " " << resp.status_msg << "\n\n";

    proveInt("Expected 200 with directory listing (autoindex fallback)",
             200, resp.status_code);   /* FAIL: returns 404 */

    std::cout << "  PROOF: GetHandler.cpp — once the index branch is entered,\n";
    std::cout << "  open() failure returns 404 with no autoindex retry:\n";
    std::cout << "    if (!location.getIndex().empty()) {\n";
    std::cout << "        filePath += location.getIndex();  // now points to missing file\n";
    std::cout << "    }\n";
    std::cout << "    // ... then open(filePath) fails → 404 — autoindex never checked again\n";
}

/* ================================================================== */
/*  SUMMARY                                                            */
/* ================================================================== */

int main()
{
    std::cout << "====================================================\n";
    std::cout << " HTTP Layer — Bug Hunter Results\n";
    std::cout << "====================================================\n";

    bug1_host_case_sensitivity();
    bug2_post_no_upload_path_returns_500();
    bug3_post_empty_body_returns_400();
    bug4_httpresponse_uninitialized_status_code();
    bug5_chunked_body_not_decoded();
    bug6_missing_connection_close_header();
    bug7_cgi_env_missing_required_vars();
    bug8_autoindex_includes_dot_entries();
    bug9_path_traversal_gaps();
    bug10_delete_response_has_no_headers();
    bug11_cgi_read_without_poll();
    bug12_location_trailing_slash_never_matches();
    bug13_missing_index_ignores_autoindex();

    std::cout << "\n====================================================\n";
    std::cout << " TOTAL: " << g_passed << " OK, " << g_failed << " BUG\n";
    std::cout << "====================================================\n";

    return 0;
}
