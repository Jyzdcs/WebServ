#include "../../include/server/Client.hpp"
#include <string>
#include <ctime>
#include <iostream>

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

};

bool Client::isRequestComplete() const {

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
