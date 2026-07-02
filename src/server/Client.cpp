#include "../../include/server/Client.hpp"
#include <string>
#include <ctime>
#include <iostream>
#include <unistd.h>

Client::Client(int fd, int server_port) 
	: _fd(fd), _server_port(server_port), _state(READING_REQUEST), _read_buffer(), _write_buffer(), _write_offset(0) {
	_last_activity = getCurrentTimeStamp();
};

Client::~Client() {};

long Client::getCurrentTimeStamp() const {
	std::time_t now = std::time(NULL);
	return static_cast<long>(now);
}

void Client::setReadBuffer(std::string buf) {
	_read_buffer = buf;
};

int Client::getFd() const {
	return _fd;
};

int Client::receiveData() {
	char buf[2006];
	int	n_read;
	
	n_read = read(_fd, buf, sizeof(buf));
	std::cout << "n_read: " << n_read << std::endl;
	if (n_read > 0) {
		_read_buffer.append(buf, n_read);
		updateLastActivity();
		if (isRequestComplete()) {
			_state = PROCESSING;
			std::cout << _state << std::endl;
		}
	} else if (n_read == 0) {
		std::cout << "Client " << _fd << " disconnected" << std::endl;
		setState(CLOSING);
	} else {
		setState(CLOSING);
		perror("Line 38 ");
		throw ReadFailed();
	}
	return n_read;
};

/* Request valid example
POST /test HTTP/1.1\r\n
Host: example.com\r\n
Content-Length: 11\r\n
\r\n
hello world
*/

bool Client::isRequestComplete() const {
	// std::cout << "Request Content: " << std::endl << "`" << _read_buffer << "`" << std::endl;
	// Trouver l'index de la fin du header
	std::string::size_type header_end = _read_buffer.find("\r\n\r\n");

	// Return false et print une erreur si le header est incomplet
	if (header_end == std::string::npos) {
		perror("Header not complete");
		return false;
	}

	// Stocker la length du headers
	std::string::size_type headers_size = header_end + 4;

	// Extraire le header
	std::string headers = _read_buffer.substr(0, headers_size);
	
	// Obtenir l'index de content-length
	const std::string cl_header = "Content-Length:";
	std::string::size_type pos = headers.find(cl_header);

	if (pos == std::string::npos) {
		// Pas de body ou chunked
		return true;
	}

	pos += cl_header.size() + 1;
	// Tant quon est pas sur la valeur REEL de content length, on incremente l'index
	// Exemple : Content-Length:          \t 13\r\n
	while (pos == ' ' or pos == '\t') {
		pos++;
	}

	// Compter la size de la valeur de COntent-Length
	std::string::size_type end = pos;
	while (end < headers.size() and std::isdigit(headers[end])) {
		end++;
	}

	// Extraire et convertir la value de Content-Length
	std::string cl_value = headers.substr(pos, end - pos);
	long content_length = std::strtol(cl_value.c_str(), NULL, 10);
	if (content_length < 0) {
		// valeur invalide
		return false;
	}
	// Vérifier qu'on a bien tout le body
	std::string::size_type body_bytes =
			_read_buffer.size() - headers_size;

	if (body_bytes >= static_cast<std::string::size_type>(content_length)) {
		return true;
	}
	return false;
};

int Client::sendData() {

};

bool Client::isResponseFullySent() const {

};

ClientState Client::getState() const {
	return _state;
};

void Client::setState(ClientState state) {
	_state = state;
};

std::string& Client::getReadBuffer() {
	return _read_buffer;
};

void Client::setWriteBuffer(const std::string& response) {
};

int Client::getServerPort() const {
	return _server_port;
};

long Client::getLastActivity() const {
	return _last_activity;
};

void Client::updateLastActivity() {
	_last_activity = getCurrentTimeStamp();
};

const char* Client::ReadFailed::what() const throw() {
	return "ReadFailed";
};