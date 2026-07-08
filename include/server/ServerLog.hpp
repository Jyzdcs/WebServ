#ifndef SERVER_LOG_HPP
#define SERVER_LOG_HPP

#include <string>
#include <unistd.h>

class ServerLog
{
public:
	static void listening(const std::string& host, int port, const std::string& serverName);
	static void clientConnected(int fd, int port);
	static void clientEvent(int fd, const char* reason);
	static void virtualHost(const std::string& host, const std::string& serverName);
	static void httpResponse(const std::string& rawRequest, const std::string& rawResponse);
	static void cgiStarted(const std::string& rawRequest, pid_t pid);
	static void cgiFinished(const std::string& rawResponse, bool timedOut);
	static void shuttingDown();
};

#endif
