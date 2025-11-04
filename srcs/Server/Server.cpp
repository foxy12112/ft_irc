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
	struct sockaddr_in addr; // Server address structure
	memset(&addr, 0, sizeof(addr)); // Zero out the structure
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(_port); // Convert port to network byte order
	addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) // Bind the socket
		throw std::runtime_error("Bind failed");;
	if (listen(_sock_fd, 5) < 0) // Listen for incoming connections
		throw std::runtime_error("Listen failed");
	
	std::cout << "Listening on port " << _port << std::endl;
}

void Server::run()
{
	while (true) // Main server loop
	{
		struct sockaddr_in client_addr; // Client address structure
		socklen_t len = sizeof(client_addr); // Length of the client address structure
		int client_fd = accept(_sock_fd, (struct sockaddr*)&client_addr, &len); // Accept a new connection
		if (client_fd < 0) // Server continues to run even if accept fails
			continue ;

		char kiloBuf[1024]; // Buffer for data of 1024 bytes
		size_t bytes; // Number of bytes received
		while ((bytes = recv(client_fd, kiloBuf, sizeof(kiloBuf) - 1, 0)) > 0) // Receive data from the client
		{
			kiloBuf[bytes] = '\0';
			send(client_fd, kiloBuf, bytes, 0); // Echo back the received data
		}
		close(client_fd); // Close the client connection
	}
}