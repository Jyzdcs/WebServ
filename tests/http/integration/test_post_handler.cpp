#include "../../../include/http/MethodHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
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

static HttpRequest makeReq(const std::string& method, const std::string& uri, const std::string& body = "")
{
    HttpRequest req;
    req.method  = method;
    req.uri     = uri;
    req.version = "HTTP/1.1";
    req.body    = body;
    return req;
}

static LocationConfig makeLoc(const std::string& uploadPath)
{
    LocationConfig loc;
    loc.setPath("/uploads");
    loc.setRoot("/tmp");
    loc.setUploadPath(uploadPath);
    loc.addMethod("POST");
    return loc;
}

static std::string readFile(const std::string& path)
{
    std::ifstream f(path.c_str());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main()
{
    ServerConfig server;

    // ── CAS 1 : POST upload nominal → 201 + fichier cree ────────────
    {
        LocationConfig loc = makeLoc("/tmp");
        server.addLocation(loc);
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/hello.txt", "hello world"), loc, server);
        check("POST upload: 201", res.status_code == 201);
        check("POST upload: Location header", res.headers.count("Location") > 0);
        check("POST upload: fichier cree sur disque", readFile("/tmp/hello.txt") == "hello world");
    }

    // ── CAS 2 : POST sans upload_store → 500 ────────────────────────
    {
        LocationConfig loc = makeLoc("");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/test.txt", "data"), loc, server);
        check("POST sans upload_store: 500", res.status_code == 500);
    }

    // ── CAS 3 : POST body vide → 201 (fichier vide créé, valide HTTP) ─
    {
        LocationConfig loc = makeLoc("/tmp");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/empty.txt", ""), loc, server);
        check("POST body vide: 201", res.status_code == 201);
    }

    // ── CAS 4 : POST URI sans filename → 400 ────────────────────────
    {
        LocationConfig loc = makeLoc("/tmp");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/", "data"), loc, server);
        check("POST URI sans filename: 400", res.status_code == 400);
    }

    // ── CAS 5 : POST path traversal → 400 ───────────────────────────
    {
        LocationConfig loc = makeLoc("/tmp");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/../etc/passwd", "data"), loc, server);
        check("POST path traversal: 400", res.status_code == 400);
    }

    // ── CAS 6 : POST methode non autorisee → 405 ────────────────────
    {
        LocationConfig loc;
        loc.setPath("/uploads");
        loc.setRoot("/tmp");
        loc.setUploadPath("/tmp");
        loc.addMethod("GET");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/file.txt", "data"), loc, server);
        check("POST methode non autorisee: 405", res.status_code == 405);
    }

    // ── CAS 7 : POST ecrase un fichier existant → 201 ───────────────
    {
        std::ofstream f("/tmp/existing.txt");
        f << "ancien contenu";
        f.close();

        LocationConfig loc = makeLoc("/tmp");
        MethodHandler handler;
        handler.handle(makeReq("POST", "/uploads/existing.txt", "nouveau contenu"), loc, server);
        check("POST ecrase fichier existant: contenu mis a jour",
              readFile("/tmp/existing.txt") == "nouveau contenu");
    }

    // ── CAS 8 : POST contenu binaire → 201 ──────────────────────────
    {
        std::string binary(4, '\0');
        binary[0] = '\x89'; binary[1] = 'P'; binary[2] = 'N'; binary[3] = 'G';

        LocationConfig loc = makeLoc("/tmp");
        MethodHandler handler;
        HttpResponse res = handler.handle(makeReq("POST", "/uploads/img.png", binary), loc, server);
        check("POST contenu binaire: 201", res.status_code == 201);
        check("POST contenu binaire: fichier correct", readFile("/tmp/img.png") == binary);
    }

    std::cout << std::endl << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
