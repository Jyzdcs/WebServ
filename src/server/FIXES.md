# Fixes appliqués à `Server.cpp` et `PollManager.cpp`

> **Auteur :** Byron (HTTP layer)
> **Branche :** `core-server`
> **Date :** 2026-07-08

---

## Contexte

Après que Famasito a pushé le premier jet du CGI dans `Server.cpp`, on a testé ensemble.
Tous les endpoints CGI retournaient **000** (connexion fermée sans réponse).
En creusant, on a trouvé **3 bugs critiques**. Ce fichier les explique un par un.

---

## Bug 1 — Le dispatcher appelait `handleClientRead` sur le pipe CGI

### Ce qui se passait

Quand Python finit d'écrire dans le pipe et quitte, le kernel ferme la partie écriture.
Poll détecte alors **POLLHUP** sur le file descriptor du pipe.

Le code original vérifiait `hasError` **avant** `_cgi_map.count` :

```cpp
// AVANT (cassé)
if (_poll_manager.hasError(fd)) {
    handleClientRead(fd);   // ← appelé pour le pipe CGI !
    continue;
}
if (_cgi_map.count(fd) && _poll_manager.isReadable(fd)) {
    handleCgiRead(fd);      // ← jamais atteint si POLLHUP
}
```

`hasError` retourne `true` pour **POLLHUP**. Donc le pipe CGI (fd 9 par exemple)
entrait dans `handleClientRead(9)`, qui loggait `[READ] fd 9 not found in _clients`
et retournait. La réponse CGI n'était **jamais envoyée**.

### Le fix

