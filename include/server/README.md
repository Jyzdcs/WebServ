## Comment lire ces fichiers

**PollManager** — la brique la plus basse. Encapsule `poll()` et le
tableau de `pollfd`. Tu codes ça en premier, et tu peux le tester tout
seul avec un petit `main()` qui ajoute `stdin` (fd 0) et vérifie que
`isReadable(0)` devient vrai quand tu tapes au clavier.

**Socket** — un wrapper sur `socket`/`bind`/`listen`/`accept`. Une
instance par port à écouter. Le commentaire détaille l'ordre exact des
syscalls et pourquoi (notamment `SO_REUSEADDR` et `O_NONBLOCK`, qui sont
des pièges classiques si oubliés).

**Client** — c'est le cœur de ton travail. C'est l'objet qui porte
l'état d'une connexion. J'ai volontairement mis `receiveData()` et
`sendData()` dans cette classe (et pas dans `Server`) pour que la
logique de "partial read/write" et de buffers soit localisée à un seul
endroit — c'est là que tu vas passer le plus de temps et où les edge
cases (EOF, partial write, requête incomplète) se concentrent.

**Server** — orchestrateur. La boucle `run()` est décrite en
pseudo-code dans le commentaire — c'est le squelette que tu dois
remplir. Les 4 handlers privés (`handleNewConnection`,
`handleClientRead`, `handleClientWrite`, `closeClient`) sont les seuls
endroits où tu touches aux fds.

## Core server order

1. **`src/server/PollManager.cpp`**  
   Implémente les 8 méthodes.  
   Teste avec `tests/test_poll_manager.cpp`  
   (ajoute `stdin`, vérifie `isReadable(0)`).

2. **`src/server/Socket.cpp`**  
   Implémente le constructeur  
   (les 5 étapes du commentaire)  
   et `acceptConnection()`.

3. **`src/server/Client.cpp`**  
   `receiveData()` et `sendData()` sont les plus importants.  
   Commence par une version simple,  
   ajoute le _partial write_ ensuite.

4. **`src/server/Server.cpp`**  
   Remplis `run()` en suivant le pseudo‑code du header.
