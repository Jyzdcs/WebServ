// #include "include/server/Socket.hpp"
// #include "include/config/ServerConfig.hpp"
// #include <iostream>
// #include <iostream>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <netdb.h>
// #include <sstream>
// #include <fcntl.h>

int main() {
	return 0;
// 	try {
// 		ServerConfig conf;        // adapte à ton code
// 		conf.setPort(8000);       // port de test
// 		conf.setHost("127.0.0.1");

// 		Socket sock(conf);        // va faire socket/bind/listen

// 		std::cout << "Listening on port " << conf.getPort()
// 							<< " — waiting for one client..." << std::endl;

// 		// 1) accepter UNE connexion (bloquant)
// 		struct sockaddr_in client_addr;
// 		socklen_t addrlen = sizeof(client_addr);

// 		int client_fd = ::accept(sock.getFd(),
// 														(struct sockaddr *)&client_addr,
// 														&addrlen);
// 		if (client_fd < 0) {
// 			perror("accept");
// 			return 1;
// 		}

// 		std::cout << "Client connected, waiting for data..." << std::endl;

// 		// 2) lire une fois (bloquant)
// 		char buf[1024];
// 		int n = ::recv(client_fd, buf, sizeof(buf) - 1, 0);
// 		if (n < 0) {
// 			perror("recv");
// 			::close(client_fd);
// 			return 1;
// 		}
// 		buf[n] = '\0';
// 		std::cout << "Received: " << buf << std::endl;

// 		// 3) répondre
// 		const char *msg = "Hello from test server!\n";
// 		::send(client_fd, msg, std::strlen(msg), 0);

// 		::close(client_fd);
// 		std::cout << "Done, exiting." << std::endl;
// 	} catch (std::exception &e) {
// 		std::cerr << "Exception: " << e.what() << std::endl;
		// return 1;
// 	}
}