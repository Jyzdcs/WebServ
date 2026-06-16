#include "include/server/PollManager.hpp"

int main(void)
{
	PollManager pollManager;

	std::cout << pollManager.getSize() << std::endl;
	pollManager.addFd(1, POLLIN);
	pollManager.addFd(2, POLLIN);
	std::cout << pollManager.getSize() << std::endl;
	pollManager.removeFd(2);
	std::cout << pollManager.getSize() << std::endl;
}
