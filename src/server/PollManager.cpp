#include "../../include/server/PollManager.hpp"

PollManager::PollManager() {
	_fds.reserve(5);
	std::cout << "[PollManager] ctor called\n";
};

PollManager::PollManager(const PollManager& other) {
};

PollManager& PollManager::operator=(const PollManager& other) {
	// return *this;
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
	int idx = findIndex(fd);
	if (idx < _fds.size()) {
		_fds.erase(_fds.begin() + idx);
	}
};

void PollManager::updateEvents(int fd, short events) {

};

int PollManager::poll(int timeout_ms) {

};

bool PollManager::isReadable(int fd) const {

};

bool PollManager::isWritable(int fd) const {

};

bool PollManager::hasError(int fd) const {

};
