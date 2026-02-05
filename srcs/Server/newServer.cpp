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
	std::istringstream iss(cmd.substr(5));
	std::string channelName;
	std::string modeString;
	iss >> channelName >> modeString;

	int channelIndex = cli.getChannelIndex();
	if (!channelName.empty() && channelName[0] == '#')
	{
		int idx = findChannel(channelName);
		if (idx != -1)
			channelIndex = idx;
	}
	std::map<int, Channel>::iterator chIt = _channels.find(channelIndex);
	if (chIt == _channels.end())
	{
		cli.queueResponse(":server 403 " + cli.getNickName() + " :No such channel\r\n");
		return;
	}
	Channel &channel = chIt->second;

	if (modeString.empty())
	{
		cli.queueResponse(":server 461 MODE :Not enough parameters\r\n");
		return;
	}

	if (!channel.hasMember(cli.getFd()))
	{
		cli.queueResponse(":server 442 " + cli.getNickName() + " " + channel.getName() + " :You're not on that channel\r\n");
		return;
	}
	if (!channel.isOperator(cli.getFd()))
	{
		cli.queueResponse(":server 482 " + channel.getName() + " :You must be a channel operator\r\n");
		std::cout << "cant do\n";
		return;
	}
	std::string modes;
	std::string params;
	bool adding = false;

	for (size_t i = 0; i < modeString.size(); i++)
	{
		char c = modeString[i];
		if (c == '+')
			adding = true;
		else if (c == '-')
			adding = false;
		else if (c == 'i')
		{
			channel.setInvite(adding);
			modes += (adding ? '+' : '-');
			modes += 'i';
		}
		else if (c == 't')
		{
			bool enableTopicRestriction = adding ? true : !channel.getTopicOp();
			channel.setTopicOp(enableTopicRestriction);
			modes += (enableTopicRestriction ? '+' : '-');
			modes += 't';
		}
		else if (c == 'k')
		{
			if (adding)
			{
				std::string pass;
				iss >> pass;
				if (!pass.empty())
				{
					channel.setPass(pass);
					modes += '+';
					modes += 'k';
					params += " " + pass;
				}
			}
			else
			{
				channel.setPass("");
				modes += '-';
				modes += 'k';
			}
		}
		else if (c == 'l')
		{
			if (adding)
			{
				std::string limitStr;
				iss >> limitStr;
				int limit = std::atoi(limitStr.c_str());
				if (limit > 0)
				{
					channel.setLimit(limit);
					modes += '+';
					modes += 'l';
					params += " " + limitStr;
				}
			}
			else
			{
				channel.setLimit(-1);
				modes += '-';
				modes += 'l';
			}
		}
		else if (c == 'o')
		{
			std::string user;
			iss >> user;
			if (!user.empty())
			{
				for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
				{
					if (it->second.getUserName() == user || it->second.getNickName() == user)
					{
						channel.setOperator(it->first, adding);
						modes += (adding ? '+' : '-');
						modes += 'o';
						params += " " + it->second.getNickName();
						break;
					}
				}
			}
		}
	}

	if (!modes.empty())
	{
		std::string modeMsg = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 MODE " + channel.getName() + " " + modes + params + "\r\n";
		sendToChannel(modeMsg, channelIndex);
	}
	else
		cli.queueResponse(":server 472 MODE :Unknown mode\r\n");
}

void	Server::Topic(std::string cmd, Client &cli)
{
	std::string param;
	int channelIndex = cli.getChannelIndex();
	
	if (cmd.size() == 5)
	{
		cli.queueResponse(_channels[channelIndex].getTopic());
		return;
	}

	bool isChannelOp = _channels[channelIndex].isOperator(cli.getFd());
	bool canModifyTopic = isChannelOp || !_channels[channelIndex].getTopicOp();
	
	if (!canModifyTopic)
	{
		cli.queueResponse(":server 482 " + _channels[channelIndex].getName() + " :You must be a channel operator\r\n");
		return;
	}

	std::string topicText = cmd.substr(6);
	
	if (topicText == "-delete")
	{
		_channels[channelIndex].setTopic("");
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 TOPIC " + _channels[channelIndex].getName() + " :\r\n", channelIndex);
	}
	else
	{
		_channels[channelIndex].setTopic(topicText);
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 TOPIC " + _channels[channelIndex].getName() + " :" + topicText + "\r\n", channelIndex);
	}
}

