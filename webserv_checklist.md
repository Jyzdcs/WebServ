# Webserv — Checklist complète du projet

> Copier-coller dans Discord. Cocher au fur et à mesure. Chaque grosse partie est assignée à un membre de la team (à adapter).

---

## 🟦 0. Setup du repo (toute la team — à faire en premier, ensemble)

- [ ] Créer le repo Git
- [ ] Créer l'arborescence de dossiers (`include/`, `src/`, `config/`, `www/`, `tests/`)
- [ ] Écrire le `Makefile` (NAME, all, clean, fclean, re)
- [ ] Vérifier que `make` compile un `main.cpp` vide avec `-Wall -Wextra -Werror -std=c++98`
- [ ] Vérifier que `make re` ne fait pas de relink inutile
- [ ] Créer `.gitignore` (`*.o`, le binaire `webserv`, fichiers macOS `.DS_Store`)
- [ ] Créer les branches : `feature/core-server`, `feature/http-layer`, `feature/config-parsing`
- [ ] Définir et écrire le contrat d'interface `HttpDispatcher.hpp` (signature commune entre Core Server et HTTP Layer)
- [ ] Créer `README.md` avec la première ligne italique obligatoire : *This project has been created as part of the 42 curriculum by ...*

---

## 🟩 1. Module Config Parsing

### 1.1 — Structures de données
- [ ] `ServerConfig.hpp` : host, port, server_name, error_pages, client_max_body_size, routes
- [ ] `RouteConfig.hpp` : path, méthodes autorisées, root, index, autoindex (listing), redirection, upload_enabled + upload_dir, cgi_extension + cgi_path

### 1.2 — ConfigParser
- [ ] Ouvrir et lire le fichier de config passé en argument
- [ ] Gérer le cas "pas d'argument" → chemin par défaut (`config/default.conf`)
- [ ] Tokeniser le fichier (gérer commentaires `#`, espaces, retours à la ligne)
- [ ] Parser les blocs `server { ... }`
- [ ] Parser les blocs `location /route { ... }` à l'intérieur d'un server
- [ ] Supporter plusieurs blocs `server` (multi-port)
- [ ] Remplir `ServerConfig` et `RouteConfig` à partir des tokens

### 1.3 — ConfigValidator
- [ ] Vérifier que chaque port est un entier valide (1–65535)
- [ ] Vérifier qu'il n'y a pas deux `server` sur le même host:port
- [ ] Vérifier que les méthodes listées sont parmi GET/POST/DELETE (rejeter le reste ou logguer)
- [ ] Vérifier que les chemins `root` existent sur le disque
- [ ] Vérifier que `client_max_body_size` est un nombre positif
- [ ] Vérifier que les pages d'erreur custom existent si spécifiées
- [ ] Le programme refuse de démarrer (message clair, pas de crash) si la config est invalide

### 1.4 — Fichiers de configuration de test
- [ ] `config/default.conf` — config minimale qui marche out-of-the-box
- [ ] `config/multi_port.conf` — plusieurs server blocks sur ports différents
- [ ] `config/cgi.conf` — config avec route CGI activée
- [ ] `config/errors.conf` — config avec pages d'erreur custom
- [ ] `config/invalid.conf` — config volontairement cassée pour tester la validation

### 1.5 — Tests
- [ ] `tests/test_config.cpp` : parsing simple passe
- [ ] Test : config invalide rejetée sans crash
- [ ] Test : multi-port bien reconnu (2+ ServerConfig en sortie)

---

## 🟪 2. Module Core Server (event loop)

### 2.1 — PollManager
- [ ] `addFd(fd, events)`
- [ ] `removeFd(fd)`
- [ ] `updateEvents(fd, events)`
- [ ] `poll(timeout_ms)` — gérer le retour -1 avec EINTR sans crash
- [ ] `isReadable(fd)`
- [ ] `isWritable(fd)`
- [ ] `hasError(fd)` — détecter POLLHUP / POLLERR / POLLNVAL
- [ ] `getFds()`
- [ ] Test isolé : ajouter `stdin` (fd 0), taper au clavier, vérifier `isReadable(0)`

