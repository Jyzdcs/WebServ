#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/processHttp.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/config/LocationConfig.hpp"
#include "../../../include/config/ServerConfig.hpp"
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <csignal>
#include <ctime>
#include <cstdlib>

static int passed = 0;
static int failed = 0;

static void check(const std::string& label, bool cond)
{
    if (cond) { std::cout << "[OK] " << label << "\n"; ++passed; }
    else       { std::cout << "[KO] " << label << "\n"; ++failed; }
}

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
    req.headers["host"] = host;
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
    req.headers["host"]         = "localhost";
    req.headers["content-type"] = ctype;
    req.body = body;
    return req;
}

static void section(const std::string& title)
{
    std::cout << "\n── " << title << " ";
    for (int i = (int)title.size(); i < 50; i++) std::cout << '-';
    std::cout << "\n";
}

// Exécute un CGI de façon synchrone (test uniquement).
// N'utilise pas finishCgi() (interface serveur) — appelle les primitives directement.
static HttpResponse runCgiSync(const HttpRequest& req, const LocationConfig& loc,
                                const ServerConfig& server)
{
    (void)server;
    CgiHandler    cgi;
    ProcessResult pr = cgi.start(req, loc);

    if (pr.state == ProcessResult::COMPLETE)
        return pr.httpResponse;

    std::string output;
    char        buf[4096];
    bool        timedOut = false;

    while (true)
    {
        ssize_t n = read(pr.stdoutFd, buf, sizeof(buf));
        if (n > 0)
            output.append(buf, n);
        else if (n == 0)
            break;
        else
        {
            if (time(NULL) >= pr.deadline)
            {
                timedOut = true;
                kill(pr.pid, SIGKILL);
                break;
            }
            usleep(5000);
        }
    }
    close(pr.stdoutFd);

    int exitStatus = 0;
    waitpid(pr.pid, &exitStatus, 0);

    if (timedOut)
        return buildHttpError(504, "Gateway Timeout");
    if (WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0)
        return buildHttpError(500, "Internal Server Error");
    if (WIFSIGNALED(exitStatus))
        return buildHttpError(500, "Internal Server Error");
    if (output.empty())
        return buildHttpError(500, "Internal Server Error");

    return cgi.parseOutput(output);
}

