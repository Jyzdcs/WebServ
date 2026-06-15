#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <vector>
#include <poll.h>

/*
** PollManager
** -----------
** Goal: Centraliser la liste des file descriptors surveillés par poll()
**       et exposer une interface simple pour les ajouter/retirer/interroger.
**
** Cette classe NE SAIT PAS ce qu'est un client HTTP, un CGI ou un socket
** d'écoute. Elle manipule uniquement des entiers (fd) et des flags
** (POLLIN, POLLOUT). C'est la frontière la plus basse du serveur.
**
** Règle absolue : un seul poll() pour tout le programme. Cette classe
** EST ce poll() unique.
*/
class PollManager
{
	private:
		std::vector<struct pollfd> _fds;

		/*
		** Goal: Trouver l'index dans _fds correspondant à un fd donné.
		** Retourne -1 si non trouvé.
		** Interne uniquement — utilisé par updateEvents/removeFd/isReadable...
		*/
		int findIndex(int fd) const;

		// Non copyable : PollManager possède une ressource unique (la liste fds)
		PollManager(const PollManager& other);
		PollManager& operator=(const PollManager& other);

	public:
		PollManager();
		~PollManager();

		/*
		** Goal: Ajouter un fd à la liste surveillée par poll().
		** events = POLLIN, POLLOUT, ou POLLIN | POLLOUT
		** Appelé quand : un nouveau client se connecte (accept),
		**                 un CGI démarre (pipe), etc.
		*/
		void addFd(int fd, short events);

		/*
		** Goal: Retirer un fd de la liste.
		** Appelé quand : un client se déconnecte, un CGI a fini,
		**                 une erreur fatale sur ce fd.
		** Important : NE FAIT PAS le close() du fd. C'est la responsabilité
		** de l'appelant (Server/Client) de fermer le fd lui-même.
		*/
		void removeFd(int fd);

		/*
		** Goal: Modifier les events surveillés pour un fd existant.
		** Cas d'usage typique : un Client a fini de lire (POLLIN suffit)
		** puis a une réponse à envoyer (passer à POLLIN | POLLOUT).
		** Si fd non trouvé : ne fait rien (no-op silencieux).
		*/
		void updateEvents(int fd, short events);

		/*
		** Goal: Appeler poll() sur la liste actuelle.
		** timeout en millisecondes (ex: 1000 = 1s). Utiliser un timeout
		** fini (pas -1) pour pouvoir détecter les clients inactifs
		** (cf. gestion des timeouts côté Server).
		** Retourne : nombre de fds prêts (>= 0), ou -1 en cas d'erreur
		** (errno EINTR à gérer : ne pas crasher, juste re-looper).
		*/
		int poll(int timeout_ms);

		/*
		** Goal: Après un poll() réussi, indique si ce fd est prêt en lecture.
		** Utilisé par Server pour décider d'appeler read() sur ce fd.
		*/
		bool isReadable(int fd) const;

		/*
		** Goal: Idem mais pour l'écriture (POLLOUT déclenché).
		** Utilisé par Server pour décider d'appeler write() sur ce fd.
		*/
		bool isWritable(int fd) const;

		/*
		** Goal: Détecte une erreur/hangup sur ce fd (POLLHUP, POLLERR, POLLNVAL).
		** Si true, le fd doit être nettoyé (close + removeFd) sans tenter
		** read/write supplémentaire.
		*/
		bool hasError(int fd) const;

		/*
		** Goal: Exposer la liste complète des fds actuellement surveillés.
		** Utile pour itérer dans Server::run() après chaque poll().
		*/
		const std::vector<struct pollfd>& getFds() const;
};

#endif