### 2.2 — Socket
- [ ] Constructeur : `socket()` 
- [ ] `setsockopt(SO_REUSEADDR)`
- [ ] Passage en non-bloquant (`fcntl` + `O_NONBLOCK`)
- [ ] `bind()` sur host:port
- [ ] `listen()` avec backlog
- [ ] Gestion d'erreur sur chaque étape (pas de crash, message clair)
- [ ] `getFd()`
- [ ] `getPort()`
- [ ] `acceptConnection()` — retourne le fd du nouveau client, le passe en non-bloquant
- [ ] Test : `./webserv` démarre et écoute (vérifiable avec `lsof -i :8080` ou `netstat`)

### 2.3 — Client
- [ ] Constructeur (fd, server_port)
- [ ] `getFd()`
- [ ] `receiveData()` — read() + accumulation dans read_buffer
- [ ] Gérer read() qui retourne 0 → EOF → state = CLOSING
- [ ] Gérer read() qui retourne -1 → erreur → state = CLOSING (sans check errno pour la logique)
- [ ] `isRequestComplete()` — détecte `\r\n\r\n` + vérifie Content-Length si présent
- [ ] `sendData()` — write() avec gestion du **partial write** (offset)
- [ ] `isResponseFullySent()`
- [ ] `getState()` / `setState()`
- [ ] `getReadBuffer()` / `setWriteBuffer()`
- [ ] `getServerPort()`
- [ ] `getLastActivity()` / `updateLastActivity()`
- [ ] Destructeur (pas de close() ici si géré par Server::closeClient)

### 2.4 — Server (boucle principale)
- [ ] `addServerConfig()` — crée les `Socket` correspondants, les ajoute au PollManager (POLLIN)
- [ ] `run()` : boucle infinie avec `poll_manager.poll(timeout)`
- [ ] Distinguer "fd = socket d'écoute" vs "fd = client existant"
- [ ] `handleNewConnection()` — accept() + création Client + ajout PollManager
- [ ] `handleClientRead()` — receiveData() + appel HTTP Layer si requête complète
- [ ] `handleClientWrite()` — sendData() + gestion fin d'envoi (keep-alive ou close)
- [ ] `closeClient()` — close(fd) + removeFd + delete + erase (centralisé, aucune fuite de fd)
- [ ] `checkTimeouts()` — fermer les clients inactifs depuis > X secondes
- [ ] `getConfigForClient()` — retrouver le bon ServerConfig selon le port
- [ ] `stop()` — arrêt propre (SIGINT géré sans crash)
- [ ] **Vérifier : aucun read/write n'est appelé sans passage par poll() avant**
- [ ] **Vérifier : un seul poll() pour tout le programme (listen + clients + CGI pipes)**

### 2.5 — Tests Core Server
- [ ] Test manuel telnet : connexion acceptée, log affiché
- [ ] Test manuel telnet : déconnexion détectée proprement (pas de crash)
- [ ] Test manuel telnet : requête envoyée en plusieurs morceaux → bien accumulée
- [ ] Test : 2 clients connectés simultanément, chacun reçoit sa propre réponse
- [ ] Test : client qui n'envoie rien pendant longtemps → timeout déclenché et connexion fermée
- [ ] Test : grosse requête (plusieurs Mo) → pas de crash, lue en plusieurs read()

---

## 🟧 3. Module HTTP Layer

### 3.1 — Structures
- [ ] `HttpRequest.hpp` : method, path, version, headers (map), body, query_string
- [ ] `HttpResponse.hpp` : status_code, status_message, headers (map), body
- [ ] `HttpStatus.hpp` : enum/constantes pour tous les codes utilisés (200, 201, 204, 301, 400, 403, 404, 405, 411, 413, 500, 501, 505...)

### 3.2 — RequestParser
- [ ] Parser la status line (méthode, path, version)
- [ ] Parser les headers (map clé/valeur, gérer casse insensible)
- [ ] Détecter présence de `\r\n\r\n` (fin des headers)
- [ ] Extraire le `Content-Length` et vérifier la complétude du body
- [ ] Gérer `Transfer-Encoding: chunked` — un-chunker le body avant de le passer au reste
- [ ] Extraire `query_string` depuis le path (`?key=value`)
- [ ] Rejeter une requête malformée → 400 Bad Request (sans crash)
- [ ] Rejeter une version HTTP non supportée → 505 (ou traiter en 1.0/1.1 selon stratégie)
- [ ] `isComplete(raw_buffer)` — fonction réutilisable par `Client::isRequestComplete()`

