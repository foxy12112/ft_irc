#include "newServer.hpp"

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

void	Server::Mode(std::string cmd, Client &cli)
{

	// Determine channel to operate on
	int channelIndex = cli.getChannelIndex();
	std::string param;
	if (cmd.size() > 5)
		param = cmd.substr(5);
	else
		param = "";

	// If a channel name is provided as first token, use it
	std::istringstream iss(param);
	std::string first;
	iss >> first;
	if (!first.empty() && first[0] == '#')
	{
		int idx = findChannel(first);
		if (idx != -1)
			channelIndex = idx;
		// remove the channel token from param so next token is the mode
		std::string rest;
		std::getline(iss, rest);
		// rest may start with a space, trim one leading space
		if (!rest.empty() && rest[0] == ' ')
			rest = rest.substr(1);
		param = rest;
	}

	if (channelIndex < 0 || channelIndex >= (int)_channels.size())
	{
		cli.queueRespone(":server 403 " + cli.getNickName() + " : No such channel\r\n");
		return;
	}

	Channel &channel = _channels[channelIndex];

	// Extract mode flag (first token in param)
	std::string mode;
	std::istringstream iss2(param);
	iss2 >> mode;
	if (mode.empty())
	{
		cli.queueRespone(":server 461 MODE : Not enough parameters\r\n");
		return;
	}

	if (mode == "-i")
	{
		channel.setInvite(!channel.getInvite());
		cli.queueRespone(std::string("Channel: ") + channel.getName() + " has been set to " + (channel.getInvite() ? "invite only\r\n" : "no invite needed\r\n"));
		return;
	}
	else if (mode == "-t")
	{
		channel.setTopicOp(!channel.getTopicOp());
		cli.queueRespone(std::string("Channel: ") + channel.getName() + " Topic can be edited by " + (channel.getTopicOp() ? "operators only\r\n" : "anyone\r\n"));
		return;
	}
	else if (mode == "-k")
	{
		// next token is optional password
		std::string pass;
		iss2 >> pass;
		if (!pass.empty())
		{
			channel.setPass(pass);
			cli.queueRespone("Channel: password has been set\r\n");
		}
		else
		{
			// remove password
			channel.setPass(std::string());
			cli.queueRespone("Channel: password has been removed\r\n");
		}
		return;
	}
	else if (mode == "-o")
	{
		std::string user;
		iss2 >> user;
		if (user.empty())
		{
			cli.queueRespone(":server 461 MODE -o : Not enough parameters\r\n");
			return;
		}
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.getUserName() == user)
			{
				bool newOp = !it->second.getOp();
				it->second.setOp(newOp);
				it->second.queueRespone(std::string("Client: ") + it->second.getUserName() + (newOp ? " is now an operator\r\n" : " is not an operator anymore\r\n"));
				cli.queueRespone(std::string("Client: ") + it->second.getUserName() + (newOp ? " is now an operator\r\n" : " is not an operator anymore\r\n"));
				break;
			}
		}
		return;
	}
	else if (mode == "-l")
	{
		std::string limit;
		iss2 >> limit;
		if (limit.empty())
		{
			channel.setLimit(-1);
			cli.queueRespone("Channel: Limit has been removed\r\n");
		}
		else
		{
			channel.setLimit(std::atoi(limit.c_str()));
			cli.queueRespone(std::string("Channel: Limit has been set to ") + limit + "\r\n");
		}
		return;
	}
	else
	{
		cli.queueRespone(":server 472 MODE : Unknown mode\r\n");
		return;
	}
}

void	Server::Topic(std::string cmd, Client &cli)
{
	std::string param;
	std::istringstream iss(cmd.substr(6));
	
	if (cmd.size() == 5)
		cli.queueRespone(_channels[cli.getChannelIndex()].getTopic());
	else if (cli.getOp() == true || _channels[cli.getChannelIndex()].getTopicOp() == false) //check that topic can be changed by everyone or not, operators can always change topic
	{
		iss >> param;
		if (param == "-delete")//check if target should delete topic
		{
			iss >> param;
			if (param[0] == '#')
				_channels[findChannel(param)].setTopic(NULL);
			else
				_channels[cli.getChannelIndex()].setTopic(NULL);
		}
		else if (param[0] == '#')//either gets the topic of a specific channel or changes it
		{
			int channel = findChannel(param);
			iss >> param;
			if (param.empty())
				cli.queueRespone(_channels[channel].getTopic());
			else
				_channels[channel].setTopic(cmd.substr(6 + _channels[channel].getName().size()));
		}
		else//gets current channeels topic
			_channels[cli.getChannelIndex()].setTopic(cmd.substr(6));
	}
}

