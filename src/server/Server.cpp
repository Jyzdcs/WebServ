#include "../../include/server/Server.hpp"
#include <algorithm>

Server::Server()
	: _running(false), _listening_sockets(), _poll_manager(), _clients(), _configs_by_port(){
};

Server::~Server() {};

void Server::handleNewConnection(Socket *socket) {
	try {
		int newFd = socket->acceptConnection();
		Client* newClient = new Client(fd, socket->getPort());
		_clients.insert(newFd, newClient);
		_poll_manager.addFd(newFd, POLLIN);
	} catch (std::exception& err) {
		std::cout << err << std::endl;
	}
};

void Server::handleClientRead(int fd) {

};

void Server::handleClientWrite(int fd) {

};

void Server::closeClient(int fd) {
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end()) {
		delete it->second;
		_clients.erase(it);
	};
	_poll_manager.removeFd(fd);
};

void Server::checkTimeouts() {

};

void Server::addServerConfig(const ServerConfig& config) {
	_configs_by_port.insert(config.getPort, config);

	Socket *s = new Socket(config);
	_listening_sockets.push_back(s);

	_poll_manager.addFd(s, POLLIN);
};

void Server::run() {
	_running = true;

	while (_running) {
		/*
		** TIMEOUT == 0 : pas d'attente du tout (test l'etat des fd et return direct = non bloquant)
		** TIMEOUT > 0 : durée d'attente maximal en ms
		** TIMEOUT < 0 : attendre indefiniment (bloque jusqua ce qu'au moins un fd ait un evenement)
		*/
		const int timeout = 0;
		int poll_count = _poll_manager.pollEngine(timeout);

		if (poll_count == -1) {
			perror("Line 43 ");
			throw PollFailed();
		};

		/*
		** Get tout les fds surveillés pour pouvoir boucler dessus et ainsi inspecter revents
		*/
		std::vector<struct pollfd> fds = _poll_manager.getFds();
		for(int i = 0; i < _poll_manager.getSize(); i++) {
			int fd = fds[i].fd;
			/*
			** Si le fd observé est un listener et quil est readable = client essaye de se connecter
			*/
			if (std::find(_listening_sockets.begin(), _listening_sockets.end(), fd)) {
				if (_poll_manager.isReadable(fd)) {
					handleNewConnection(_listening_sockets[std::find(_listening_sockets.begin(), _listening_sockets.end(), fds[i])]);
				}
			} else {
				/*
				** Sinn si le fd n'est pas un fd d'ecoute alors on a 3 options
				** soit un client est pret a etre read (POLLIN)
				** soit un client est pret a etre write (POLLOUT)
				** soit un client s'est deconnecter (POLLER | POLLHUP)
				*/
				if (_poll_manager.isReadable(fd)) {
					handleClientRead(fd);
				} else if (_poll_manager.isWritable(fd)) {
					handleClientWrite(fd);
				} else {
					closeClient(fd);
				}
			}
		}
		checkTimeouts();
	}
};

void Server::stop() {
	_state = false;
};

const char* Server::PollFailed::what() const throw() {
	return "Server.cpp: PollFailed";
};