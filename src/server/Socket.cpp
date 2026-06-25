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

Socket::Socket(ServerConfig serverConf) {
	std::cout << "[Socket] ctor called\n";

	_port = serverConf.getPort();
	_host = serverConf.getHost();

	int yes=1;
	int rv;

	struct addrinfo hints, *ai, *p;

	// Fill hints with all the parameters
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

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

		// Braek cause we need to just bind 1 ip for the server
		break;
	}

	// If p == NULL it mean that there has no available socket
	if (p == NULL) {
		throw FailedToBindPort();
	}

	freeaddrinfo(ai); // free all the addrinfo
};

  
Socket::~Socket() {
	std::cout << "[Socket] dtor called\n";
};


inline std::string Socket::intToString(int value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

int Socket::getFd() const {

};


int Socket::acceptConnection() const {

};


int Socket::getPort() const {

};

const char * Socket::FailedToBindPort::what() const throw() {
	return "SocketError: Failed to bind port!";
};