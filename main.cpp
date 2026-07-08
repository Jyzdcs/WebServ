#include "include/server/Server.hpp"
#include <iostream>
#include <csignal>
#include "include/config/ConfigValidator.hpp"

int main(int argc, char **argv) {
	if (argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
		return 1;
	}
	std::string path = (argc == 2) ? argv[1] : "config/default.conf";

	Config config;
	try {
		ConfigParser parser;
		ConfigValidator validator;
		config = parser.parse(path);
		validator.validate(config);
	} catch (const std::exception& e) {
		std::cerr << "webserv: " << path << ": " << e.what() << std::endl;
		return 1;
	}

	const std::vector<ServerConfig>& serversConf = config.getServers();
	std::cout << "Config OK: " << serversConf.size() << " server(s)" << std::endl;

	try {
		Server server;
		signal(SIGINT,  Server::sigHandler);
		signal(SIGTERM, Server::sigHandler);
		for (size_t i = 0; i < serversConf.size(); i++)
			server.addServerConfig(serversConf[i]);
		std::cout << "Server ready." << std::endl;
		server.run();
	} catch (std::exception &e) {
		std::cerr << "webserv: " << e.what() << std::endl;
		return 1;
	}

	std::cout << "Server stopped." << std::endl;
	return 0;
}
