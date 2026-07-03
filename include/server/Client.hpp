#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

/*
** Client
** ------
** Goal: Représenter l'état d'UNE connexion TCP active. C'est l'objet
**       central qui fait le pont entre Core Server et HTTP Layer.
**
** Cette classe ne fait AUCUN appel système réseau elle-même (pas de
** read/write directement ici idéalement — voir note ci-dessous).
** Elle stocke juste l'état : ce qu'on a reçu, ce qu'on doit envoyer,
** et où on en est dans le cycle de vie de la requête.
**
** C'est l'objet que Server manipule à chaque tour de boucle :
**   - Server fait read() → range les octets dans client.read_buffer
**   - HTTP Layer lit client.read_buffer → remplit client.write_buffer
**   - Server fait write() → vide client.write_buffer
**
** Note: selon ton design, tu peux mettre les appels read()/write()
** directement dans des méthodes de Client (receiveData/sendData) —
** c'est acceptable et même conseillé pour garder Server léger.
** Le prototype ci-dessous part sur cette option.
*/

// État du cycle de vie d'une requête — utile pour savoir
// "où on en est" sans tout recalculer à chaque tour de poll().
enum ClientState
{
    READING_REQUEST,   // on accumule read_buffer, requête pas encore complète
    PROCESSING,        // requête complète, HTTP Layer doit la traiter
    SENDING_RESPONSE,  // write_buffer prêt, on envoie au client
    CGI_RUNNING,       // en attente de la sortie d'un processus CGI
    DONE,              // réponse envoyée — fermer ou attendre keep-alive
    CLOSING            // erreur ou déconnexion — à nettoyer
};

class Client
{
	private:
		int _fd;
		int _server_port;
		ClientState _state;

		std::string _read_buffer;
		std::string _write_buffer;
		size_t _write_offset;

		long _last_activity;

		long getCurrentTimeStamp() const;

	public:
		/*
		** Goal: Initialiser un client à partir du fd retourné par accept().
		** server_port permet de retrouver le bon ServerConfig (multi-port)
		** sans avoir à le chercher à chaque requête.
		*/
		Client(int fd, int server_port);
		~Client();

		// Utils
		void setReadBuffer(std::string);

		// --- Accès au fd ---

		/*
		** Goal: Retourne le fd de ce client.
		** Utilisé par Server pour PollManager::isReadable(getFd()) etc.
		*/
		int getFd() const;

		// --- Lecture (déclenché par PollManager::isReadable) ---

		/*
		** Goal: Appelle read() sur le fd et accumule dans read_buffer.
		** Retourne :
		**   > 0  : nombre d'octets lus (normal, continuer)
		**   == 0 : le client a fermé la connexion (EOF) → state = CLOSING
		**   < 0  : erreur read() → state = CLOSING (ne PAS check errno
		**          pour décider du comportement HTTP, juste fermer proprement)
		**
		** Après chaque appel réussi, vérifie isRequestComplete() et passe
		** state à PROCESSING si c'est le cas.
		*/
		int receiveData();

		/*
		** Goal: Vérifie si read_buffer contient une requête HTTP complète.
		** - Sans body : présence de "\r\n\r\n"
		** - Avec body : présence de "\r\n\r\n" ET
		**   read_buffer.size() - header_end >= Content-Length
		**
		** Cette logique est dupliquée/déléguée avec RequestParser —
		** à toi de choisir : soit Client appelle un helper de
		** RequestParser::isComplete(), soit Client a sa propre version
		** légère. Recommandation : déléguer à RequestParser pour éviter
		** la duplication de logique de parsing.
		*/
		bool isRequestComplete() const;

		// --- Écriture (déclenché par PollManager::isWritable) ---

		/*
		** Goal: Appelle write() avec write_buffer et avance _write_offset.
		** Gère le "partial write" : write() peut envoyer moins d'octets
		** que demandé, il NE FAUT PAS considérer ça comme une erreur.
		**
		** Retourne :
		**   > 0  : octets envoyés (continuer au prochain poll si incomplet)
		**   == 0 : rien envoyé (peu probable si POLLOUT était set, mais
		**          ne pas crasher)
		**   < 0  : erreur → state = CLOSING
		**
		** Quand _write_offset == write_buffer.size() → state = DONE,
		** et PollManager doit repasser ce fd en POLLIN seulement
		** (ou removeFd si pas de keep-alive).
		*/
		int sendData();

		/*
		** Goal: Indique si write_buffer a été entièrement envoyé.
		** Utilisé par Server pour savoir quand basculer
		** updateEvents(fd, POLLIN) (retirer POLLOUT).
		*/
		bool isResponseFullySent() const;

		// --- État et accesseurs ---

		ClientState getState() const;
		void setState(ClientState state);

		/*
		** Goal: Accès en lecture/écriture au buffer de requête brut.
		** L'HTTP Layer (RequestParser) lit ce buffer pour produire
		** un HttpRequest. Après parsing réussi, le buffer est vidé
		** (clear) ou avancé (pour gérer le pipelining keep-alive —
		** optionnel selon votre ambition).
		*/
		std::string& getReadBuffer();

		/*
		** Goal: Définit la réponse complète à envoyer.
		** Appelé par HTTP Layer (ResponseBuilder) une fois la réponse
		** assemblée. Remet _write_offset à 0 et state = SENDING_RESPONSE.
		*/
		void setWriteBuffer(const std::string& response);

		/*
		** Goal: Port du serveur sur lequel ce client s'est connecté.
		** Permet au Router de retrouver le bon ServerConfig
		** (cas multi-port avec configs différentes).
		*/
		int getServerPort() const;

		/*
		** Goal: Timestamp de la dernière activité (read ou write réussi).
		** Utilisé par Server pour détecter et fermer les connexions
		** inactives (anti "hang indéfiniment", règle du sujet).
		*/
		long getLastActivity() const;
		void updateLastActivity();
		
		class ReadFailed : public std::exception {
			const char *what() const throw();
		};
};

#endif