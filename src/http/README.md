# HTTP Layer — Documentation d'intégration

> Ce README est destiné au **Core Server** (Famasito).
> Tu n'as pas besoin de comprendre l'intérieur du HTTP layer — juste comment l'appeler.

---

## API complète : 2 fonctions, 1 struct

```cpp
#include "http/processHttp.hpp"

// Appelle ça pour chaque requête reçue.
ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server);

// Appelle ça quand le pipe CGI est EOF ou timeout.
std::string finishCgi(const std::string& output, pid_t pid, bool timedOut,
                      bool shouldClose, const ServerConfig& server);
```

C'est tout. Le HTTP layer gère le parsing, le routing, les handlers, les CGI, les erreurs.

---

## ProcessResult — la struct de retour

```cpp
struct ProcessResult {
    enum State { COMPLETE, CGI_PENDING };

    State        state;
    std::string  rawResponse;  // valide si COMPLETE — string prête pour send()
    bool         shouldClose;  // fermer la connexion après envoi ?

    // valides si CGI_PENDING :
    int          stdoutFd;     // fd du pipe à ajouter dans poll(POLLIN)
    pid_t        pid;          // pid du script (pour kill si timeout)
    time_t       deadline;     // time(NULL) + timeout — calculé par le HTTP layer
};
```

---

## Cas 1 — Requête normale (statique, upload, delete, redirect...)

```cpp
ProcessResult result = processHttp(client->getReadBuffer(), getConfig(client));

if (result.state == ProcessResult::COMPLETE)
{
    client->setWriteBuffer(result.rawResponse); // string HTTP prête
    client->setShouldClose(result.shouldClose);
    poll_manager.updateEvents(fd, POLLIN | POLLOUT);
}
```

`rawResponse` est déjà formattée, headers inclus. Tu envoies, c'est fini.

---

## Cas 2 — Requête CGI (CGI_PENDING)

Le script Python/PHP tourne dans un processus enfant. Le HTTP layer t'a retourné le fd du pipe.
**Ton rôle : surveiller ce fd dans poll(), accumuler l'output, appeler finishCgi quand c'est fini.**

### Quand processHttp() retourne CGI_PENDING

```cpp
else // CGI_PENDING
{
    // Associer le pipe au client pour retrouver qui attend la réponse
    CgiContext ctx;
    ctx.pid        = result.stdoutFd;   // NON — voir ci-dessous
    ctx.pid        = result.pid;
    ctx.deadline   = result.deadline;
    ctx.clientFd   = fd;
    ctx.shouldClose = result.shouldClose; // à stocker pour finishCgi
    ctx.output     = "";

    cgi_map[result.stdoutFd] = ctx; // map stdoutFd → contexte

    poll_manager.addFd(result.stdoutFd, POLLIN);
}
```

### Dans la boucle poll(), quand stdoutFd est POLLIN

```cpp
if (cgi_map.count(fd))
{
    CgiContext& ctx = cgi_map[fd];
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n > 0)
    {
        ctx.output += std::string(buf, n); // accumule
    }
    else // n == 0 (EOF) ou n < 0 (erreur)
    {
        bool timedOut = (n < 0 && time(NULL) >= ctx.deadline);
        if (timedOut)
            kill(ctx.pid, SIGKILL);

        // Le HTTP layer construit et retourne la réponse finale
        std::string response = finishCgi(ctx.output, ctx.pid, timedOut,
                                          ctx.shouldClose, getConfig(clients[ctx.clientFd]));

        clients[ctx.clientFd]->setWriteBuffer(response);
        clients[ctx.clientFd]->setShouldClose(ctx.shouldClose);
        poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);

        poll_manager.removeFd(fd);
        close(fd);
        cgi_map.erase(fd);
    }
}
```

### Vérifier le timeout dans checkTimeouts()

```cpp
for (auto& [pipeFd, ctx] : cgi_map)
{
    if (time(NULL) >= ctx.deadline)
    {
        kill(ctx.pid, SIGKILL);

        std::string response = finishCgi("", ctx.pid, true,
                                          ctx.shouldClose, getConfig(clients[ctx.clientFd]));
        clients[ctx.clientFd]->setWriteBuffer(response);
        poll_manager.removeFd(pipeFd);
        close(pipeFd);
        cgi_map.erase(pipeFd); // attention : invalide l'itérateur, gérer correctement
        break;
    }
}
```

---

## Connection keep-alive / close

`result.shouldClose` est calculé par le HTTP layer selon :
- Le client est en HTTP/1.0 → `true`
- Le client a envoyé `Connection: close` → `true`
- Sinon → `false` (keep-alive)

Tu n'as pas à lire les headers toi-même.

---

## Ce que le HTTP layer gère pour toi

- Parsing de la requête (méthode, URI, headers, body)
- Validation HTTP/1.1 (Host manquant → 400, version inconnue → 505...)
- Routing vers la bonne location
- Servir les fichiers statiques, autoindex, redirections
- Upload (POST) et suppression (DELETE)
- Lancement du CGI (fork + execve + pipes)
- Headers de réponse (Content-Type, Content-Length, Date, Server...)
- Pages d'erreur custom (si configurées)
- Keep-alive / Connection: close

## Ce que tu dois faire

- Accumuler les bytes du socket jusqu'à `\r\n\r\n` + `Content-Length` bytes
- Appeler `processHttp()` quand la requête est complète
- Si `COMPLETE` : mettre `rawResponse` dans le write buffer
- Si `CGI_PENDING` : tracker `stdoutFd` dans poll() et appeler `finishCgi()` à la fin
- Gérer le timeout CGI en vérifiant `deadline` dans ta boucle

---

## Structure des fichiers

```
src/http/
  processHttp.cpp          ← point d'entrée unique
  parser/                  ← parsing HTTP/1.1
  router/                  ← matching location
  handlers/                ← GET, POST, DELETE, redirect
  cgi/
    start.cpp              ← fork + execve → retourne CGI_PENDING
    finish.cpp             ← finishCgi() — construit la réponse finale
    env.cpp                ← variables d'environnement CGI
    output.cpp             ← parsing de l'output du script
  response/                ← ResponseBuilder (sérialise HttpResponse → string)
  builders/                ← buildHttpOk, buildHttpError, etc.
  utils/                   ← Content-Type, read/write helpers

include/http/
  processHttp.hpp          ← ProcessResult + signatures publiques
```

---

## Headers requis dans Server.cpp

```cpp
#include "../../include/http/processHttp.hpp"
// C'est le seul include nécessaire pour tout utiliser.
```
