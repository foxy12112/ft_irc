#include "newServer.hpp"

// Sends welcome numerics to authenticated clients
void Server::sendWelcome(Client &cli) {
	// Prevent duplicate welcomes
	if (!cli.getAuth() || cli.getUserName().empty() || cli.getNickName().empty())
		return;
	if (cli.getCon() == false)
		return;
	static std::set<int> welcomed;
	if (welcomed.count(cli.getFd())) return;
	welcomed.insert(cli.getFd());
	std::string host = _hostname;
	cli.queueResponse(":" + host + " 001 " + cli.getNickName() + " :Welcome to the IRC server " + cli.getNickName() + "!" + cli.getUserName() + "@" + host + "\r\n");
	cli.queueResponse(":" + host + " 002 " + cli.getNickName() + " :Your host is " + host + ", running version 1.0\r\n");
	cli.queueResponse(":" + host + " 003 " + cli.getNickName() + " :This server was created today\r\n");
	cli.queueResponse(":" + host + " 004 " + cli.getNickName() + " " + host + " 1.0 * *\r\n");
}

// Creates and configures the server socket
void Server::createSocket()
{
	_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
	if (_sock_fd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Set socket options
	fcntl(_sock_fd, F_SETFL, O_NONBLOCK); // Set socket to non-blocking mode
}

// Binds socket to port and starts listening for connections
void Server::bindAndListen()
{
	struct sockaddr_in addr; // Server address structure
	memset(&addr, 0, sizeof(addr)); // Zero out the structure
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(_port); // Convert port to network byte  order
	addr.sin_addr.s_addr= INADDR_ANY; // Accept connections from any address

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) // Bind the socket
		throw std::runtime_error("Bind failed");
	if (listen(_sock_fd, 5) < 0) // Listen for incoming connections
		throw std::runtime_error("Listen failed");
	std::cout << "Listening on port " << _port << std::endl;
}

// Initializes server with default channels and topics
Server::Server(int port, const std::string &password)
{
	_sock_fd = -1;
	_port = port;
	_password = password;
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) != 0)
		strncpy(hostname, "localhost", sizeof(hostname));
	_hostname = hostname;

	signal(SIGPIPE, SIG_IGN); // Prevents crash on write to closed socket
	createSocket();
	bindAndListen();
}

// Clean up resources on shutdown
Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
	for (std::map<int, Client>::iterator iter = _clients.begin(); iter != _clients.end(); iter++)
		iter->second.disconnect();
}

// Checks if a nickname or username is already in use by another client
bool Server::isNameInUse(const std::string &name, bool checkNick, int requesterFd)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) // Iterate through clients
	{
		if (it->first == requesterFd) // If checking own nickname/username, skip
			continue;
		if (checkNick)
		{
			if (name == it->second.getNickName()) // If nickname is in use, log and return true
			{
				std::cout << "[nick in use] fd=" << requesterFd << " tried '" << name << "' but fd=" << it->first << " already uses that nick\n";
				return true;
			}
		}
		else
		{
			if (name == it->second.getUserName()) // If username is in use, log and return true
			{
				std::cout << "[user in use] fd=" << requesterFd << " tried '" << name << "' but fd=" << it->first << " already uses that username\n";
				return true;
			}
		}
	}
	return false;
}

// Sends a message to all members of a channel
void	Server::sendToChannel(std::string msg, int channelIndex)
{
	std::string out = msg;
	if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n") // Ensure message ends with CRLF
		out += "\r\n";
	std::map<int, Channel>::iterator chIt = _channels.find(channelIndex);
	if (chIt == _channels.end())
		return;
	const std::set<int> &members = chIt->second.members();
	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		std::map<int, Client>::iterator cit = _clients.find(*it);
		if (cit != _clients.end())
			cit->second.queueResponse(out);
	}
}

// Finds a channel by name and returns its index, or -1 if not found
int		Server::findChannel(std::string channel)
{
	for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		if (it->second.getName() == channel)
			return it->first;
	return (-1);
}

// Finds a client by nickname and returns a reference, or throws if not found
Client	&Server::findClient(std::string client)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (client == it->second.getNickName())
			return it->second;
	throw std::runtime_error("Client not found");
}

// Dispatches authenticated client commands
void	Server::Commands(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	Command c = stringToCommand(cmd);
	if (cmd.find("PING") == 0)
		cli.queueResponse("PONG :" + host + "\r\n");
	else
		switch(c)
		{
			case CMD_KICK: Kick(cmd, cli) ; break;
			case CMD_INVITE: Invite(cmd, cli); break;
			case CMD_TOPIC: Topic(cmd, cli); break;
			case CMD_MODE: Mode(cmd, cli);break;
			case CMD_JOIN: Join(cmd, cli); break;
			case CMD_NICK: Nick(cmd, cli); break;
			case CMD_USER: User(cmd, cli); break;
			case CMD_MSG: Message(cmd, cli); break;
			case CMD_WHOIS: Whois(cmd, cli); break;
			case CMD_OPER: oper(cmd, cli); break;
			default: break ;
		}
}