### 3.3 — Router
- [ ] Recevoir `HttpRequest.path` + liste de `RouteConfig`
- [ ] Matcher le préfixe le plus long (`/kapouet/pouic` → route `/kapouet` si elle existe)
- [ ] Retourner 404 si aucune route ne matche (ou fallback sur `/`)
- [ ] Vérifier que la méthode HTTP est autorisée pour cette route → sinon 405
- [ ] Résoudre le chemin disque final : `route.root + (path - route.prefix)`
- [ ] Gérer la redirection HTTP (301/302) si configurée sur la route

### 3.4 — MethodHandler — GET
- [ ] Si le chemin est un fichier → le lire et le servir
- [ ] Si le chemin est un dossier ET qu'un `index` est configuré → servir ce fichier
- [ ] Si le chemin est un dossier ET `autoindex on` → générer un listing HTML du dossier
- [ ] Si le chemin est un dossier sans index ni autoindex → 403 ou 404 (à décider)
- [ ] Fichier introuvable → 404 avec page d'erreur (custom ou défaut)
- [ ] Permissions insuffisantes → 403

### 3.5 — MethodHandler — POST
- [ ] Vérifier `Content-Length` <= `client_max_body_size` → sinon 413 Payload Too Large
- [ ] Si route a `upload_enabled` → écrire le body dans `upload_dir` avec un nom de fichier unique
- [ ] Répondre 200/201 avec confirmation
- [ ] Si POST sans upload configuré → comportement à définir (CGI ou 403)

### 3.6 — MethodHandler — DELETE
- [ ] Vérifier que le fichier existe → sinon 404
- [ ] Vérifier les permissions → sinon 403
- [ ] Supprimer le fichier (`remove()` / `unlink`)
- [ ] Répondre 200 ou 204 selon convention choisie

### 3.7 — StaticFileServer
- [ ] `open()` + `read()` du fichier (rappel : pas besoin de poll() pour les fichiers disque)
- [ ] Détection du MIME type selon l'extension (au moins : html, css, js, png, jpg, gif, txt, pdf)
- [ ] Construire `Content-Type` et `Content-Length` corrects

### 3.8 — ResponseBuilder
- [ ] Construire la status line (`HTTP/1.1 <code> <message>`)
- [ ] Ajouter les headers obligatoires : `Content-Type`, `Content-Length`, `Connection`
- [ ] Gérer `Connection: keep-alive` vs `Connection: close` selon la requête
- [ ] Assembler headers + `\r\n\r\n` + body en un seul `std::string`
- [ ] Fonction pour générer une réponse d'erreur (status code → page HTML correspondante)

### 3.9 — Pages d'erreur par défaut
- [ ] `www/errors/400.html`
- [ ] `www/errors/403.html`
- [ ] `www/errors/404.html`
- [ ] `www/errors/405.html`
- [ ] `www/errors/413.html`
- [ ] `www/errors/500.html`
- [ ] Utiliser la page custom de la config si elle existe, sinon la page par défaut

### 3.10 — CgiHandler
- [ ] Détecter si la route demandée correspond à une extension CGI (ex: `.py`, `.php`)
- [ ] Créer `pipe_in` et `pipe_out`
- [ ] `fork()`
- [ ] **Processus enfant** : `dup2` stdin/stdout sur les pipes, fermer les fds inutiles
- [ ] **Processus enfant** : construire le tableau d'environnement CGI (REQUEST_METHOD, PATH_INFO, CONTENT_LENGTH, QUERY_STRING, SCRIPT_NAME, SERVER_PROTOCOL, etc.)
- [ ] **Processus enfant** : `chdir()` dans le bon dossier pour les chemins relatifs du script
- [ ] **Processus enfant** : `execve()` + gestion du cas où execve échoue (exit 127)
- [ ] **Processus parent** : fermer les fds inutiles, écrire le body sur `pipe_in`, fermer `pipe_in` (EOF)
- [ ] **Processus parent** : ajouter `pipe_out` au PollManager (non-bloquant)
- [ ] Lire la sortie du CGI via poll() (pas de read direct)
- [ ] Gérer le cas où le CGI ne renvoie pas de `Content-Length` → utiliser EOF comme fin
- [ ] Un-chunker une requête chunked AVANT de l'envoyer au CGI
- [ ] `waitpid()` pour éviter les processus zombies
- [ ] Timeout sur le CGI (si le script ne répond jamais → tuer le process avec `kill()`)
- [ ] Tester avec au moins un langage (Python ou PHP-CGI)

