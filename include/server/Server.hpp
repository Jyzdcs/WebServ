#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include "Socket.hpp"
#include "Client.hpp"
#include "PollManger.hpp"
#include "../../config/default.conf"

/*
** Server
** ------
** Goal: Chef d'orchestre. Possède tous les Socket d'écoute, tous les
**       Client connectés, et LE PollManager unique. Contient la boucle
**       principale run().
**
** C'est la SEULE classe qui doit connaître à la fois :
**   - les sockets d'écoute (Socket)
**   - les clients connectés (Client)
**   - le poll() (PollManager)
**   - et qui DOIT appeler l'HTTP Layer (via un point d'entrée unique,
**     ex: HttpDispatcher::handle(client, configs) — à définir avec
**     l'équipe HTTP Layer comme LE point de contact entre les 2 modules)
**
** Tout le reste (Socket, Client, PollManager) ne sait rien des autres
** classes du Core Server — c'est Server qui les fait collaborer.
*/
class Server
{
    public:
        Server();
        ~Server();

        /*
        ** Goal: Enregistrer une config serveur (= un ou plusieurs
        ** host:port à écouter). Appelé depuis main() après le parsing
        ** de la config, AVANT run().
        ** Crée en interne le(s) Socket correspondant(s) et les ajoute
        ** au PollManager (POLLIN, car on attend des connexions).
        */
        void addServerConfig(const ServerConfig& config);

        /*
        ** Goal: LA boucle principale. Ne retourne jamais sauf signal
        ** d'arrêt (ex: SIGINT proprement géré) ou erreur fatale.
        **
        ** Pseudo-code de la boucle (1 tour = 1 itération) :
        **
        **   while (running) {
        **       poll_manager.poll(TIMEOUT_MS);
        **
        **       for each fd in poll_manager.getFds():
        **           if fd == un des listening sockets:
        **               if isReadable(fd): handleNewConnection(fd);
        **           else:
        **               if hasError(fd): closeClient(fd);
        **               else if isReadable(fd): handleClientRead(fd);
        **               else if isWritable(fd): handleClientWrite(fd);
        **
        **       checkTimeouts(); // déconnecte les clients inactifs
        **   }
        **
        ** Règle absolue : TOUT read/write passe par les handlers privés
        ** ci-dessous, jamais d'appel direct dans run().
        */
        void run();

        /*
        ** Goal: Demande l'arrêt propre de la boucle (ex: appelé depuis
        ** un signal handler SIGINT). Met _running = false.
        ** La boucle actuelle finit son tour puis sort de run().
        */
        void stop();

    private:
        std::vector<Socket*> _listening_sockets;
        std::map<int, Client*> _clients;   // fd -> Client*
        std::map<int, ServerConfig> _configs_by_port; // port -> config (multi-port)
        PollManager _poll_manager;
        bool _running;

        /*
        ** Goal: Un socket d'écoute a un événement POLLIN → accept().
        ** Crée un nouveau Client, l'ajoute à _clients et au PollManager
        ** (POLLIN seulement au début — on n'a rien à écrire encore).
        */
        void handleNewConnection(int listening_fd);

        /*
        ** Goal: Un client a des données à lire.
        ** Appelle client->receiveData().
        ** Si state == CLOSING (déconnexion/erreur) → closeClient(fd).
        ** Si state == PROCESSING (requête complète) → appelle le point
        ** d'entrée HTTP Layer pour obtenir la réponse, puis
        ** client->setWriteBuffer(response) et
        ** poll_manager.updateEvents(fd, POLLIN | POLLOUT).
        */
        void handleClientRead(int fd);

        /*
        ** Goal: Un client est prêt à recevoir des données.
        ** Appelle client->sendData().
        ** Si isResponseFullySent() :
        **   - si keep-alive : updateEvents(fd, POLLIN), state = READING_REQUEST,
        **     vider read_buffer pour la prochaine requête sur ce fd
        **   - sinon : closeClient(fd)
        ** Si state == CLOSING → closeClient(fd).
        */
        void handleClientWrite(int fd);

        /*
        ** Goal: Fermeture propre d'un client.
        ** Fait : close(fd), poll_manager.removeFd(fd),
        ** delete _clients[fd], _clients.erase(fd).
        ** Centraliser ici évite les fuites de fd dispersées dans le code.
        */
        void closeClient(int fd);

        /*
        ** Goal: Parcourt _clients et ferme ceux dont
        ** getLastActivity() dépasse un seuil (ex: 60s sans activité).
        ** Implémente la règle "une requête ne doit jamais hang indéfiniment".
        ** Appelé une fois par tour de boucle, après le traitement des fds.
        */
        void checkTimeouts();

        /*
        ** Goal: Retrouve le ServerConfig correspondant au port sur lequel
        ** le client est connecté. Utilisé pour passer la bonne config
        ** (routes, error pages, max body size) au Router de l'HTTP Layer.
        */
        const ServerConfig& getConfigForClient(const Client* client) const;

        // Non copyable
        Server(const Server& other);
        Server& operator=(const Server& other);
};

#endif