// Handles pre-authentication commands (PASS, NICK, USER)
void	Server::WelcomeCommands(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	if (cmd.find("PASS ") == 0 && !cli.getAuth())
	{
		if (cmd.substr(5) == _password)
			cli.setAuth(true);
		else
		{
			cli.queueResponse(":" + host + " 464 " + cli.getNickName() + " :Password incorrect\r\n");
			cli.setCon(false);
			cli.disconnect(); // Immediately close the socket
		}
	}
	else if (cli.getUserName().empty())
		if (cmd.find("USER ") == 0)
			User(cmd, cli);
	       if ((!_password.empty() ? cli.getAuth() : true) && !cli.getUserName().empty() && !cli.getNickName().empty())
	       {
		       std::cout << "[welcome] fd=" << cli.getFd() << " nick='" << cli.getNickName() << "' user='" << cli.getUserName() << "\n";
		       sendWelcome(cli);
	       }
}

// Initializes server with default channels and topics
void	Server::createChannel()
{
	this->_channels[0] = Channel("#General", "idk basic channel ig");
	this->_channels[1] = Channel("#Coffebreak", "Casual chat for developers, designers, and night owls");
	this->_channels[2] = Channel("#retrocomputing", "Everything about old PCs, terminals, and vintage OSes");
	this->_channels[3] = Channel("#Operator channel", "Channel only for operators");
	this->_channels[4] = Channel("#zenmode", "A calm space for meditation, mindfulness, and philosophy talk");
}

// Main server loop: handles connections and client commands
void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	createChannel();
	while(true)
	{
		if (poll(&fds[0], fds.size(), -1) < 0) // Wait for events on the sockets
			throw std::runtime_error("Poll failed"); // Handle poll errors when they occur
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) // Handle socket errors and disconnections
			{
				if (fds[i].fd != _sock_fd)
				{
					Client &cli = _clients[fds[i].fd];
					cli.setCon(false);
					std::cout << "Client fd=" << fds[i].fd << " disconnected (poll error)" << std::endl;
					cli.disconnect();
					_clients.erase(fds[i].fd);
					fds.erase(fds.begin() + i);
					--i;
					continue ;
				}
			}
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == _sock_fd) // New client connection on the listening socket
				{
					struct sockaddr_in addr; // Client address structure for accept
					socklen_t len = sizeof(addr); // Length of the address structure
					int client_fd = accept(_sock_fd, (struct sockaddr*)&addr, &len); // Accept a new client connection
					std::cout << "Client connected, fd=" << client_fd << std::endl; // Log the new connection
					if (client_fd < 0)
						continue ;
					fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set the new client socket to non-blocking mode
					struct pollfd pfd = {client_fd, POLLIN, 0}; // Create a pollfd structure for the new client
					fds.push_back(pfd); // Add the new client to the list of monitored sockets
					_clients[client_fd] = Client(client_fd, _hostname); // Create a new Client object for the new connection
				}
				else // Data available to read from an existing client
				{
					Client &cli = _clients[fds[i].fd];
					int bytes = cli.receive(_clients[fds[i].fd]);
					if (bytes <= 0 && !cli.getCon())
					{
						std::cout << "Client fd=" << fds[i].fd << " disconnected" << std::endl;
						cli.disconnect();
						_clients.erase(fds[i].fd);
						fds.erase(fds.begin() + i);
						i--;
						continue ;
					}
					std::string cmd;
					while(cli.extractNextCommand(cmd))
					{
						std::cout << cli.getChannelIndex() << "\t\t" << cmd << std::endl; // Log the received command for debugging
						if (cmd.empty())
							continue;
						if (cmd.find("CAP LS") == 0) //client will send request for a list of available capabilities
							cli.queueResponse(":" + _hostname + " CAP * LS :\r\n"); //send empty capability list since we dont have any
						if (cmd.find("NICK ") == 0)
						{
							Nick(cmd, cli);
							continue;
						}
						
						if (cmd.find("CAP END") == 0 && !cli.getNickName().empty() && !cli.getUserName().empty() && cli.getAuth())
						{
							sendWelcome(cli);
						}
						if (!cli.getAuth() || cli.getUserName().empty())
							WelcomeCommands(cmd, cli);
						else if (cli.getAuth() && !cli.getUserName().empty())
							Commands(cmd, cli);
						else if (cli.getAuth() == false && cli.getUserName().empty())
							cli.queueResponse(":" + _hostname + " 464: Authenticate first\r\n");
					}
				}
			}
			if (fds[i].revents & POLLOUT && fds[i].fd != _sock_fd)
			{
				Client &cli = _clients[fds[i].fd];
				cli.flushSend();
				if (!cli.getCon())
				{
					std::cout << "Client fd=" << fds[i].fd << " disconnected during send" << std::endl;
					cli.disconnect();
					_clients.erase(fds[i].fd);
					fds.erase(fds.begin() + i);
					--i;
					continue ;
				}
			}
		}
		for (size_t j = 1; j < fds.size(); j++)
		{
			std::map<int, Client>::iterator it = _clients.find(fds[j].fd);
			if (it == _clients.end())
				continue ;
			fds[j].events = POLLIN | (it->second.hasDataToSend() ? POLLOUT : 0);
		}
	}
}