void Server::Kick(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5); // Remove "KICK "
	std::istringstream iss(param);
	std::string channel;
	std::string targets;
	std::string reason = "Womp Womp";

	// Parse: KICK #channel user1,user2,user3 :optional reason
	iss >> channel >> targets;
	
	// Get reason if provided (starts after ":")
	size_t reason_pos = param.find(" :");
	if (reason_pos != std::string::npos)
		reason = param.substr(reason_pos + 2); // Skip " :"

	// Validate channel format
	if (channel.empty() || channel[0] != '#')
	{
		cli.queueResponse("403 " + channel + " :No such channel\r\n");
		return;
	}

	// Validate channel exists and issuing user is on it
	int channelIndex = findChannel(channel);
	if (channelIndex == -1)
	{
		cli.queueResponse("403 " + cli.getNickName() + " " + channel + " :No such channel\r\n");
		return;
	}
	if (!_channels[channelIndex].hasMember(cli.getFd()))
	{
		cli.queueResponse("442 " + cli.getNickName() + " " + channel + " :You're not on that channel\r\n");
		return;
	}
	if (!_channels[channelIndex].isOperator(cli.getFd()))
	{
		cli.queueResponse(":server 482 " + channel + " :You must be a channel operator\r\n");
		return;
	}

	// Split targets by comma
	std::istringstream target_stream(targets);
	std::string target;
	
	while (std::getline(target_stream, target, ','))
	{
		// Trim whitespace
		target.erase(0, target.find_first_not_of(" \t\r\n"));
		target.erase(target.find_last_not_of(" \t\r\n") + 1);
		
		if (target.empty())
			continue;

		std::cout << "Kicking " << target << " from " << channel << " (reason: " << reason << ")" << std::endl;
		
		// Find the client to kick
		try
		{
			Client &kickedClient = findClient(target);
			if (!_channels[channelIndex].hasMember(kickedClient.getFd()))
			{
				cli.queueResponse("441 " + cli.getNickName() + " " + target + " " + channel + " :They aren't on that channel\r\n");
				continue;
			}
			
			// Build KICK message with full prefix
			std::string kickPrefix = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1";
			std::string kickMsg = kickPrefix + " KICK " + channel + " " + target + " :" + reason + "\r\n";

			// First, remove from channel membership
			_channels[channelIndex].removeMember(kickedClient.getFd());
			// Then, broadcast KICK to channel members (target excluded now)
			sendToChannel(kickMsg, channelIndex);
		// Deliver explicit KICK and NOTICE to the kicked user
		kickedClient.queueResponse(kickMsg);
		kickedClient.queueResponse(":server NOTICE " + kickedClient.getNickName() + " :You were kicked from " + channel + " (" + reason + ")\r\n");
		}
		catch (...)
		{
			cli.queueResponse("401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
		}
	}
}

void	Server::wasInvited(std::string cmd, Client &cli)
{
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
			cli.queueResponse(":server 401 " + cli.getNickName() + " " + cli.getInvitedClient() + " :No such nick\r\n");
		}
	}
}


void Server::Invite(std::string cmd, Client &cli)
{
	std::istringstream iss(cmd.substr(7));
	std::string targetNick;
	std::string channelName;
	iss >> targetNick >> channelName;

	if (targetNick.empty())
	{
		cli.queueResponse(":server 461 INVITE :Not enough parameters\r\n");
		return;
	}

	Client *invitee = NULL;
	try
	{
		invitee = &findClient(targetNick);
	}
	catch (...)
	{
		cli.queueResponse(":server 401 " + cli.getNickName() + " " + targetNick + " :No such nick\r\n");
		return;
	}

	if (channelName.empty() && _channels.count(cli.getChannelIndex()))
		channelName = _channels[cli.getChannelIndex()].getName();

	int chIdx = findChannel(channelName);
	if (channelName.empty() || chIdx == -1) 
	{
		cli.queueResponse(":server 403 " + cli.getNickName() + " " + channelName + " :No such channel\r\n");
		return;
	}

	if (!_channels[chIdx].isOperator(cli.getFd()))
	{
		cli.queueResponse(":server 482 " + channelName + " :You must be a channel operator\r\n");
		return;
	}

	invitee->setInvitedIndex(chIdx);
	invitee->setInvitedClient(cli.getUserName());
	invitee->setWasInvited(true);

	std::string host = "127.0.0.1";
	cli.queueResponse(":server 341 " + cli.getNickName() + " " + invitee->getNickName() + " " + channelName + "\r\n");
	std::string prefix = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host;
	invitee->queueResponse(prefix + " INVITE " + invitee->getNickName() + " :" + channelName + "\r\n");
}

