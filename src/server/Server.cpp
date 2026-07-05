#include "../../include/server/Server.hpp"
#include <algorithm>
#include <ctime>

Server::Server()
	: _listening_sockets(), _clients(), _configs_by_port(), _poll_manager(), _running(false) {
};

Server::~Server() {};

void Server::handleNewConnection(Socket *socket) {
	try {
		int newFd = socket->acceptConnection();
		Client* newClient = new Client(newFd, socket->getPort());
		_clients[newFd] = newClient;
		_poll_manager.addFd(newFd, POLLIN);
	} catch (std::exception& err) {
		std::cout << err.what() << std::endl;
	}
};

void Server::handleClientRead(int fd) {
	try {
		int n_read = _clients[fd]->receiveData();
		
		if (n_read == 0 and _clients[fd]->getState() == CLOSING) {
			closeClient(_clients[fd]->getFd());
		} else if (_clients[fd]->getState() == PROCESSING) {
			_clients[fd]->setWriteBuffer(processHttp(_clients[fd]->getReadBuffer(), getConfigForClient(_clients[fd])));
			_poll_manager.updateEvents(fd, POLLIN | POLLOUT);
		} else if (_clients[fd]->getState() == CLOSING) {
			closeClient(_clients[fd]->getFd());
		}
	} catch (std::exception& err) {
		closeClient(fd);
		std::cout << err.what() << std::endl;
	}
};

void Server::handleClientWrite(int fd) {
	try {
		if (_clients[fd]->getState() == SENDING_RESPONSE) {
			_clients[fd]->sendData();
			if (_clients[fd]->getState() == DONE) {
				_clients[fd]->setState(READING_REQUEST);
				_poll_manager.updateEvents(fd, POLLIN);
			} else if (_clients[fd]->getState() == CLOSING) {
				closeClient(_clients[fd]->getFd());
			}
		}
	} catch (std::exception& err) {
		std::cout << err.what() << std::endl;
	}
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
	std::time_t now = std::time(NULL);
	const long TIMEOUT = 5;

	std::map<int, Client*>::iterator it = _clients.begin();
	while (it != _clients.end()) {
		int fd = it->first;
		Client *c = it->second;

		if (now - c->getLastActivity() > TIMEOUT) {
			std::cout << "Client " << fd << " has been disconnected for inactivity" << std::endl;
			++it;
			closeClient(fd);
		} else {
			++it;
		}
	}
}

const ServerConfig& Server::getConfigForClient(const Client* client) const {
	int port = client->getServerPort();

	std::map<int, ServerConfig>::const_iterator it = _configs_by_port.find(port);

	if (it == _configs_by_port.end()) {
		throw std::runtime_error("No ServerConfig for port");
	}
	return it->second;
}

Socket *Server::findListeningSocketByFd(int fd) {
	std::vector<Socket*>::iterator it = _listening_sockets.begin();
	while (it != _listening_sockets.end()) {
		if ((*it)->getFd() == fd)
			return *it;
		it++;
	}
	return NULL;
}

void Server::addServerConfig(const ServerConfig& config) {
	_configs_by_port[config.getPort()] = config;
	
	Socket *s = new Socket(config);
	_listening_sockets.push_back(s);

	_poll_manager.addFd(s->getFd(), POLLIN);
	std::cout << "port: " << config.getPort() << std::endl;
	std::cout << "socket fd: " << s->getFd() << std::endl;
	std::cout << "Added server config" << std::endl;
};

void Server::run() {
	_running = true;

	while (_running) {
		// std::cout << _running << std::endl;
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
			Socket *socket = findListeningSocketByFd(fd);
			/*
			** Si le fd observé est un listener et quil est readable = client essaye de se connecter
			*/
			if (socket and _poll_manager.isReadable(fd)) {
				std::cout << "handleNewConnection" << std::endl;
				handleNewConnection(socket);
				continue;
			}
			
			if (_poll_manager.hasError(fd)) {
				handleClientRead(fd);
				continue;
			}
			/*
			** Sinn si le fd n'est pas un fd d'ecoute alors on a 3 options
			** soit un client est pret a etre read (POLLIN)
			** soit un client est pret a etre write (POLLOUT)
			** soit un client s'est deconnecter (POLLER | POLLHUP)
			*/
			if (_poll_manager.isReadable(fd)) {
				std::cout << "handleClientRead" << std::endl;
				handleClientRead(fd);
			} else if (_poll_manager.isWritable(fd)) {
				std::cout << "handleClientWrite" << std::endl;
				handleClientWrite(fd);
			}
		}
		checkTimeouts();
	}
};

void Server::stop() {
	_running = false;
};

const char* Server::PollFailed::what() const throw() {
	return "Server.cpp: PollFailed";
};