void	Server::Kick(std::string cmd, Client &cli)
{
	// if (cli.getOp() == true)
	if (1)
	{
		std::string param = cmd.substr(5);
		std::istringstream iss(param);
		std::string target;
		std::string reason;
		size_t			reason_pos;
		reason_pos = param.find(' ');

		if (reason_pos != std::string::npos)
			reason = param.substr(reason_pos + 1);

		while(std::getline(iss, target, ','))
		{
			findClient(target).queueRespone(reason);
		}
	}
	else
		cli.queueRespone("you are not an operator\r\n");
}

void	Server::wasInvited(std::string cmd, Client &cli)
{
	if (cmd == "y" || cmd == "accept" || cmd == "yes")
		cli.setChannelIndex(cli.getInvitedIndex());
	else
		findClient(cli.getInvitedClient()).queueRespone(":"+cli.getUserName()+": declined the invitation\r\n");
}


void	Server::Invite(std::string cmd, Client &cli)
{
	std::string param;
	std::istringstream iss(cmd.substr(7));
	iss >> param;
	Client &inviting = findClient(param);
	iss >> param;
	if (!param.empty())
	{
		if (findChannel(param) == -1)
		{
			cli.queueRespone(":server 403: channel does not exist\r\n");
			return ;
		}
		inviting.setInvitedIndex(findChannel(param));
		inviting.setInvitedClient(cli.getUserName());
		inviting.queueRespone(":" + cli.getUserName() + " invited you to join the channel: " + _channels[findChannel(param)].getName() + "\r\n");
	}
	else
	{
		inviting.setInvitedIndex(cli.getChannelIndex());
		inviting.setInvitedClient(cli.getUserName());
		inviting.queueRespone(":" + cli.getUserName() + " invited you to join the channel: " + _channels[cli.getChannelIndex()].getName() + "\r\n");
	}
}

void	Server::Join(std::string cmd, Client &cli)
{
	std::string channel = cmd.substr(5);
	int channelIndex = findChannel(channel);

	if (channelIndex == -1)
	{
		cli.queueRespone(":server 403 " + cli.getNickName() + " " + channel + " :No such channel\r\n");
		return ;
	}
	if (_channels[channelIndex].getInvite())
	{
		cli.queueRespone(":server 473 " + cli.getNickName() + " " + channel + " :Cannot join channel (+i)\r\n");
		return ;
	}
	if (_channels[channelIndex].getLimit() != -1 && _channels[channelIndex].getUsers() >= _channels[channelIndex].getLimit())
	{
		cli.queueRespone(":Server 47 1 " + cli.getNickName() + " " + channel + " :Cannot join channel (+l)\r\n");
		return ;
	}
	cli.setChannelIndex(channelIndex);
}

void	Server::Nick(std::string cmd, Client &cli)
{
	std::string nick = cmd.substr(5);
	if (isNameInUse(nick, true, cli.getFd()))
	{
		std::cout << "[nick reject] fd=" << cli.getFd() << " tried '" << nick << "' but name already in use\n";
		cli.queueRespone(":Server 433 * " + nick + " :User- or Nickname is already in use\r\n");
		return;
	}
	std::cout << "[nick set] fd=" << cli.getFd() << " -> nick='" << nick << "'\n";
	sendToChannel(":"+cli.getNickName()+"!~"+cli.getUserName()+"@example.com NICK "+nick, cli.getChannelIndex());
	cli.setNickName(nick);
}

void	Server::User(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::istringstream iss(param);
	std::string user;
	iss >> user;
	if (isNameInUse(user, false, cli.getFd()))
	{
		std::string newUser = user;
		do
		{
			newUser += "_";
		} while (isNameInUse(newUser, false, cli.getFd()));
		std::cout << "[user dup -> adjusted] fd=" << cli.getFd() << " tried '" << user << "' -> assigned '" << newUser << "'\n";
		cli.setUserName(newUser);
		if (!cli.getNickName().empty())
			cli.queueRespone(":server NOTICE " + cli.getNickName() + " :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		else
			cli.queueRespone(":server NOTICE * :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		return;
	}
	std::cout << "[user set] fd=" << cli.getFd() << " -> user='" << user << "'\n";
	cli.setUserName(user);
}

void	Server::Message(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(8);
	std::istringstream iss(param);
	std::string target;
	ssize_t msg_pos = param.find(':');
	if (msg_pos == (ssize_t)std::string::npos)
		return;
	iss >> target;
	std::string message = param.substr(msg_pos + 1);
	std::string prefix = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@example.com";
	if (!target.empty() && target[0] == '#')
	{
		int chidx = findChannel(target);
		if (chidx == -1)
			return;
		std::string out = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.getChannelIndex() == chidx)
			{
				if (it->first == cli.getFd())
					continue;
				it->second.queueRespone(out);
			}
		}
	}
	else
	{
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.getNickName() == target || it->second.getUserName() == target)
			{
				std::string out = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
				it->second.queueRespone(out);
				break;
			}
		}
	}
}

