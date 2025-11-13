#ifndef SERVER_HPP
# define SERVER_HPP

#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>
#include <stdexcept>
#include <signal.h>
#include <poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <map>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
	private:
		int _sock_fd;
		int _port;
		std::string _password;
		std::map<int, Client> _clients; // Map of client FDs to Client data
		std::map<int, Channel> _channels;


		void createSocket();
		void bindAndListen();
		void createChannel(void);

	public:
		Server(int port, const std::string &password);
		~Server();
		void run();
		std::map<int, Client>& getClients();
		const std::string& getPassword() const;
		void broadcast(const std::string& msg, int exclude_fd = -1);
};

#endif