#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include "../config/ConfigMock.hpp"

/*
** Socket
** ------
** Goal: Encapsuler la création d'un socket d'écoute serveur :
**       socket() + setsockopt() + bind() + listen().
**
** Une instance = un socket d'écoute pour une combinaison host:port
** définie dans la config. Si la config définit 3 ports, tu crées
** 3 instances de Socket.
**
** Cette classe NE GÈRE PAS les clients connectés (c'est Client) et
** NE FAIT PAS de poll() elle-même (c'est PollManager). Elle fournit
** juste un fd prêt à être surveillé.
*/
class Socket
{
	private:
		int					_fd;
		int					_port;
		std::string	_host;


	inline std::string intToString(int value);
	
	public:
		/*
		** Goal: Construire et configurer le socket d'écoute.
		** host = "0.0.0.0" ou une IP spécifique (ex: "127.0.0.1")
		** port = ex 8080
		**
		** Fait, dans l'ordre :
		**   1. socket(AF_INET, SOCK_STREAM, 0)
		**   2. setsockopt(SO_REUSEADDR) — évite "Address already in use"
		**      au redémarrage rapide du serveur
		**   3. fcntl(fd, F_SETFL, O_NONBLOCK) — OBLIGATOIRE, le sujet
		**      interdit tout fd bloquant
		**   4. bind() sur host:port
		**   5. listen() avec un backlog raisonnable (ex: 128)
		**
		** En cas d'échec à une étape : throw une exception ou retourne
		** un état d'erreur — à toi de choisir la stratégie d'erreur,
		** mais le programme ne doit JAMAIS crash (cf. règles générales).
		*/
		Socket(ServerConfig serverConf);
		~Socket();

		/*
		** Goal: Retourner le fd du socket d'écoute.
		** Utilisé pour : PollManager::addFd(getFd(), POLLIN)
		** et pour comparer fds[i].fd == getFd() dans Server::run()
		** afin de savoir "est-ce un nouveau client ou un client existant ?"
		*/
		int getFd() const;

		/*
		** Goal: Accepter une nouvelle connexion entrante.
		** Doit être appelé UNIQUEMENT quand PollManager::isReadable(getFd())
		** est true (sinon violation de la règle "jamais sans poll()").
		**
		** Retourne : le fd du nouveau client (>= 0), ou -1 si accept()
		** échoue (ex: EAGAIN car un autre thread/process l'a pris avant —
		** improbable ici mais à gérer sans crash).
		**
		** Le fd retourné doit être mis en O_NONBLOCK avant d'être utilisé.
		*/
		int acceptConnection() const;

		/*
		** Goal: Retourne le port sur lequel ce socket écoute.
		** Utile pour le logging / debug et pour faire correspondre
		** une connexion entrante à son ServerConfig (multi-port).
		*/
		int getPort() const;

		class FailedToBindPort : public std::exception {
			const char *what() const throw();
		};

};

#endif