// #include "include/server/Socket.hpp"
// #include "include/server/Client.hpp"
// #include "include/config/ServerConfig.hpp"
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
// #include "include/http/processHttp.hpp"
// #include "include/config/ServerConfig.hpp"
// #include "include/config/ConfigParser.hpp"

int main(int ac, char **av) {
	try {
		const std::string &path = (ac > 1) ? av[1] : "config/default.conf";
		ConfigParser parser;
		Config config = parser.parse(path);
		const std::vector<ServerConfig> &servers = config.getServers();
		if (servers.empty()) {
				std::cerr << "No server configured in " << path << std::endl;
				return 1;
		}
		const ServerConfig &serverConf = servers[0];
		Server server;
		// Socket sock(serverConf);        // va faire socket/bind/listen

		server.addServerConfig(serverConf);
		std::cout << "Listening on port " << serverConf.getPort()
		<< " — waiting for one client..." << std::endl;
		server.run();

		// // 1) accepter UNE connexion (bloquant)
		// struct sockaddr_in client_addr;
		// socklen_t addrlen = sizeof(client_addr);

		// int client_fd = ::accept(sock.getFd(), (struct sockaddr *)&client_addr, &addrlen);
		// if (client_fd < 0) {
		// 	perror("accept");
		// 	return 1;
		// }

		// std::cout << "Client connected, waiting for data..." << std::endl;
		// Client newClient(client_fd, serverConf.getPort());
		// newClient.receiveData();
		// newClient.setWriteBuffer(processHttp(newClient.getReadBuffer(), serverConf));
		// newClient.sendData();
		// ::close(client_fd);
	} catch (std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}