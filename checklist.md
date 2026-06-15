# WebServ — Checklist du projet

Suivi en grandes lignes. Chaque module définit **ce qu'il doit livrer**, pas comment l'implémenter.
Les choix d'implémentation appartiennent à la personne qui prend en charge le module.

Pour les exigences détaillées du sujet et la grille d'évaluation, voir `docs/subject.md` et `docs/evaluation.md`.

---

## Coordination

| Module | Responsable | Statut |
|--------|-------------|--------|
| Config Parsing | — | 🔴 Pas commencé |
| Core Server (boucle d'événements) | — | 🔴 Pas commencé |
| HTTP Layer | — | 🔴 Pas commencé |

Les **points d'intégration** sont les moments où les trois modules doivent s'aligner.
Les planifier ensemble avant de passer à la phase suivante.

---

## Phase 0 — Setup partagé (toute l'équipe, avant de se répartir)

- [ ] Le Makefile compile proprement avec `-Wall -Wextra -Werror -std=c++98`, sans relink inutile
- [ ] `.gitignore` en place
- [ ] Définir le **contrat d'interface** entre Core Server et HTTP Layer :
  quelles données le serveur passe à la couche HTTP, et ce qu'il reçoit en retour
  *(à définir ensemble — c'est la seule frontière d'API partagée)*
- [ ] `README.md` avec la première ligne obligatoire

---

## Phase 1 — Module : Config Parsing

**Livraison :** une représentation parsée et validée du fichier de configuration, interrogeable par le reste du programme.

- [ ] Lire un fichier de config passé en argument, ou utiliser un chemin par défaut
- [ ] Supporter plusieurs blocs server avec des paires host:port différentes
- [ ] Supporter la configuration par route (méthodes, répertoire racine, index, listing, redirections, upload, CGI)
- [ ] Valider la config — rejeter les configurations invalides avec un message clair, sans crash
- [ ] host:port dupliqué → refus de démarrer
- [ ] Fournir des fichiers de config représentatifs pour les tests (valide, multi-port, CGI, erreurs, invalide)

---

## Phase 2 — Module : Core Server

**Livraison :** une boucle d'événements non-bloquante qui accepte les connexions, lit les requêtes et écrit les réponses — tout en passant par un seul appel à poll().

- [ ] Un seul `poll()` (ou équivalent) pour tous les I/O : sockets d'écoute, sockets clients, pipes CGI
- [ ] `poll()` surveille la lecture ET l'écriture simultanément
- [ ] Aucun read/write sur un socket sans passer par poll() au préalable
- [ ] Pas d'utilisation d'`errno` après read/recv/write/send pour adapter le comportement
- [ ] Accepter les nouvelles connexions clients
- [ ] Lire les données entrantes et détecter quand une requête est complète
- [ ] Déléguer les requêtes complètes à la couche HTTP (via l'interface définie)
- [ ] Écrire les réponses aux clients en gérant les écritures partielles
- [ ] Détecter et nettoyer les clients déconnectés
- [ ] Timeout sur les connexions inactives
- [ ] Arrêt propre sur SIGINT

---

## Phase 3 — Module : HTTP Layer

**Livraison :** à partir d'un buffer de requête brut et d'une config serveur, retourner une réponse HTTP complète.

- [ ] Parser la ligne de requête, les headers et le body
- [ ] Gérer `Transfer-Encoding: chunked` (dé-chunker avant traitement)
- [ ] Router la requête vers la bonne location par correspondance du préfixe le plus long
- [ ] Retourner 405 si la méthode n'est pas autorisée pour la route
- [ ] Gérer les redirections HTTP
- [ ] GET : servir les fichiers statiques, l'index d'un répertoire, ou le listing de répertoire
- [ ] POST : appliquer la limite de taille du body, gérer l'upload de fichiers
- [ ] DELETE : supprimer le fichier cible
- [ ] Requêtes inconnues/malformées → réponse d'erreur appropriée, sans crash
- [ ] Codes de statut HTTP corrects partout
- [ ] Pages d'erreur par défaut pour les codes courants (400, 403, 404, 405, 413, 500)
- [ ] Pages d'erreur custom depuis la config si disponibles
- [ ] CGI : exécuter les scripts, passer le bon environnement, gérer GET et POST, gérer les timeouts, pas de crash sur les scripts défaillants
- [ ] CGI uniquement via `fork()` — aucune autre utilisation de `fork()`

---

## Point d'intégration 1 — Après les phases 1–3

Se réunir en équipe et vérifier :

- [ ] Le serveur démarre avec un fichier de config et écoute sur les ports attendus
- [ ] Une requête GET depuis un navigateur retourne une réponse valide
- [ ] Le multi-port fonctionne — contenu différent par port
- [ ] Le CGI s'exécute correctement de bout en bout
- [ ] Aucun push direct sur main — tout passe par PR + skill pr-review

---

## Phase 4 — Robustesse & conformité

- [ ] Aucun crash en toutes circonstances (requêtes malformées, headers énormes, déconnexions brutales)
- [ ] Stress test avec siege — disponibilité ≥ 99,5% sur un GET simple
- [ ] Pas de fuite mémoire sous charge soutenue
- [ ] Pas de connexions pendantes
- [ ] Comportement comparé avec NGINX sur les cas clés (headers, réponses d'erreur)
- [ ] Testé avec un vrai navigateur (vérifier l'onglet réseau)
- [ ] `fcntl()` utilisé uniquement avec `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` (macOS)
- [ ] Pas de librairie externe, pas de C++11+

---

## Phase 5 — Documentation

- [ ] README complet (voir `docs/subject.md` §V pour les sections obligatoires)
- [ ] Fichiers de config et pages par défaut inclus pour la démo d'évaluation
- [ ] Tous les scénarios d'évaluation peuvent être démontrés (voir `docs/evaluation.md`)

---

## Bonus (uniquement si le mandatory est entièrement complet et stable)

- [ ] Gestion des cookies et des sessions
- [ ] Plusieurs types de CGI

---

## Sync hebdomadaire

| Semaine | Jalon |
|---------|-------|
| — | Phases 0–1 terminées, contrat d'interface défini |
| — | Phases 2–3 terminées, point d'intégration 1 validé |
| — | Phase 4 terminée, stress test passant |
| — | Phase 5 terminée, prêt pour l'évaluation |
