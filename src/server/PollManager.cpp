#include "../../include/server/PollManager.hpp"

PollManager::PollManager() {
	_fds.reserve(5);
	std::cout << "[PollManager] ctor called\n";
};

PollManager::~PollManager() {
	std::cout << "[PollManager] dtor called\n";
};

// ---------------------- PRIVATE ----------------------

int PollManager::findIndex(int fd) const {
	std::size_t i = 0;
	while (_fds[i].fd != fd) {
		i++;
	};
	return i;
};

// ---------------------- PUBLIC ----------------------

int PollManager::getSize() const {
	return _fds.size();
};

void PollManager::addFd(int fd, short events) {
	struct pollfd newFd;

	newFd.fd = fd;
	newFd.events = events;
	_fds.push_back(newFd);
};

void PollManager::removeFd(int fd) {
	size_t idx = findIndex(fd);
	if (idx < _fds.size()) {
		_fds.erase(_fds.begin() + idx);
	}
};

void PollManager::updateEvents(int fd, short events) {
	int idx = findIndex(fd);
	_fds[idx].events = events;
};

int PollManager::pollEngine(int timeout_ms) {
	int poll_count = poll(&_fds[0], _fds.size(), timeout_ms);
	
	return poll_count;
};

bool PollManager::isReadable(int fd) const {
	int idx = findIndex(fd);

	if (_fds[idx].revents & POLLIN) {
		return true;
	}
	return false;
};

bool PollManager::isWritable(int fd) const {
	int idx = findIndex(fd);

	if (_fds[idx].revents & POLLOUT) {
		return true;
	}
	return false;
};

bool PollManager::hasError(int fd) const {
	int idx = findIndex(fd);
	
	if (_fds[idx].revents & (POLLHUP | POLLNVAL | POLLERR)) {
		return true;
	}
	return false;
};

const std::vector<struct pollfd>& PollManager::getFds() const {
	return _fds;
};