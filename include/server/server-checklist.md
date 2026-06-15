# Webserv — Checklist complète du projet

---

## 🟪 Module Core Server (event loop)

### 1 — PollManager

- [ ] `addFd(fd, events)`
- [ ] `removeFd(fd)`
- [ ] `updateEvents(fd, events)`
- [ ] `poll(timeout_ms)` — gérer le retour -1 avec EINTR sans crash
- [ ] `isReadable(fd)`
- [ ] `isWritable(fd)`
- [ ] `hasError(fd)` — détecter POLLHUP / POLLERR / POLLNVAL
- [ ] `getFds()`
- [ ] Test isolé : ajouter `stdin` (fd 0), taper au clavier, vérifier `isReadable(0)`

### 2 — Socket

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

### 3 — Client

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

### 4 — Server (boucle principale)

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

### 5 — Tests Core Server

- [ ] Test manuel telnet : connexion acceptée, log affiché
- [ ] Test manuel telnet : déconnexion détectée proprement (pas de crash)
- [ ] Test manuel telnet : requête envoyée en plusieurs morceaux → bien accumulée
- [ ] Test : 2 clients connectés simultanément, chacun reçoit sa propre réponse
- [ ] Test : client qui n'envoie rien pendant longtemps → timeout déclenché et connexion fermée
- [ ] Test : grosse requête (plusieurs Mo) → pas de crash, lue en plusieurs read()
