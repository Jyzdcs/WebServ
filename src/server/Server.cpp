#include "../../include/server/Server.hpp"
#include <algorithm>
#include <ctime>
#include <unistd.h>
#include <signal.h>

Server::Server()
	: _listening_sockets(), _clients(), _configs_by_port(), _cgi_map(), _poll_manager(), _running(false) {
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
		std::map<int, Client*>::iterator it = _clients.find(fd);
		if (it == _clients.end() || it->second == NULL) {
				std::cerr << "[READ] fd " << fd
									<< " not found in _clients\n";
				return;
		}
		Client *client = it->second;

		int n_read = client->receiveData();
		
		if (n_read == 0 and client->getState() == CLOSING) {
			closeClient(client->getFd());
			std::cout << "Client " << fd << " disconnected" << std::endl;
		} else if (client->getState() == PROCESSING) {
			// std::cout << "ReadBuffer size: " << client->getReadBuffer().size() << std::endl;
			// std::cout << "ReadBuffer content: [" << client->getReadBuffer() << "]" << std::endl;

			// MOCK DE CLAUDE
			// if (client->getReadBuffer().find("/cgi-bin") != std::string::npos) {
			// 	client->setWriteBuffer("HTTP/1.1 200 OK\r\nContent-Length: 8\r\nConnection: close\r\n\r\nCGI mock");
			// 	client->setShouldClose(true);
			// 	_poll_manager.updateEvents(fd, POLLIN | POLLOUT);
			// 	return;
			// }

			ProcessResult result = processHttp(client->getReadBuffer(), getConfigForClient(client));

			if(result.state == ProcessResult::COMPLETE) {
				client->setWriteBuffer(result.rawResponse);
				client->setShouldClose(result.shouldClose);
				_poll_manager.updateEvents(fd, POLLIN | POLLOUT);
			} else {
				CgiContext ctx;
				ctx.pid = result.pid;
				ctx.deadline = result.deadline;
				ctx.clientFd = fd;
				ctx.shouldClose = result.shouldClose;
				ctx.output = "";

				// std::cout << "clientFd which has cgi running=" << ctx.clientFd << std::endl;
				// std::cout << "ctx.clientFd=" << ctx.clientFd << std::endl
				// 					<< "result.stdoutFd=" << result.stdoutFd << std::endl
				// 					<< "fd=" << fd << std::endl;
				_cgi_map[result.stdoutFd] = ctx;
				_poll_manager.addFd(result.stdoutFd, POLLIN);
				// std::cout << "pipe fd added to the poll manager" << std::endl;
				// std::cout << _cgi_map.count(result.stdoutFd) << std::endl;
			}
		} else if (client->getState() == CLOSING) {
			closeClient(client->getFd());
			std::cout << "Client " << fd << " disconnected" << std::endl;
		}
	} catch (std::exception& err) {
		closeClient(fd);
		std::cout << "Client " << fd << " disconnected" << std::endl;
		std::cout << "Exception: " << err.what() << std::endl;
	}
};

void Server::handleClientWrite(int fd) {
	try {
		if (_clients[fd]->getState() == SENDING_RESPONSE) {
			_clients[fd]->sendData();
			if (_clients[fd]->shouldClose() and _clients[fd]->getState() == DONE) {
				std::cout << "KeepAlive: false" << std::endl;
				closeClient(_clients[fd]->getFd());
				std::cout << "Client " << fd << " has been disconnected cause of keepAlive set to false" << std::endl;
			} else if (_clients[fd]->getState() == DONE) {
				_clients[fd]->setState(READING_REQUEST);
				_poll_manager.updateEvents(fd, POLLIN);
			} else if (_clients[fd]->getState() == CLOSING) {
				closeClient(_clients[fd]->getFd());
				std::cout << "Client " << fd << " disconnected" << std::endl;
			}
		}
	} catch (std::exception& err) {
		std::cout << "Exception: " << err.what() << std::endl;
	}
};

