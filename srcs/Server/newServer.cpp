// Move include to top
#include "newServer.hpp"

// Helper to send full welcome numerics
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

void Server::createSocket()
{
	_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
	if (_sock_fd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Set socket options
	fcntl(_sock_fd, F_SETFL, O_NONBLOCK); // Set socket to non-blocking mode
}

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

Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
	for (std::map<int, Client>::iterator iter = _clients.begin(); iter != _clients.end(); iter++)
		iter->second.disconnect();
}


bool Server::isNameInUse(const std::string &name, bool checkNick, int requesterFd)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->first == requesterFd)
			continue;
		if (checkNick)
		{
			if (name == it->second.getNickName())
			{
				std::cout << "[nick in use] fd=" << requesterFd << " tried '" << name << "' but fd=" << it->first << " already uses that nick\n";
				return true;
			}
		}
		else
		{
			if (name == it->second.getUserName())
			{
				std::cout << "[user in use] fd=" << requesterFd << " tried '" << name << "' but fd=" << it->first << " already uses that username\n";
				return true;
			}
		}
	}
	return false;
}

void	Server::wasInvited(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	if (cmd == "y" || cmd == "accept" || cmd == "yes")
	{
		int newIdx = cli.getInvitedIndex();
		if (newIdx >= 0 && _channels.find(newIdx) != _channels.end())
			_channels[newIdx].addMember(cli.getFd());
		cli.setChannelIndex(newIdx);
	}
	else
	{
		try
		{
			findClient(cli.getInvitedClient()).queueResponse(":"+cli.getUserName()+": declined the invitation\r\n");
		}
		catch (std::exception &)
		{
			cli.queueResponse(":" + host + " 401 " + cli.getNickName() + " " + cli.getInvitedClient() + " :No such nick\r\n");
		}
	}
}

void	Server::sendToChannel(std::string msg, int channelIndex)
{
	std::string out = msg;
	if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n")
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

int		Server::findChannel(std::string channel)
{
	for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		if (it->second.getName() == channel)
			return it->first;
	return (-1);
}

Client	&Server::findClient(std::string client)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (client == it->second.getNickName())
			return it->second;
	throw std::runtime_error("Client not found");
}



//CAP LS needs :your.server.name CAP * LS : as response from server


//after nick and user have been correctly send, and cap ls,
//send welcoming sequence,

//example

//:Server 001 user :Welcome to ft_irc user!hostname

// /kick sends											KICK (optional)channel <usernames> (optional)reason:
												//example:  KICK #General bob,sarah stop flodding!

void	Server::Commands(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	Command c = stringToCommand(cmd);
	// if (cli.getInvited() == true)
	// 	wasInvited(cmd, cli);
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

void	Server::createChannel()
{
	this->_channels[0] = Channel("#General", "idk basic channel ig");
	this->_channels[1] = Channel("#Coffebreak", "Casual chat for developers, designers, and night owls");
	this->_channels[2] = Channel("#retrocomputing", "Everything about old PCs, terminals, and vintage OSes");
	this->_channels[3] = Channel("#Operator channel", "Channel only for operators");
	this->_channels[4] = Channel("#zenmode", "A calm space for meditation, mindfulness, and philosophy talk");
}

void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	createChannel();
	while(true)
	{
		if (poll(&fds[0], fds.size(), -1) < 0)
			throw std::runtime_error("Poll failed");
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
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
				if (fds[i].fd == _sock_fd)
				{
					struct sockaddr_in addr;
					socklen_t len = sizeof(addr);
					int client_fd = accept(_sock_fd, (struct sockaddr*)&addr, &len);
					std::cout << "Client connected, fd=" << client_fd << std::endl;
					if (client_fd < 0)
						continue ;
					fcntl(client_fd, F_SETFL, O_NONBLOCK);
					struct pollfd pfd = {client_fd, POLLIN, 0};
					fds.push_back(pfd);
					_clients[client_fd] = Client(client_fd, _hostname);
				}
				else
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
						std::cout << cli.getChannelIndex() << "\t\t" << cmd << std::endl;
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