#include "include/server/PollManager.hpp"

// Ce test vite si la detection isReadable fonctionne ainsi que le fait de pouvoir switch pour ecouter le writtable

int main(void)
{
	PollManager pollManager;

	// std::cout << pollManager.getSize() << std::endl;
	pollManager.addFd(0, POLLIN);
	int pollNum = pollManager.pollEngine(-1);
	std::cout << pollManager.isReadable(0) << std::endl;
	pollManager.updateEvents(0, POLLOUT);
	pollNum = pollManager.pollEngine(-1);
	std::cout << pollManager.isWritable(0) << std::endl;
}