int main()
{
    MethodHandler  handler;
    ServerConfig   server;
    LocationConfig loc = makeCgiLoc();
    server.addLocation(loc);

    // ══════════════════════════════════════════════════════════════════════════
    section("ENV VARS TRANSMISSION");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/env_dump.py"), loc, server);
        check("env: status 200", res.status_code == 200);
        check("env: REQUEST_METHOD=GET", res.body.find("REQUEST_METHOD=GET") != std::string::npos);
        check("env: GATEWAY_INTERFACE=CGI/1.1", res.body.find("GATEWAY_INTERFACE=CGI/1.1") != std::string::npos);
        check("env: SERVER_PROTOCOL=HTTP/1.1", res.body.find("SERVER_PROTOCOL=HTTP/1.1") != std::string::npos);
        check("env: SCRIPT_FILENAME non vide", res.body.find("SCRIPT_FILENAME=__MISSING__") == std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makePost("/cgi-bin/env_dump.py", "x=1"), loc, server);
        check("env POST: REQUEST_METHOD=POST", res.body.find("REQUEST_METHOD=POST") != std::string::npos);
        check("env POST: CONTENT_LENGTH=3", res.body.find("CONTENT_LENGTH=3") != std::string::npos);
        check("env POST: CONTENT_TYPE present", res.body.find("CONTENT_TYPE=application/x-www-form-urlencoded") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/env_dump.py?name=Byron&lang=cpp"), loc, server);
        check("env: QUERY_STRING=name=Byron&lang=cpp",
              res.body.find("QUERY_STRING=name=Byron&lang=cpp") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/env_dump.py", "myserver:8080"), loc, server);
        check("env: HTTP_HOST=myserver:8080",
              res.body.find("HTTP_HOST=myserver:8080") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("POST BODY TRANSMISSION");

    {
        std::string body = "hello_webserv_42";
        HttpResponse res = runCgiSync(makePost("/cgi-bin/echo_post.py", body), loc, server);
        check("POST echo: status 200", res.status_code == 200);
        check("POST echo: body identique", res.body == body);
    }

    {
        std::string body = "data=hello%20world&value=42&special=!@#$";
        HttpResponse res = runCgiSync(makePost("/cgi-bin/echo_post.py", body), loc, server);
        check("POST echo: special chars transmis", res.body == body);
    }

    {
        HttpResponse res = runCgiSync(makePost("/cgi-bin/echo_post.py", ""), loc, server);
        check("POST echo: body vide → 200", res.status_code == 200);
        check("POST echo: body vide → response vide", res.body.empty());
    }

    {
        std::string body(10 * 1024, 'Z');
        HttpResponse res = runCgiSync(makePost("/cgi-bin/echo_post.py", body), loc, server);
        check("POST echo: 10KB body transmis", res.body.size() == body.size());
        check("POST echo: contenu correct", res.body == body);
    }

    {
        HttpResponse res = runCgiSync(
            makePost("/cgi-bin/post_form.py", "username=alice&age=30&city=Paris"), loc, server);
        check("POST form: username=alice", res.body.find("username=alice") != std::string::npos);
        check("POST form: age=30",         res.body.find("age=30") != std::string::npos);
        check("POST form: city=Paris",     res.body.find("city=Paris") != std::string::npos);
        check("POST form: CONTENT_TYPE present",
              res.body.find("CONTENT_TYPE=application/x-www-form-urlencoded") != std::string::npos);
    }

    {
        std::string json = "{\"key\":\"value\",\"num\":42}";
        HttpResponse res = runCgiSync(makePost("/cgi-bin/echo_post.py", json, "application/json"), loc, server);
        check("POST json: body correct", res.body == json);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("QUERY STRING PARSING");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/hello.py?name=Byron"), loc, server);
        check("query: name dans body", res.body.find("Byron") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/json_api.py?name=Alice%20Smith"), loc, server);
        check("query: URL-decoded name", res.body.find("Alice Smith") != std::string::npos);
        check("query: content-type JSON",
              res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/env_dump.py?"), loc, server);
        check("query vide: QUERY_STRING=", res.body.find("QUERY_STRING=\n") != std::string::npos ||
                                           res.body.find("QUERY_STRING=\r\n") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("CUSTOM STATUS CODES FROM CGI");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/redirect.py?to=https://42.fr"), loc, server);
        check("CGI 302: status 302", res.status_code == 302);
        check("CGI 302: Location header present", res.headers.count("Location") > 0);
        check("CGI 302: Location correct",
              res.headers.count("Location") > 0 &&
              res.headers.at("Location") == "https://42.fr");
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/custom_404.py"), loc, server);
        check("CGI custom 404: status 404", res.status_code == 404);
        check("CGI custom 404: body present", res.body.find("Custom 404") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("CONTENT-TYPE PROPAGATION");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/json_api.py"), loc, server);
        check("JSON CT: application/json", res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
        check("JSON CT: body is JSON", res.body.find("{\"status\":\"ok\"") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/no_content_type.py"), loc, server);
        check("no CT: status 200", res.status_code == 200);
        check("no CT: Content-Type added by server", res.headers.count("Content-Type") > 0);
        check("no CT: body present", res.body.find("body without") != std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("LARGE OUTPUT");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/large_output.py"), loc, server);
        check("large: status 200", res.status_code == 200);
        check("large: body = 100KB", res.body.size() == 100 * 1024);
        check("large: content correct", res.body.find_first_not_of('A') == std::string::npos);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("ERROR HANDLING");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/doesnotexist.py"), loc, server);
        check("404: script inexistant → 404", res.status_code == 404);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/crash.py"), loc, server);
        check("500: script crash → 500", res.status_code == 500);
    }

    {
        std::ofstream f("www/cgi-bin/noperm.py");
        f << "#!/usr/bin/env python3\nprint('Content-Type: text/html\\n\\nhello')\n";
        f.close();
        chmod("www/cgi-bin/noperm.py", 0644);

        HttpResponse res = runCgiSync(makeGet("/cgi-bin/noperm.py"), loc, server);
        check("403: non executable → 403", res.status_code == 403);
        remove("www/cgi-bin/noperm.py");
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/stderr_safe.py"), loc, server);
        check("stderr: pas de crash", res.status_code == 200);
        check("stderr: body correct", res.body.find("stderr_ok") != std::string::npos);
    }

    {
        std::cout << "  [wait] timeout test (5s)..." << std::flush;
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/infinite.py"), loc, server);
        std::cout << "\n";
        check("timeout: 504 Gateway Timeout", res.status_code == 504);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("METHOD DISPATCH (via MethodHandler)");

    // GET et POST CGI via runCgiSync (handle() ne fait plus de CGI)
    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/hello.py"), loc, server);
        check("MH: GET CGI → 200", res.status_code == 200);
        check("MH: GET CGI body html", res.body.find("CGI fonctionne") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makePost("/cgi-bin/hello.py", "message=test42"), loc, server);
        check("MH: POST CGI → 200", res.status_code == 200);
        check("MH: POST CGI body recu", res.body.find("message=test42") != std::string::npos);
        check("MH: POST CGI methode", res.body.find("POST") != std::string::npos);
    }

    // DELETE non autorise → 405 (vérifié par handle() avant même d'atteindre le CGI)
    {
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

        HttpResponse res = handler.handle(delReq, locGetPost, srv2).httpResponse;
        check("MH: DELETE non autorise → 403", res.status_code == 403);
    }

    // ══════════════════════════════════════════════════════════════════════════
    section("EVAL-STYLE END-TO-END");

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/json_api.py?name=Evaluator"), loc, server);
        check("e2e: JSON API status 200", res.status_code == 200);
        check("e2e: JSON content-type",
              res.headers.count("Content-Type") > 0 &&
              res.headers.at("Content-Type").find("application/json") != std::string::npos);
        check("e2e: JSON body wellformed",
              res.body.find("{") == 0 && res.body.rfind("}") == res.body.size() - 2);
        check("e2e: name dans JSON", res.body.find("Evaluator") != std::string::npos);
    }

    {
        HttpResponse res = runCgiSync(makeGet("/cgi-bin/redirect.py"), loc, server);
        check("e2e: redirect default location",
              res.headers.count("Location") > 0 &&
              res.headers.at("Location") == "https://example.com");
    }

    {
        HttpResponse res = runCgiSync(
            makePost("/cgi-bin/post_form.py", "login=student&project=webserv"), loc, server);
        check("e2e: form login=student",  res.body.find("login=student") != std::string::npos);
        check("e2e: form project=webserv", res.body.find("project=webserv") != std::string::npos);
    }

    std::cout << "\n──────────────────────────────────────────────────────────\n";
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
