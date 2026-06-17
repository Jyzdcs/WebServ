#include "../../../include/http/MethodHandler.hpp"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>

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

static HttpRequest makeReq(const std::string& method, const std::string& uri)
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    return req;
}

static LocationConfig makeLoc(const std::string& path, const std::string& root, const std::string& method)
{
    LocationConfig loc;
    loc.setPath(path);
    loc.setRoot(root);
    loc.addMethod(method);
    return loc;
}

static void createFile(const std::string& path, const std::string& content)
{
    std::ofstream f(path.c_str());
    f << content;
}

int main()
{
    ServerConfig server;

    // ── CAS 1 : DELETE fichier existant → 204 ───────────────────────
    createFile("/tmp/to_delete.txt", "hello");
    {
        LocationConfig loc = makeLoc("/", "/tmp", "DELETE");
        server.addLocation(loc);
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/to_delete.txt"), loc, server);
        check("DELETE fichier existant: 204", res.status_code == 204);

        struct stat st;
        check("DELETE fichier existant: fichier supprime", stat("/tmp/to_delete.txt", &st) == -1);
    }

    // ── CAS 2 : DELETE fichier inexistant → 404 ─────────────────────
    {
        LocationConfig loc = makeLoc("/", "/tmp", "DELETE");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/inexistant.txt"), loc, server);
        check("DELETE fichier inexistant: 404", res.status_code == 404);
    }

    // ── CAS 3 : DELETE sur un dossier → 403 ─────────────────────────
    {
        LocationConfig loc = makeLoc("/", "/tmp", "DELETE");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/"), loc, server);
        check("DELETE dossier: 403", res.status_code == 403);
    }

    // ── CAS 4 : DELETE methode non autorisee → 405 ──────────────────
    {
        LocationConfig loc = makeLoc("/", "/tmp", "GET");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/to_delete.txt"), loc, server);
        check("DELETE methode non autorisee: 405", res.status_code == 405);
    }

    // ── CAS 5 : DELETE puis GET verifie que le fichier est bien parti
    createFile("/tmp/gone.txt", "bye");
    {
        LocationConfig locDel = makeLoc("/", "/tmp", "DELETE");
        LocationConfig locGet = makeLoc("/", "/tmp", "GET");
        MethodHandler handler;
        handler.handle(makeReq("DELETE", "/gone.txt"), locDel, server);
        HttpResponse res = handler.handle(makeReq("GET", "/gone.txt"), locGet, server);
        check("DELETE puis GET: fichier plus accessible (404)", res.status_code == 404);
    }

    // ── CAS 6 : DELETE path traversal → 400 ────────────────────────
    {
        LocationConfig loc = makeLoc("/", "/tmp", "DELETE");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/../etc/passwd"), loc, server);
        check("DELETE path traversal: 400", res.status_code == 400);
    }

    // ── CAS 7 : DELETE path traversal embede → 400 ──────────────────
    {
        LocationConfig loc = makeLoc("/", "/tmp", "DELETE");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("DELETE", "/uploads/../secret.txt"), loc, server);
        check("DELETE path traversal embede: 400", res.status_code == 400);
    }

    std::cout << std::endl << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