void	Server::Join(std::string cmd, Client &cli)
{
	std::istringstream iss(cmd.substr(5));
	std::string channel;
	std::string password;
	iss >> channel >> password;

	int channelIndex = findChannel(channel);

	if (channelIndex == -1)
	{
		cli.queueResponse(":server 403 " + cli.getNickName() + " " + channel + " :No such channel\r\n");
		return ;
	}

	if (_channels[channelIndex].getInvite())
	{
		if (!cli.getInvited() || cli.getInvitedIndex() != channelIndex)
		{
			cli.queueResponse(":server 473 " + cli.getNickName() + " " + channel + " :Cannot join channel (+i)\r\n");
			return ;
		}
		cli.setWasInvited(false);
		cli.setInvitedIndex(-1);
	}
	
	int currentUsers = _channels[channelIndex].members().size();
	if (_channels[channelIndex].getLimit() != -1 && currentUsers >= _channels[channelIndex].getLimit())
	{
		cli.queueResponse(":Server 471 " + cli.getNickName() + " " + channel + " :Cannot join channel (+l)\r\n");
		return ;
	}
	
	if (!_channels[channelIndex].getPass().empty())
	{
		if (password != _channels[channelIndex].getPass())
		{
			cli.queueResponse(":server 475 " + cli.getNickName() + " " + channel + " :Cannot join channel (+k)\r\n");
			return ;
		}
	}
	_channels[channelIndex].addMember(cli.getFd());
	cli.setChannelIndex(channelIndex);
}

