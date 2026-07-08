#include "../../include/server/ServerLog.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

static void parseRequestLine(const std::string& raw, std::string& method, std::string& uri)
{
	method.clear();
	uri.clear();
	std::size_t end = raw.find("\r\n");
	if (end == std::string::npos)
		return;
	std::string line = raw.substr(0, end);
	std::size_t s1 = line.find(' ');
	if (s1 == std::string::npos)
		return;
	std::size_t s2 = line.find(' ', s1 + 1);
	if (s2 == std::string::npos)
		return;
	method = line.substr(0, s1);
	uri = line.substr(s1 + 1, s2 - s1 - 1);
}

static int parseStatusCode(const std::string& rawResponse)
{
	if (rawResponse.size() < 12 || rawResponse.compare(0, 5, "HTTP/") != 0)
		return 0;
	std::size_t sp = rawResponse.find(' ');
	if (sp == std::string::npos)
		return 0;
	std::size_t sp2 = rawResponse.find(' ', sp + 1);
	std::string code = rawResponse.substr(sp + 1,
		sp2 == std::string::npos ? std::string::npos : sp2 - sp - 1);
	return std::atoi(code.c_str());
}

static std::string parseLocationHeader(const std::string& rawResponse)
{
	std::size_t pos = rawResponse.find("\r\nLocation:");
	if (pos == std::string::npos)
		pos = rawResponse.find("\r\nlocation:");
	if (pos == std::string::npos)
		return "";
	pos += 11;
	while (pos < rawResponse.size() && (rawResponse[pos] == ' ' || rawResponse[pos] == '\t'))
		pos++;
	std::size_t end = rawResponse.find("\r\n", pos);
	if (end == std::string::npos)
		return "";
	return rawResponse.substr(pos, end - pos);
}

void ServerLog::listening(const std::string& host, int port, const std::string& serverName)
{
	std::cout << "Listening on " << host << ":" << port;
	if (!serverName.empty())
		std::cout << " (" << serverName << ")";
	std::cout << std::endl;
}

void ServerLog::clientConnected(int fd, int port)
{
	std::cout << "Client " << fd << " connected (port " << port << ")" << std::endl;
}

void ServerLog::clientEvent(int fd, const char* reason)
{
	if (reason)
		std::cout << "Client " << fd << " " << reason << std::endl;
}

void ServerLog::virtualHost(const std::string& host, const std::string& serverName)
{
	std::cout << "Host: " << host << " → server " << serverName << std::endl;
}

void ServerLog::httpResponse(const std::string& rawRequest, const std::string& rawResponse)
{
	std::string method;
	std::string uri;
	parseRequestLine(rawRequest, method, uri);
	if (method.empty())
		return;

	int status = parseStatusCode(rawResponse);
	if (status == 413)
		std::cout << method << " " << uri << " → 413 (body too large)" << std::endl;
	else if (status == 301 || status == 302) {
		std::string loc = parseLocationHeader(rawResponse);
		if (!loc.empty())
			std::cout << method << " " << uri << " → " << status << " → " << loc << std::endl;
		else
			std::cout << method << " " << uri << " → " << status << std::endl;
	} else
		std::cout << method << " " << uri << " → " << status << std::endl;
}

void ServerLog::cgiStarted(const std::string& rawRequest, pid_t pid)
{
	std::string method;
	std::string uri;
	parseRequestLine(rawRequest, method, uri);
	if (uri.empty())
		std::cout << "CGI started (pid " << pid << ")" << std::endl;
	else
		std::cout << "CGI " << uri << " started (pid " << pid << ")" << std::endl;
}

void ServerLog::cgiFinished(const std::string& rawResponse, bool timedOut)
{
	if (timedOut)
		std::cout << "CGI timeout → 504" << std::endl;
	else
		std::cout << "CGI finished → " << parseStatusCode(rawResponse) << std::endl;
}

void ServerLog::shuttingDown()
{
	std::cout << "Shutting down..." << std::endl;
}
