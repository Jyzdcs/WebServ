#include "../../include/server/PollManager.hpp"
#include <unistd.h>

PollManager::PollManager() {
	_fds.reserve(5);
	std::cout << "[PollManager] ctor called\n";
};

PollManager::~PollManager() {
	std::cout << "[PollManager] dtor called\n";
};

// ---------------------- PRIVATE ----------------------

int PollManager::findIndex(int fd) const {
	for (std::size_t i = 0; i < _fds.size(); i++) {
		if (_fds[i].fd == fd)
			return (int)i;
	}
	return -1;
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
	int idx = findIndex(fd);
	if (idx >= 0 && (std::size_t)idx < _fds.size()) {
		_fds.erase(_fds.begin() + idx);
	}
	close(fd);
};

void PollManager::updateEvents(int fd, short events) {
	int idx = findIndex(fd);
	if (idx >= 0)
		_fds[idx].events = events;
};

int PollManager::pollEngine(int timeout_ms) {
	int poll_count = poll(&_fds[0], _fds.size(), timeout_ms);
	
	return poll_count;
};

bool PollManager::isReadable(int fd) const {
	int idx = findIndex(fd);
	if (idx < 0)
		return false;
	if (_fds[idx].revents & POLLIN) {
		return true;
	}
	return false;
};

bool PollManager::isWritable(int fd) const {
	int idx = findIndex(fd);
	if (idx < 0)
		return false;
	if (_fds[idx].revents & POLLOUT) {
		return true;
	}
	return false;
};

bool PollManager::hasError(int fd) const {
	int idx = findIndex(fd);
	if (idx < 0)
		return false;
	if (_fds[idx].revents & (POLLHUP | POLLNVAL | POLLERR)) {
		return true;
	}
	return false;
};

const std::vector<struct pollfd>& PollManager::getFds() const {
	return _fds;
};