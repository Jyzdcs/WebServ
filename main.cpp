#include "include/server/Server.hpp"
#include <iostream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <fcntl.h>
#include "include/config/ConfigValidator.hpp"

int main(int argc, char **argv) {
	if (argc > 2)
	{
		std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
		return 1;
	}
	std::string path = (argc == 2) ? argv[1] : "config/default.conf";

	Config config;
	try
	{
		ConfigParser parser;
		ConfigValidator validator;

		config = parser.parse(path);
		validator.validate(config);
	}
	catch (const std::exception& e)
	{
		std::cerr << "webserv: " << path << ": " << e.what() << std::endl;
		return 1;
	}
	const std::vector<ServerConfig>& serversConf = config.getServers();
	std::cout << "Config OK: " << serversConf.size() << " server(s)" << std::endl;
	for (size_t i = 0; i < serversConf.size(); ++i)
	{
		std::cout << "  - " << serversConf[i].getHost() << ":" << serversConf[i].getPort()
							<< " (" << serversConf[i].getLocations().size() << " location(s))"
							<< std::endl;
	}
	try {
		Server server;

		server.addServerConfig(serversConf[0]);
		std::cout << "Listening on port " << serversConf[0].getPort()
		<< " — waiting for one client..." << std::endl;
		server.run();
	} catch (std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}