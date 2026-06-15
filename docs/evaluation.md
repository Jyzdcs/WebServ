# WebServ — Grille d'évaluation (Scale)

> 2 students to evaluate per team

---

## Flags disponibles

`Ok` · `Outstanding project` · `Empty work` · `Incomplete work` · `Invalid compilation` · `Norm` · `Cheat` · `Crash` · `Leaks` · `Forbidden function` · `Concerning situations`

---

## Guidelines évaluateur

- Seul le contenu du dépôt Git est noté
- Vérifier que le `git clone` est fait dans un dossier vide
- Aucune triche tolérée
- Segfault / erreur mémoire / terminaison inattendue → note 0, utiliser le flag approprié
- Vérifier les fuites mémoire (heap doit être libéré avant la fin d'exécution)

---

## MANDATORY PART

### 1. Check the code and ask questions

#### Configuration & théorie

- [ ] Lancer NGINX avec le tester home-made pour comparaison
- [ ] Questions sur les bases d'un serveur HTTP (le groupe doit pouvoir répondre)
- [ ] Questions sur le rôle de l'I/O multiplexing
- [ ] Questions sur le nombre de fois où `select()` (ou équivalent) est appelé

#### Vérification critique du code — **note 0 si faux**

- [ ] Il n'y a **qu'un seul `select()`/`poll()`/`epoll()`/`kqueue()`** dans la boucle principale
- [ ] Ce `poll()` vérifie les fd pour **read ET write en même temps**
- [ ] La valeur de retour de `read`/`recv`/`write`/`send` est **correctement vérifiée**
- [ ] **`errno` n'est PAS vérifié** après read/recv/write/send → si oui : invalid compilation flag

> ⚠️ Si un seul de ces points est manquant ou incorrect → **note 0**

---

### 2. Configuration

Vérifier dans le fichier de configuration :

- [ ] **Plusieurs serveurs avec différents ports** fonctionnent
- [ ] **Plusieurs serveurs avec différents hostnames** (ex: `curl --resolve test1.com:80:127.0.0.1/`)
- [ ] **Limit du body client** fonctionne (ex: `curl -X POST --data "BODY TROP GRAND"` → erreur attendue)
- [ ] **Routes vers différents répertoires** configurables
- [ ] **Fichier par défaut** si l'URL pointe sur un répertoire
- [ ] **Liste des méthodes acceptées** par route (ex: tenter DELETE avec/sans permission)

> Statut HTTP incorrect → pas de point accordé

---

### 3. Basic checks

Avec `wget`, `curl`, ou fichiers préparés :

- [ ] **GET** request fonctionne
- [ ] **POST** request fonctionne
- [ ] **DELETE** request fonctionne
- [ ] Requêtes **inconnues/malformées** → pas de crash, réponse appropriée
- [ ] **Upload d'un fichier** puis récupération
- [ ] Codes de statut HTTP corrects pour chaque test

---

### 4. Check CGI

- [ ] CGI fonctionne correctement avec le serveur
- [ ] CGI lancé dans le **bon répertoire** (accès fichiers par chemin relatif)
- [ ] CGI testé avec **GET**
- [ ] CGI testé avec **POST**
- [ ] CGI avec des **fichiers contenant des erreurs** → serveur gère proprement, pas de crash
- [ ] Système de gestion des erreurs visible en cas de problème CGI

---

### 5. Check with a browser

Utiliser le navigateur de référence du groupe (ouvrir l'onglet Network) :

- [ ] Request headers et response headers visibles et corrects
- [ ] **Site web statique** servi correctement
- [ ] **URL incorrecte** → réponse appropriée (pas de crash)
- [ ] **Listing de répertoire** (si activé)
- [ ] **URL redirigée** fonctionne

---

### 6. Port issues

- [ ] Configuration avec **plusieurs ports** → bon site retourné pour chaque port (navigateur)
- [ ] Configuration avec **le même port deux fois** → serveur **refuse de démarrer** avec une erreur claire
  - Si une config invalide est présente, le serveur ne doit pas démarrer du tout

---

### 7. Siege & stress test

- [ ] Stress test avec **Siege** (`siege -b`)
- [ ] **Disponibilité ≥ 99.5%** pour un GET simple sur une page vide
- [ ] **Pas de fuite mémoire** (mémoire ne monte pas indéfiniment pendant le test)
- [ ] **Pas de connexions pendantes** (hanging connections)
- [ ] Serveur utilisable **indéfiniment** sans problème (siege -b prolongé)

---

## BONUS PART

> Évalué **uniquement si la partie mandatory est PARFAITE**

- [ ] Système de **cookies et session** fonctionnel
- [ ] **Plusieurs types de CGI** supportés

---

## Points de vigilance pour nos PRs

Avant de soumettre une PR, vérifier :

1. **Un seul poll()** dans la boucle principale, jamais en dehors
2. **Jamais de read/write sans poll()** au préalable
3. **errno jamais checké** après read/write/recv/send
4. **fork() uniquement pour CGI**
5. **fcntl() uniquement avec** `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` (macOS)
6. **Pas de external libraries**
7. **C++98 strict** — pas de C++11+
8. Makefile sans relinking inutile
9. Pas de fuite mémoire
10. Toute requête obtient une réponse (pas de hang)