On vérifie `_cgi_map.count(fd)` en **premier**. Si le fd est un pipe CGI, on appelle
`handleCgiRead` que ce soit POLLIN ou POLLHUP (les deux signaux qu'un pipe peut avoir) :

```cpp
// APRÈS (correct)
if (_cgi_map.count(fd)) {
    if (_poll_manager.isReadable(fd) || _poll_manager.hasError(fd))
        handleCgiRead(fd);
}
else if (_poll_manager.hasError(fd)) {
    handleClientRead(fd);
    continue;
}
else if (_poll_manager.isReadable(fd)) {
    handleClientRead(fd);
} else if (_poll_manager.isWritable(fd)) {
    handleClientWrite(fd);
}
```

### Pourquoi POLLHUP sans POLLIN ?

Quand Python écrit peu de données et quitte vite :
- Poll montre **POLLIN + POLLHUP** → on lit les données, puis n == 0 (EOF)
- Si les données ont déjà été lues et que le child est sorti : poll montre **POLLHUP seulement**

Dans ce deuxième cas, `isReadable` était false → `handleCgiRead` jamais appelé → pipe oublié pour toujours.

De plus, `handleCgiRead` boucle maintenant en interne pour tout lire d'un coup :

```cpp
while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0)  { ctx.output += ...; }   // accumule
    else if (n == 0) { finishCgi(...); return; }   // EOF → réponse
    else {
        if (errno == EAGAIN) return;  // pas de données pour l'instant
        finishCgi(...); return;       // erreur → réponse
    }
}
```

---

## Bug 2 — Boucle `for` avec `getSize()` au lieu de `fds.size()`

### Ce qui se passait

Dans `run()`, on fait un snapshot des fds **avant** la boucle :

```cpp
std::vector<struct pollfd> fds = _poll_manager.getFds();  // copie fixe
for(int i = 0; i < _poll_manager.getSize(); i++) {        // taille live !
    int fd = fds[i].fd;
```

`fds` est une **copie** de taille N. Mais `_poll_manager.getSize()` retourne la taille **actuelle** de `_poll_manager._fds`, qui peut avoir augmenté si `handleClientRead` a ajouté le pipe CGI via `addFd`.

Résultat : quand `getSize()` vaut N+1, la boucle fait `fds[N]` → **accès hors bornes** → undefined behavior silencieux (lecture de mémoire non initialisée, valeur garbage pour `fd`, puis `findIndex(garbage)` bouclait indéfiniment).

### Le fix

On capture la taille du snapshot **une seule fois**, au moment où il est fait :

```cpp
std::vector<struct pollfd> fds = _poll_manager.getFds();
int fdsSize = (int)fds.size();  // ← taille fixe du snapshot
for(int i = 0; i < fdsSize; i++) {
```

Les nouveaux fds ajoutés pendant l'itération seront traités au **prochain** tour de la boucle principale — c'est le comportement correct pour un event loop.

---

## Bug 3 — Crash si le client se déconnecte pendant qu'un CGI tourne

### Ce qui se passait

Scénario : client envoie une requête CGI (ex: `slow.py` qui prend 3s), puis ferme sa connexion avant que le script finisse.

1. Poll détecte **POLLHUP** sur le client fd → `closeClient(clientFd)` → le client est supprimé de `_clients` **et** son fd est retiré de `_fds`
2. Quelques secondes plus tard, `slow.py` finit → `handleCgiRead(pipeFd)` est appelé
3. `handleCgiRead` appelle `_poll_manager.updateEvents(ctx.clientFd, ...)` où `ctx.clientFd` n'est plus dans `_fds`
4. `updateEvents` appelle `findIndex(clientFd)` — fd **introuvable** → la boucle while de `findIndex` sortait des bornes du vecteur → **crash / undefined behavior**
5. Même crash avec `_clients[ctx.clientFd]->setWriteBuffer(...)` → client supprimé → déréférencement de pointeur null

Ce crash a été **reproduit** : le serveur devenait mort (curl retournait 000 sur toutes les requêtes suivantes).

### Le fix — deux parties

**Partie A : `PollManager::findIndex` avec bounds check**

```cpp
// AVANT (dangereux)
int PollManager::findIndex(int fd) const {
    std::size_t i = 0;
    while (_fds[i].fd != fd) { i++; };  // boucle infinie si fd absent !
    return i;
}

// APRÈS (sûr)
int PollManager::findIndex(int fd) const {
    for (std::size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd)
            return (int)i;
    }
    return -1;  // fd non trouvé
}
```

Tous les appelants (`isReadable`, `isWritable`, `hasError`, `updateEvents`, `removeFd`) vérifient maintenant `idx >= 0` avant d'accéder à `_fds[idx]`. Si fd absent → retourne false / ne fait rien.

**Partie B : guard dans `handleCgiRead` et `checkCgiTimeouts`**

Dans `handleCgiRead`, on fait une **copie** du CgiContext (pas une référence) pour éviter un dangling ref si on erase pendant qu'on lit. Et on vérifie que le client existe encore avant d'accéder à `_clients` :

```cpp
void Server::handleCgiRead(int fd) {
    CgiContext ctx = _cgi_map[fd];   // COPIE, pas référence

    // ... lire le pipe ...

    // Nettoyer le pipe en premier
    _poll_manager.removeFd(fd);
    _cgi_map.erase(fd);

    // Puis vérifier si le client existe encore
    if (!_clients.count(ctx.clientFd))
        return;  // client parti → on abandonne proprement, pas de crash

    // Seulement ici on touche au client
    std::string res = finishCgi(...);
    _clients[ctx.clientFd]->setWriteBuffer(res);
    _poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
}
```

Même logique dans `checkCgiTimeouts` : on efface le pipe d'abord, puis on vérifie le client.

---

## Bug 4 — Pas de gestion du timeout CGI (`checkCgiTimeouts` manquant)

### Ce qui se passait

Si un script CGI ne se termine jamais (`infinite.py`), le pipe ne devenait jamais lisible.
Avec `timeout=0` dans poll, la boucle spinnait sans jamais détecter le timeout.

`checkTimeouts` fermait le client après 5 secondes d'inactivité. Mais :
- Le pipe CGI restait orphelin dans `_cgi_map` et `_poll_manager`
- `_clients[ctx.clientFd]` était supprimé → crash au prochain `handleCgiRead`
- Le process Python restait zombie

### Le fix

**1. `checkCgiTimeouts()`** — nouvelle fonction appelée à chaque tour :

```cpp
void Server::checkCgiTimeouts() {
    std::time_t now = std::time(NULL);
    for (it = _cgi_map.begin(); it != _cgi_map.end(); ) {
        if (now >= ctx.deadline) {
            kill(ctx.pid, SIGKILL);          // tue le process CGI
            _poll_manager.removeFd(pipeFd);  // nettoie le pipe
            it = _cgi_map.erase(it);         // nettoie la map
            if (_clients.count(ctx.clientFd)) {
                // envoie un 504 au client s'il est encore là
                finishCgi("", pid, true, ...) → "504 Gateway Timeout"
                _clients[ctx.clientFd]->setWriteBuffer(res);
                _poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
            }
        } else { ++it; }
    }
}
```

**2. `checkTimeouts` ne ferme plus les clients avec CGI actif** :

```cpp
void Server::checkTimeouts() {
    for (each client) {
        // vérifie si ce client a un CGI en cours
        bool hasCgi = false;
        for (each entry in _cgi_map)
            if (entry.clientFd == fd) { hasCgi = true; break; }

        if (!hasCgi && now - c->getLastActivity() > TIMEOUT)
            closeClient(fd);  // seulement si pas de CGI actif
    }
}
```

---

## Résumé des fichiers modifiés

| Fichier | Changement |
|---------|-----------|
| `src/server/PollManager.cpp` | `findIndex` avec bounds check + tous les appelants gèrent `-1` |
| `src/server/Server.cpp` | Dispatcher CGI en premier, loop fix `fds.size()`, `handleCgiRead` avec guard client, `checkCgiTimeouts` ajouté, `checkTimeouts` skip les clients CGI |
| `src/server/Server.hpp` | Déclaration de `checkCgiTimeouts()` |

---

## Comportements après fix

| Scénario | Avant | Après |
|----------|-------|-------|
| GET /cgi-bin/hello.py | 000 | 200 ✅ |
| POST /cgi-bin/echo_post.py | 000 | 200 ✅ |
| GET /cgi-bin/crash.py | 000 | 500 ✅ |
| GET /cgi-bin/nope.py | 000 | 404 ✅ |
| GET /cgi-bin/infinite.py | 000 puis crash | 504 en ~5s ✅ |
| Client coupe pendant CGI | crash serveur | cleanup propre ✅ |
| Serveur après timeout CGI | mort | répond encore ✅ |
