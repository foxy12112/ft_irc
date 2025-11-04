#include "../../include/Server.hpp"

Server::Server(int port, const std::string &password) : _sock_fd(-1), _port(port), _password(password)
{
	createSocket();
	bindAndListen();
}

Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
}

void Server::createSocket()
{
	_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
	if (_sock_fd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Set socket options
}

void Server::bindAndListen()
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("Bind failed");;
	if (listen(_sock_fd, 5) < 0)
		throw std::runtime_error("Listen failed");
	
	std::cout << "Listening on port " << _port << std::endl;
}