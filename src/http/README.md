# HTTP Layer — Guide d'intégration

## Utilisation (4 lignes)

```cpp
#include "http/RequestParser.hpp"
#include "http/Router.hpp"
#include "http/MethodHandler.hpp"
#include "http/ResponseBuilder.hpp"

RequestParser   parser;
HttpRequest     req = parser.parse(rawString); // rawString = bytes du socket

if (req.method.empty())
    // requête invalide → envoyer 400

Router          router;
LocationConfig  loc = router.route(req, serverConfig);

MethodHandler   handler;
HttpResponse    res = handler.handle(req, loc, serverConfig);

ResponseBuilder builder;
std::string     raw = builder.build(res);

send(fd, raw.c_str(), raw.size(), 0);
```

> **Important** : accumuler les données du socket jusqu'à trouver `\r\n\r\n` avant d'appeler `parse()`.
> Le calcul de la taille du body (`Content-Length`) et la gestion des bytes incomplets sont à gérer côté core server — le parser suppose que la requête complète est déjà dans le buffer.
> Si `req.method` est vide après `parse()`, c'est au core server de retourner 400 au client — le HTTP layer ne throw pas d'exception.

---

## Ce que chaque classe fait

| Classe | Input | Output |
|---|---|---|
| `RequestParser` | string brute du socket | `HttpRequest` (vide si invalide) |
| `Router` | `HttpRequest` + `ServerConfig` | `LocationConfig` qui matche |
| `MethodHandler` | `HttpRequest` + `LocationConfig` + `ServerConfig` | `HttpResponse` |
| `ResponseBuilder` | `HttpResponse` | string HTTP prête pour `send()` |

---

## Codes retournés par MethodHandler

| Code | Cas |
|---|---|
| 200 | Fichier servi |
| 201 | Upload POST réussi |
| 204 | DELETE réussi |
| 301 | Redirection |
| 400 | Requête invalide (path traversal, body vide...) |
| 403 | Accès interdit |
| 404 | Fichier introuvable |
| 405 | Méthode non autorisée sur cette route |
| 500 | Erreur serveur (ex: upload_store manquant) |
