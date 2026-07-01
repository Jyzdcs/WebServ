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

int Client::getFd() const {
	return _fd;
};

int Client::receiveData() {
	char buf[256];
	int	n_read;
	
	n_read = read(_fd, buf, sizeof(buf));
	if (n_read > 0) {
		_read_buffer.append(buf, n_read);
		updateLastActivity();
		if (isRequestComplete()) {
			_state = PROCESSING;
		}
	} else if (n_read == 0) {
		setState(CLOSING);
	} else {
		setState(CLOSING);
		perror("Line 38 : ");
		ReadFailed();
	}
	return n_read;
};

bool Client::isRequestComplete() const {
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

	pos += cl_header.size();
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
	return "Error: ReadFailed";
};