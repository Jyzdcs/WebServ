#include "../../include/server/Server.hpp"
#include <algorithm>
#include <ctime>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

volatile sig_atomic_t Server::_stop = 0;

void Server::sigHandler(int) { Server::_stop = 1; }

Server::Server()
	: _listening_sockets(), _clients(), _configs_by_port(), _cgi_map(), _poll_manager(), _running(false) {
};

Server::~Server() {
	for (std::map<int, CgiContext>::iterator it = _cgi_map.begin(); it != _cgi_map.end(); ++it) {
		kill(it->second.pid, SIGKILL);
		waitpid(it->second.pid, NULL, 0);
		close(it->first);
	}
	_cgi_map.clear();

	while (!_clients.empty())
		closeClient(_clients.begin()->first);

	for (std::vector<Socket*>::iterator it = _listening_sockets.begin(); it != _listening_sockets.end(); ++it)
		delete *it;
};

void Server::handleNewConnection(Socket *socket) {
	try {
		int newFd = socket->acceptConnection();
		Client* newClient = new Client(newFd, socket->getPort());
		_clients[newFd] = newClient;
		_poll_manager.addFd(newFd, POLLIN);
	} catch (std::exception& err) {
		(void)err;
	}
};

void Server::handleClientRead(int fd) {
	try {
		std::map<int, Client*>::iterator it = _clients.find(fd);
		if (it == _clients.end() || it->second == NULL)
			return;
		Client *client = it->second;

		int n_read = client->receiveData();
		
		if (n_read == 0 && client->getState() == CLOSING) {
			closeClient(client->getFd());
		} else if (client->getState() == PROCESSING) {
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
				ctx.config = getConfigForClient(client);
				_cgi_map[result.stdoutFd] = ctx;
				_poll_manager.addFd(result.stdoutFd, POLLIN);
			}
		} else if (client->getState() == CLOSING) {
			closeClient(client->getFd());
		}
	} catch (std::exception& err) {
		(void)err;
		closeClient(fd);
	}
};

void Server::handleClientWrite(int fd) {
	try {
		if (_clients[fd]->getState() == SENDING_RESPONSE) {
			_clients[fd]->sendData();
			if (_clients[fd]->shouldClose() && _clients[fd]->getState() == DONE) {
				closeClient(_clients[fd]->getFd());
			} else if (_clients[fd]->getState() == DONE) {
				_clients[fd]->setState(READING_REQUEST);
				_poll_manager.updateEvents(fd, POLLIN);
			} else if (_clients[fd]->getState() == CLOSING) {
				closeClient(_clients[fd]->getFd());
			}
		}
	} catch (std::exception& err) {
		(void)err;
		closeClient(fd);
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
			if (!_clients.count(ctx.clientFd)) {
				waitpid(ctx.pid, NULL, 0);
				return;
			}
			std::string res = finishCgi(ctx.output, ctx.pid, false, ctx.shouldClose, ctx.config);
			_clients[ctx.clientFd]->setWriteBuffer(res);
			_clients[ctx.clientFd]->setShouldClose(ctx.shouldClose);
			_poll_manager.updateEvents(ctx.clientFd, POLLIN | POLLOUT);
			return;
		} else {
			CgiContext ctx = _cgi_map[fd];
			_poll_manager.removeFd(fd);
			_cgi_map.erase(fd);
			if (!_clients.count(ctx.clientFd)) {
				bool timedOut = (time(NULL) >= ctx.deadline);
				if (timedOut) kill(ctx.pid, SIGKILL);
				waitpid(ctx.pid, NULL, 0);
				return;
			}
			bool timedOut = (time(NULL) >= ctx.deadline);
			if (timedOut) kill(ctx.pid, SIGKILL);
			std::string res = finishCgi(ctx.output, ctx.pid, timedOut, ctx.shouldClose, ctx.config);
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
	if (!_clients.count(ctx.clientFd)) {
		bool timedOut = (time(NULL) >= ctx.deadline);
		if (timedOut) kill(ctx.pid, SIGKILL);
		waitpid(ctx.pid, NULL, 0);
		return;
	}
	bool timedOut = (time(NULL) >= ctx.deadline);
	if (timedOut) kill(ctx.pid, SIGKILL);
	std::string res = finishCgi(ctx.output, ctx.pid, timedOut, ctx.shouldClose, ctx.config);
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
			if (!_clients.count(ctx.clientFd)) {
				waitpid(ctx.pid, NULL, 0);
				continue;
			}
			std::string res = finishCgi(ctx.output, ctx.pid, true, ctx.shouldClose, ctx.config);
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

	std::map<int, std::vector<ServerConfig> >::const_iterator it = _configs_by_port.find(port);
	if (it == _configs_by_port.end())
		throw std::runtime_error("No ServerConfig for port");

	const std::vector<ServerConfig>& configs = it->second;
	if (configs.size() == 1)
		return configs[0];

	const std::string& buf = client->getReadBuffer();
	std::string host_value;
	std::string::size_type pos = buf.find("\r\nHost:");
	if (pos == std::string::npos)
		pos = buf.find("\r\nhost:");
	if (pos != std::string::npos) {
		pos += 7;
		while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == '\t'))
			pos++;
		std::string::size_type end = buf.find("\r\n", pos);
		if (end != std::string::npos) {
			host_value = buf.substr(pos, end - pos);
			std::string::size_type colon = host_value.find(':');
			if (colon != std::string::npos)
				host_value = host_value.substr(0, colon);
		}
	}

	for (std::size_t i = 0; i < configs.size(); ++i) {
		if (configs[i].getServerName() == host_value)
			return configs[i];
	}
	return configs[0];
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
	int port = config.getPort();
	_configs_by_port[port].push_back(config);

	for (std::size_t i = 0; i < _listening_sockets.size(); ++i) {
		if (_listening_sockets[i]->getPort() == port)
			return;
	}
	Socket *s = new Socket(config);
	_listening_sockets.push_back(s);
	_poll_manager.addFd(s->getFd(), POLLIN);
};

void Server::run() {
	_running = true;

	while (_running && !Server::_stop) {
		const int timeout = 0;
		int poll_count = _poll_manager.pollEngine(timeout);

		if (poll_count == -1) {
			if (Server::_stop) break;
			throw PollFailed();
		}

		std::vector<struct pollfd> fds = _poll_manager.getFds();
		int fdsSize = (int)fds.size();
		for(int i = 0; i < fdsSize; i++) {
			int fd = fds[i].fd;
			Socket *socket = findListeningSocketByFd(fd);

			if (socket && _poll_manager.isReadable(fd)) {
				handleNewConnection(socket);
				continue;
			}

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
				handleClientRead(fd);
			} else if (_poll_manager.isWritable(fd)) {
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
