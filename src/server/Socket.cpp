#include "../../include/server/Socket.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <fcntl.h>

Socket::Socket(ServerConfig serverConf) {
	std::cout << "[Socket] ctor called\n";
	struct addrinfo hints, *ai, *p;
	int yes=1;
	int rv;

	_port = serverConf.getPort();
	_host = serverConf.getHost();

	// Fill hints with all the parameters
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;						// IPv4
	hints.ai_socktype = SOCK_STREAM;			// TCP
	hints.ai_flags = AI_PASSIVE;					// pour bind() cote server

	// Call getaddrinfo to get all the potentially usable adress that match with the config
	if ((rv = getaddrinfo(NULL, intToString(_port).c_str(), &hints, &ai)) != 0) {
		fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	// Find the first socket where we're able to bind
	for(p = ai; p != NULL; p = p->ai_next) {
		_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		// If socket fail, try to the next
		if (_fd < 0) {
			continue;
		}

		// Add option to the socket which will allow us to rebind an old port
		setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
						sizeof(int));

		// If bind isnt successful check the next socket
		if (bind(_fd, p->ai_addr, p->ai_addrlen) < 0) {
				close(_fd);
				continue;
		}

		// Break cause we need to just bind 1 ip for the server
		break;
	}

	// If p == NULL it mean that there has no available socket
	if (p == NULL) {
		std::cout << "Line 59 in Socket.cpp  ";
		throw FailedToBindPort();
	}

	freeaddrinfo(ai); // free all the addrinfo

	// Mettre l'etat du server en LISTEN
	if (listen(_fd, 10) == -1) {
		std::cout << "Line 67 in Socket.cpp ";
		throw ListenFalied();
	}

	// Rendre le socket _fd non bloquant
	// int flags = fcntl(_fd, F_GETFL, 0);
	// if (flags == -1) {
	// 	std::cout << "Line 74 in Socket.cpp ";
	// 	throw FcntlFailed();
	// };
	// if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
	// 	std::cout << "Line 78 in Socket.cpp ";
	// 	throw FcntlFailed();
	// }
};

  
Socket::~Socket() {
	close(_fd);
	std::cout << "[Socket] dtor called\n";
};


inline std::string Socket::intToString(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

int Socket::getFd() const {
	return _fd;
};


int Socket::acceptConnection() const {
	struct sockaddr_in client_addr;
	socklen_t 				addrlen = sizeof(client_addr);
	int 							newConnectionFd;
	int 							flags;
	
	// Creation de la nouvelle socket client
	newConnectionFd = accept(_fd, (struct sockaddr *)&client_addr, &addrlen);
	if (newConnectionFd < 0) {
		std::cout << "Line 111 in Socket.cpp ";
		throw AcceptNewConnectionFailed();
	}
	// Mettre le socket client en non bloquant
	flags = fcntl(newConnectionFd, F_GETFL, 0);
	if (flags == -1) {
		std::cout << "Line 116 in Socket.cpp ";
		throw FcntlFailed();
	};
	if (fcntl(newConnectionFd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cout << "Line 120 in Socket.cpp ";
		throw FcntlFailed();
	}
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