### 3.11 — HttpDispatcher (point d'entrée unique)
- [ ] Définir la signature exacte (input: read_buffer + ServerConfig, output: std::string réponse complète)
- [ ] Orchestrer : RequestParser → Router → MethodHandler → ResponseBuilder
- [ ] Gérer le cas CGI (délégation à CgiHandler, retour asynchrone via PollManager)
- [ ] Capturer toute exception interne → réponse 500 (jamais de crash qui remonte au Server)

### 3.12 — Tests HTTP Layer
- [ ] `tests/test_request_parser.cpp` : GET simple, POST avec body, requête incomplète
- [ ] `tests/test_router.cpp` : matching de préfixes, méthode non autorisée → 405
- [ ] Test manuel : GET sur fichier existant → 200 + bon contenu
- [ ] Test manuel : GET sur fichier inexistant → 404 + page d'erreur
- [ ] Test manuel : POST upload → fichier bien créé sur le disque
- [ ] Test manuel : DELETE sur fichier existant → fichier supprimé
- [ ] Test manuel : requête sur route CGI → sortie du script bien renvoyée

---

## 🟨 4. Intégration & conformité au sujet

- [ ] Le serveur démarre avec `./webserv [config]` ET avec `./webserv` (config par défaut)
- [ ] Le serveur écoute sur plusieurs ports simultanément (vérifié avec 2 configs différentes)
- [ ] Aucun `read`/`recv`/`write`/`send` sur socket sans passage par poll() — audit du code fait
- [ ] Aucune vérification d'`errno` après read/write pour adapter le comportement — audit du code fait
- [ ] Un seul `poll()` (ou équivalent) dans tout le programme — audit fait
- [ ] `poll()` surveille lecture ET écriture simultanément selon les besoins
- [ ] Aucun `fork()` utilisé hors CGI
- [ ] Pas d'`execve` d'un autre serveur web
- [ ] Programme ne crash jamais (testé avec requêtes malformées, headers énormes, etc.)
- [ ] Compatible avec un navigateur standard (Chrome/Firefox) — testé manuellement
- [ ] Comparaison de comportement avec NGINX sur au moins 3-4 cas (headers, erreurs)

---

## 🟥 5. Tests de robustesse / stress

- [ ] Tester avec `telnet` : requêtes envoyées caractère par caractère
- [ ] Tester avec `curl` : GET, POST, DELETE, headers custom
- [ ] Tester avec navigateur : navigation sur le site statique complet (liens, images, CSS)
- [ ] Stress test avec `wrk` (`wrk -t4 -c100 -d30s http://localhost:8080/`)
- [ ] Stress test avec un script Python/Go maison (connexions simultanées, déconnexions brutales)
- [ ] Test : couper la connexion en plein milieu d'un POST → pas de crash, ressources libérées
- [ ] Test : envoyer des requêtes invalides/garbage → 400, pas de crash
- [ ] Test : laisser le serveur tourner 10+ minutes sous charge → pas de fuite mémoire (vérifier avec `leaks` sur macOS ou `valgrind` sur Linux)
- [ ] Vérifier qu'aucun fd ne fuit (`lsof -p <pid>` avant/après une session de stress)
- [ ] Test : plusieurs requêtes keep-alive sur la même connexion

---

## 🟫 6. README & documentation

- [ ] Première ligne italique : *This project has been created as part of the 42 curriculum by ...*
- [ ] Section "Description" (objectif, vue d'ensemble)
- [ ] Section "Instructions" (compilation, lancement, arguments)
- [ ] Section "Resources" (RFC, docs, tutos utilisés + description de l'usage de l'IA, pour quelles tâches)
- [ ] Section "Feature list" (méthodes supportées, CGI supportés, etc.)
- [ ] Section "Configuration" (comment écrire un fichier .conf, exemples)
- [ ] README entièrement en anglais
- [ ] Fichiers de config et fichiers par défaut fournis dans le repo pour la démo

---

## 🟦 7. Bonus (uniquement si tout le mandatory est ✅ et stable)

- [ ] Support des cookies (set + lecture)
- [ ] Gestion de sessions (exemple simple fonctionnel)
- [ ] Support d'un deuxième langage CGI (ex: PHP en plus de Python)

---

## 📊 Suivi d'avancement (à mettre à jour chaque semaine)

| Module | % complété | Bloquants actuels |
|---|---|---|
| Config Parsing | __% | |
| Core Server | __% | |
| HTTP Layer | __% | |
| Tests & stress | __% | |
| README | __% | |