void    Server::Nick(std::string cmd, Client &cli)
{
    std::string nick = cmd.substr(5);
    if (isNameInUse(nick, true, cli.getFd()))
    {
        std::string newNick = nick;
        do {
            newNick += "_";
        } while (isNameInUse(newNick, true, cli.getFd()));
        std::cout << "[nick dup -> adjusted] fd=" << cli.getFd() << " tried '" << nick << "' -> assigned '" << newNick << "'\n";
        cli.setNickName(newNick);
        if (!cli.getUserName().empty())
            cli.queueResponse(":server NOTICE " + cli.getNickName() + " :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        else
            cli.queueResponse(":server NOTICE * :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        return;
    }
    std::cout << "[nick set] fd=" << cli.getFd() << " -> nick='" << nick << "'\n";
    sendToChannel(":"+cli.getNickName()+"!~"+cli.getUserName()+"@127.0.0.1 NICK "+nick, cli.getChannelIndex());
    cli.setNickName(nick);
}

void	Server::oper(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::string name;
	std::string password;
	std::istringstream iss(param);
	iss >> name;
	iss >> password;
	Client *clintPtr = NULL;
	try
	{
		Client &clint = findClient(name);
		clintPtr = &clint;
	}
	catch (std::exception &)
	{
		cli.queueResponse(":server 401 " + cli.getNickName() + " " + name + " :No such nick\r\n");
		return;
	}
	if (password == "operpass")
	{
		bool newStatus = !clintPtr->getServerOp();
		clintPtr->setServerOp(newStatus);
		for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
			it->second.setOperator(clintPtr->getFd(), newStatus);
	}
	else
		clintPtr->queueResponse(":server 464 "+clintPtr->getNickName()+" :Password incorrect\r\n");
	if (clintPtr->getServerOp() == true)
		clintPtr->queueResponse(":server 381 " + clintPtr->getNickName() + " :You are now an IRC operator\r\n");
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
			cli.queueResponse(":server NOTICE " + cli.getNickName() + " :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		else
			cli.queueResponse(":server NOTICE * :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
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
		// Prevent non-members from speaking in the channel using membership set
		if (!_channels[chidx].hasMember(cli.getFd()))
		{
			cli.queueResponse("442 " + cli.getNickName() + " " + target + " :You're not on that channel\r\n");
			return;
		}
		std::string out = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
		const std::set<int> &members = _channels[chidx].members();
		for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
		{
			if (*it == cli.getFd()) continue;
			std::map<int, Client>::iterator cit = _clients.find(*it);
			if (cit != _clients.end())
				cit->second.queueResponse(out);
		}
	}
	else
	{
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.getNickName() == target || it->second.getUserName() == target)
			{
				std::string out = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
				it->second.queueResponse(out);
				break;
			}
		}
	}
}

void	Server::Whois(std::string cmd, Client &cli)
{
	// Extract target safely even when WHOIS is sent without arguments
	size_t spacePos = cmd.find(' ');
	std::string param = (spacePos == std::string::npos || spacePos + 1 >= cmd.size()) ? std::string() : cmd.substr(spacePos + 1);
	std::istringstream iss(param);
	std::string target;
	iss >> target;
	if (target.empty())
	{
		cli.queueResponse(":server 431 " + cli.getNickName() + " :No nickname given\r\n");
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
		cli.queueResponse(":server 401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
		return;
	}
		// RPL_WHOISUSER 311
		std::string tn = targetCli->getNickName();
		std::string tu = targetCli->getUserName();
		std::string tr = targetCli->getRealName();
		std::string host = "example.com";
		cli.queueResponse(":server 311 " + cli.getNickName() + " " + tn + " " + tu + " " + host + " * :" + tr + "\r\n");

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
		cli.queueResponse(":server 319 " + cli.getNickName() + " " + tn + " :" + chlist + "\r\n");

		// RPL_WHOISOP 313 if operator
		if (targetCli->getServerOp())
			cli.queueResponse(":server 313 " + cli.getNickName() + " " + tn + " :is an IRC operator\r\n");

		// RPL_ENDOFWHOIS 318
		cli.queueResponse(":server 318 " + cli.getNickName() + " " + tn + " :End of /WHOIS list\r\n");
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
	// if (cli.getInvited() == true)
	// 	wasInvited(cmd, cli);
	if (cmd.find("PING") == 0)
		cli.queueResponse("PONG :server\r\n");
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
	if (cmd.find("PASS ") == 0 && !cli.getAuth())
	{
		if (cmd.substr(5) == _password)
			cli.setAuth(true);
		else
			cli.queueResponse(":server 464 :Password incorrect\r\n");
	}
	else if (cli.getUserName().empty())
		if (cmd.find("USER ") == 0)
			User(cmd, cli);
	if ((!_password.empty() ? cli.getAuth() : true) && !cli.getUserName().empty() && !cli.getNickName().empty())
	{
		std::cout << "[welcome] fd=" << cli.getFd() << " nick='" << cli.getNickName() << "' user='" << cli.getUserName() << "'\n";
		cli.queueResponse(":server 001 " + cli.getNickName() + " :Welcome to the IRC server\r\n");
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
							cli.queueResponse(":server CAP * LS :\r\n"); //send empty capability list since we dont have any
						if (cmd.find("NICK ") == 0)
						{
							Nick(cmd, cli);
							continue;
						}
						
						if (cmd.find("CAP END") == 0 && !cli.getNickName().empty() && !cli.getUserName().empty() && cli.getAuth())
						{
							std::string host = "127.0.0.1"; // Dynamic on demand, used to show use in 001 response
							cli.queueResponse(":" + host + " 001 " + cli.getNickName() + " :Welcome to the IRC server " + cli.getNickName() + "!" + cli.getUserName() + "@" + host + "\r\n");
							cli.queueResponse(":server 002 " + cli.getNickName() + " :Your host is servername, running version 1.0\r\n");
							cli.queueResponse(":server 003 " + cli.getNickName() + " :This server was created today\r\n");
							cli.queueResponse(":server 004 " + cli.getUserName() + " servername 1.0 * *\r\n");
						}
						if (!cli.getAuth() || cli.getUserName().empty())
							WelcomeCommands(cmd, cli);
						else if (cli.getAuth() && !cli.getUserName().empty())
							Commands(cmd, cli);
						else if (cli.getAuth() == false && cli.getUserName().empty())
							cli.queueResponse(":server 464: Authenticate first\r\n");
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