void Server::handleCgiRead(int fd) {
	char buf[4096];

	while (true) {
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n > 0) {
			_cgi_map[fd].output += std::string(buf, n);
		} else if (n == 0) {
			CgiContext ctx = _cgi_map[fd];
			_poll_manager.removeFd(fd);
			_cgi_map.erase(fd);
			if (!_clients.count(ctx.clientFd))
				return;
			std::string res = finishCgi(ctx.output, ctx.pid, false, ctx.shouldClose, getConfigForClient(_clients[ctx.clientFd]));
			_clients[ctx.clientFd]->setWriteBuffer(res);
			_clients[ctx.clientFd]->setShouldClose(ctx.shouldClose);
			_poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
			return;
		} else {
			CgiContext ctx = _cgi_map[fd];
			_poll_manager.removeFd(fd);
			_cgi_map.erase(fd);
			if (!_clients.count(ctx.clientFd))
				return;
			bool timedOut = (time(NULL) >= ctx.deadline);
			if (timedOut) kill(ctx.pid, SIGKILL);
			std::string res = finishCgi(ctx.output, ctx.pid, timedOut, ctx.shouldClose, getConfigForClient(_clients[ctx.clientFd]));
			_clients[ctx.clientFd]->setWriteBuffer(res);
			_clients[ctx.clientFd]->setShouldClose(ctx.shouldClose);
			_poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
			return;
		}
	}
};

void Server::handleCgiHup(int fd) {
	CgiContext ctx = _cgi_map[fd];
	_poll_manager.removeFd(fd);
	_cgi_map.erase(fd);
	if (!_clients.count(ctx.clientFd))
		return;
	bool timedOut = (time(NULL) >= ctx.deadline);
	if (timedOut) kill(ctx.pid, SIGKILL);
	std::string res = finishCgi(ctx.output, ctx.pid, timedOut, ctx.shouldClose, getConfigForClient(_clients[ctx.clientFd]));
	_clients[ctx.clientFd]->setWriteBuffer(res);
	_clients[ctx.clientFd]->setShouldClose(ctx.shouldClose);
	_poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
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

		bool hasCgi = false;
		for (std::map<int, CgiContext>::iterator ci = _cgi_map.begin(); ci != _cgi_map.end(); ++ci) {
			if (ci->second.clientFd == fd) { hasCgi = true; break; }
		}

		if (!hasCgi && now - c->getLastActivity() > TIMEOUT) {
			std::cout << "Client " << fd << " has been disconnected for inactivity" << std::endl;
			++it;
			closeClient(fd);
		} else {
			++it;
		}
	}
}

void Server::checkCgiTimeouts() {
	std::time_t now = std::time(NULL);
	std::map<int, CgiContext>::iterator it = _cgi_map.begin();
	while (it != _cgi_map.end()) {
		int pipeFd = it->first;
		CgiContext ctx = it->second;
		if (now >= ctx.deadline) {
			kill(ctx.pid, SIGKILL);
			_poll_manager.removeFd(pipeFd);
			_cgi_map.erase(it++);
			if (!_clients.count(ctx.clientFd))
				continue;
			std::string res = finishCgi(ctx.output, ctx.pid, true, ctx.shouldClose, getConfigForClient(_clients[ctx.clientFd]));
			_clients[ctx.clientFd]->setWriteBuffer(res);
			_clients[ctx.clientFd]->setShouldClose(true);
			_poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
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
		int fdsSize = (int)fds.size();
		for(int i = 0; i < fdsSize; i++) {
			int fd = fds[i].fd;
			Socket *socket = findListeningSocketByFd(fd);
			/*
			** Si le fd observé est un listener et quil est readable = client essaye de se connecter
			*/
			if (socket and _poll_manager.isReadable(fd)) {
				// std::cout << "handleNewConnection" << std::endl;
				handleNewConnection(socket);
				continue;
			}
			
			/*
			** Sinn si le fd n'est pas un fd d'ecoute alors on a 3 options
			** soit un client s'est deconnecter (POLLER | POLLHUP)
			** soit c'est le stdoutFd du pipe d'un cgi est en POLLIN | POLLOUT
			** soit un client est pret a etre read (POLLIN)
			** soit un client est pret a etre write (POLLOUT)
			*/

			if (_cgi_map.count(fd)) {
				if (_poll_manager.isReadable(fd))
					handleCgiRead(fd);
				else if (_poll_manager.hasError(fd))
					handleCgiHup(fd);
			}
			else if (_poll_manager.hasError(fd)) {
				handleClientRead(fd);
				continue;
			}
			else if (_poll_manager.isReadable(fd)) {
				std::cout << "handleClientRead" << std::endl;
				handleClientRead(fd);
			} else if (_poll_manager.isWritable(fd)) {
				std::cout << "handleClientWrite" << std::endl;
				handleClientWrite(fd);
			}
		}
		checkTimeouts();
		checkCgiTimeouts();
	}
};

void Server::stop() {
	_running = false;
};

const char* Server::PollFailed::what() const throw() {
	return "Server.cpp: PollFailed";
};