void	Server::Whois(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(6);
	std::istringstream iss(param);
	std::string target;
	iss >> target;
	if (target.empty())
	{
		cli.queueRespone(":server 431 " + cli.getNickName() + " :No nickname given\r\n");
		return;
	}
	Client *targetCli = NULL;
	try
	{
		Client &c = findClient(target);
		targetCli = &c;
	}
	catch (std::exception &e)
	{
		cli.queueRespone(":server 401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
		return;
	}

	// RPL_WHOISUSER 311
	std::string tn = targetCli->getNickName();
	std::string tu = targetCli->getUserName();
	std::string tr = targetCli->getRealName();
	std::string host = "example.com";
	cli.queueRespone(":server 311 " + cli.getNickName() + " " + tn + " " + tu + " " + host + " * :" + tr + "\r\n");

	// RPL_WHOISCHANNELS 319 - list channels the user is in
	std::string chlist;
	int cidx = targetCli->getChannelIndex();
	for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->first == cidx)
		{
			if (!chlist.empty()) chlist += " ";
			chlist += it->second.getName();
		}
	}
	cli.queueRespone(":server 319 " + cli.getNickName() + " " + tn + " :" + chlist + "\r\n");

	// RPL_WHOISOP 313 if operator
	if (targetCli->getOp())
		cli.queueRespone(":server 313 " + cli.getNickName() + " " + tn + " :is an IRC operator\r\n");

	// RPL_ENDOFWHOIS 318
	cli.queueRespone(":server 318 " + cli.getNickName() + " " + tn + " :End of /WHOIS list\r\n");
}

void	Server::sendToChannel(std::string msg, int channelIndex)
{
	std::string out = msg;
	if (out.size() < 2 || out.substr(out.size() - 2) != "\r\n")
		out += "\r\n";
	for (std::map<int, Client>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
		if (it->second.getChannelIndex() == channelIndex)
			it->second.queueRespone(out);
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
		if (client == it->second.getUserName() || client == it->second.getNickName())
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
	Command c = stringToCommand(cmd);
	if (cmd.find("PING server") == 0)
		cli.queueRespone("PONG server\r\n");
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
			default: break ;
		}
}

void	Server::WelcomeCommands(std::string cmd, Client &cli)
{
	if (cmd.find("PASS ") == 0 && !cli.getAuth())
	{
		if (cmd.substr(5) == _password)
			cli.setAuth(true);
		else
			cli.queueRespone(":server 464 :Password incorrect\r\n");
	}
	else if (cli.getUserName().empty())
		if (cmd.find("USER ") == 0)
			User(cmd, cli);
	if ((!_password.empty() ? cli.getAuth() : true) && !cli.getUserName().empty() && !cli.getNickName().empty())
	{
		std::cout << "[welcome] fd=" << cli.getFd() << " nick='" << cli.getNickName() << "' user='" << cli.getUserName() << "'\n";
		cli.queueRespone(":server 001 " + cli.getNickName() + " :Welcome to the IRC server\r\n");
	}
}

void	Server::createChannel()
{
	this->_channels[0] = Channel("#General", "idk basic channel ig");
	this->_channels[1] = Channel("#Coffebreak", "Casual chat for developers, designers, and night owls");
	this->_channels[2] = Channel("#retrocomputing", "Everything about old PCs, terminals, and vintage OSes");
	this->_channels[3] = Channel("#Operator channel", "Channel only for operators");
	this->_channels[4] = Channel("#zenmode", "A calm space for meditation, mindfulness, and philosophy talk");
	this->_channels[5] = Channel("purgatory", "this is just a purgatory for people who have been kicked from their channel");
}

void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	createChannel();
	_channels[5].setInvite(true);
	while(true)
	{
		if (poll(&fds[0], fds.size(), -1) < 0)
			throw std::runtime_error("Poll failed");
		for (size_t i = 0; i < fds.size(); i++)
		{
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
					_clients[client_fd] = Client(client_fd);
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
							cli.queueRespone(":server CAP * LS :\r\n"); //send empty capability list since we dont have any
						{
							Nick(cmd, cli);
							continue;
						}
						if (!cli.getAuth() || cli.getUserName().empty())
							WelcomeCommands(cmd, cli);
						else if (cli.getAuth() && !cli.getUserName().empty())
							Commands(cmd, cli);
						else if (cli.getAuth() == false && cli.getUserName().empty())
							cli.queueRespone(":server 464: Authenticate first\r\n");
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