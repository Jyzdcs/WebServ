#include "../../include/server/Client.hpp"
#include <string>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>

Client::Client(int fd, int server_port) 
	: _fd(fd), _server_port(server_port), _shouldClose(false), _state(READING_REQUEST), _read_buffer(), _write_buffer(), _write_offset(0) {
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

void Client::setShouldClose(bool shouldClose) {
	_shouldClose = shouldClose;
};

bool Client::shouldClose() {
	return _shouldClose;
};

int Client::getFd() const {
	return _fd;
};

int Client::receiveData() {
	char buf[4096];
	int n_read;

	n_read = recv(_fd, buf, sizeof(buf), 0);
	if (n_read > 0) {
		_read_buffer.append(buf, n_read);
		updateLastActivity();
		if (isRequestComplete())
			_state = PROCESSING;
	} else if (n_read == 0) {
		setState(CLOSING);
	} else {
		setState(CLOSING);
		throw ReadFailed();
	}
	return n_read;
};

bool Client::isRequestComplete() const {
	std::string::size_type header_end = _read_buffer.find("\r\n\r\n");

	if (header_end == std::string::npos)
		return false;

	std::string::size_type headers_size = header_end + 4;
	std::string headers = _read_buffer.substr(0, headers_size);

	std::string headers_lower = headers;
	for (std::size_t i = 0; i < headers_lower.size(); ++i)
		headers_lower[i] = std::tolower(headers_lower[i]);

	// Chunked transfer: complete when terminating "0\r\n\r\n" is present
	std::string::size_type te_pos = headers_lower.find("transfer-encoding:");
	if (te_pos != std::string::npos) {
		std::string::size_type te_end = headers_lower.find("\r\n", te_pos);
		std::string te_val = headers_lower.substr(te_pos + 18,
			te_end != std::string::npos ? te_end - te_pos - 18 : std::string::npos);
		std::string::size_type first = te_val.find_first_not_of(" \t");
		if (first != std::string::npos) te_val = te_val.substr(first);
		if (te_val.find("chunked") != std::string::npos)
			return _read_buffer.find("0\r\n\r\n", headers_size) != std::string::npos;
	}

	// Content-Length: wait until full body received
	const std::string cl_header = "content-length:";
	std::string::size_type pos = headers_lower.find(cl_header);

	if (pos == std::string::npos)
		return true;

	pos += cl_header.size();
	while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
		pos++;

	std::string::size_type end = pos;
	while (end < headers.size() && std::isdigit(headers[end]))
		end++;

	std::string cl_value = headers.substr(pos, end - pos);
	long content_length = std::strtol(cl_value.c_str(), NULL, 10);
	if (content_length < 0)
		return false;

	std::string::size_type body_bytes = _read_buffer.size() - headers_size;
	return body_bytes >= static_cast<std::string::size_type>(content_length);
};

int Client::sendData() {
	if (_write_offset >= _write_buffer.size())
		return 0;

	const char *buf = _write_buffer.data() + _write_offset;
	std::size_t to_send = _write_buffer.size() - _write_offset;

	int sent = ::send(_fd, buf, to_send, 0);

	if (sent > 0) {
		_write_offset += sent;
		updateLastActivity();
		if (_write_offset == _write_buffer.size())
			_state = DONE;
	} else if (sent < 0) {
		_state = CLOSING;
	}
	return sent;
};

bool Client::isResponseFullySent() const {
	return _write_offset >= _write_buffer.size();
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

const std::string& Client::getReadBuffer() const {
	return _read_buffer;
};

void Client::setWriteBuffer(const std::string& response) {
	_write_buffer = response;
	_write_offset = 0;
	_state = SENDING_RESPONSE;
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
