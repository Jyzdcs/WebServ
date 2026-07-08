#include "../../include/server/Socket.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <sstream>
#include <fcntl.h>

Socket::Socket(ServerConfig serverConf) {
	struct addrinfo hints, *ai, *p;
	int yes=1;
	int rv;

	_port = serverConf.getPort();
	_host = serverConf.getHost();

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, intToString(_port).c_str(), &hints, &ai)) != 0) {
		(void)rv;
		throw FailedToBindPort();
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (_fd < 0)
			continue;
		setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(_fd, p->ai_addr, p->ai_addrlen) < 0) {
			close(_fd);
			continue;
		}
		break;
	}

	if (p == NULL)
		throw FailedToBindPort();

	freeaddrinfo(ai);

	if (listen(_fd, 128) == -1)
		throw ListenFalied();

	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
		throw FcntlFailed();
	if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw FcntlFailed();
};

Socket::~Socket() {
	close(_fd);
};

inline std::string Socket::intToString(int value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

int Socket::getFd() const {
	return _fd;
};

int Socket::acceptConnection() const {
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int newConnectionFd;
	int flags;

	newConnectionFd = accept(_fd, (struct sockaddr *)&client_addr, &addrlen);
	if (newConnectionFd < 0)
		throw AcceptNewConnectionFailed();

	flags = fcntl(newConnectionFd, F_GETFL, 0);
	if (flags == -1)
		throw FcntlFailed();
	if (fcntl(newConnectionFd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw FcntlFailed();

	return newConnectionFd;
};

int Socket::getPort() const {
	return _port;
};

const char * Socket::FailedToBindPort::what() const throw() {
	return "SocketError: Failed to bind port!";
};

const char * Socket::FcntlFailed::what() const throw() {
	return "SocketError: FcntlFailed in core server!";
};

const char * Socket::ListenFalied::what() const throw() {
	return "SocketError: Listen function in core server failed!";
};

const char * Socket::AcceptNewConnectionFailed::what() const throw() {
	return "SocketError: newConnectionFd failed, there is no client to accept!";
};
