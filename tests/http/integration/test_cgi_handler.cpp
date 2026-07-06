#include "../../../include/http/MethodHandler.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/processHttp.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <csignal>
#include <ctime>
#include <cstdlib>

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

static HttpRequest makeReq(const std::string& method, const std::string& uri,
                            const std::string& body = "")
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.headers["host"] = "localhost:8080";
    if (!body.empty())
    {
        req.body = body;
        req.headers["content-type"] = "text/plain";
    }
    return req;
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
    ServerConfig   server;
    LocationConfig loc = makeCgiLoc();
    server.addLocation(loc);
    MethodHandler  handler;

    // ── CAS 1 : GET simple → 200 + body HTML ────────────────────────
    {
        HttpResponse res = runCgiSync(makeReq("GET", "/cgi-bin/hello.py"), loc, server);
        check("CGI GET: status 200", res.status_code == 200);
        check("CGI GET: body non vide", !res.body.empty());
        check("CGI GET: Content-Type present", res.headers.count("Content-Type") > 0);
    }

    // ── CAS 2 : GET avec query string → name dans le body ───────────
    {
        HttpResponse res = runCgiSync(makeReq("GET", "/cgi-bin/hello.py?name=Byron"), loc, server);
        check("CGI GET query: 200", res.status_code == 200);
        check("CGI GET query: name dans body", res.body.find("Byron") != std::string::npos);
    }

    // ── CAS 3 : POST avec body → body recu dans la reponse ──────────
    {
        HttpResponse res = runCgiSync(makeReq("POST", "/cgi-bin/hello.py", "message=bonjour"), loc, server);
        check("CGI POST: 200", res.status_code == 200);
        check("CGI POST: body recu", res.body.find("message=bonjour") != std::string::npos);
        check("CGI POST: methode correcte", res.body.find("POST") != std::string::npos);
    }

    // ── CAS 4 : script inexistant → 404 ─────────────────────────────
    {
        HttpResponse res = runCgiSync(makeReq("GET", "/cgi-bin/notfound.py"), loc, server);
        check("CGI 404: status 404", res.status_code == 404);
    }

    // ── CAS 5 : methode non autorisee → 405 ─────────────────────────
    // Testé via handle() car c'est une vérification de routing, pas d'exécution CGI
    {
        LocationConfig locGet = makeCgiLoc();
        locGet.addMethod("GET");
        HttpResponse res = handler.handle(makeReq("DELETE", "/cgi-bin/hello.py"), locGet, server).httpResponse;
        check("CGI 405: methode non autorisee", res.status_code == 405);
    }

    // ── CAS 6 : fichier non executable → 403 ────────────────────────
    {
        std::ofstream f("www/cgi-bin/noperm.py");
        f << "#!/usr/bin/env python3\nprint('Content-Type: text/html\\n\\nhello')\n";
        f.close();
        chmod("www/cgi-bin/noperm.py", 0644);

        HttpResponse res = runCgiSync(makeReq("GET", "/cgi-bin/noperm.py"), loc, server);
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

        HttpResponse res = handler.handle(makeReq("GET", "/index.html"), locGet, server).httpResponse;
        check("Non-CGI: index.html servi normalement", res.status_code == 200);
        check("Non-CGI: pas de CGI pour .html", res.body.find("WebServ") != std::string::npos);
    }

    // ── CAS 8 : script qui freeze → 504 Gateway Timeout ─────────────
    {
        std::cout << "  (attente timeout 5s...)" << std::endl;
        HttpResponse res = runCgiSync(makeReq("GET", "/cgi-bin/infinite.py"), loc, server);
        check("CGI timeout: 504", res.status_code == 504);
    }

    std::cout << std::endl << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
