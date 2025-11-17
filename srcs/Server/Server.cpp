/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julcalde <julcalde@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 01:29:20 by bszikora          #+#    #+#             */
/*   Updated: 2025/11/11 15:57:37 by julcalde         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(int port, const std::string &password) : _sock_fd(-1), _port(port), _password(password)
{
	signal(SIGPIPE, SIG_IGN); // Prevents crash on write to closed socket
	createSocket();
	bindAndListen();
}

Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
	for (std::map<int, Client>::iterator iter = _clients.begin(); iter != _clients.end(); ++iter)
		iter->second.disconnect();
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
	addr.sin_port = htons(_port); // Convert port to network byte order
	addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) // Bind the socket
		throw std::runtime_error("Bind failed");;
	if (listen(_sock_fd, 5) < 0) // Listen for incoming connections
		throw std::runtime_error("Listen failed");
	std::cout << "Listening on port " << _port << std::endl;
}

void	Server::createChannel(void)
{
	this->_channels[0] = Channel("#General", "idk basic channel ig");
	this->_channels[1] = Channel("#Coffebreak", "Casual chat for developers, designers, and night owls");
	this->_channels[2] = Channel("#retrocomputing", "Everything about old PCs, terminals, and vintage OSes");
	this->_channels[3] = Channel("#Operator channel", "Channel only for operators");
	this->_channels[4] = Channel("#zenmode", "A calm space for meditation, mindfulness, and philosophy talk");
	this->_channels[5] = Channel("purgatory", "this is just a purgatory for people who have been kicked from their channel");
}

std::map<int, Client>& Server::getClients() // Accessor for clients map
{
	return (_clients);
}

const std::string& Server::getPassword() const // Accessor for server password
{
	return (_password);
}

void Server::broadcast(const std::string& msg, int exclude_fd) // Broadcasts a msg to all authenticated clients, except for exclude_fd
{
	for (std::map<int, Client>::iterator iter = _clients.begin(); iter != _clients.end(); ++iter)
	{
		if (iter->first == exclude_fd || !iter->second.isAuthenticated())
			continue ;
		iter->second.queueResponse(msg);
	}
}

std::string Server::welcomeCommands(std::string cmd, Client &cli)
{
	std::string resp;
	if (cmd.find("OPASS ") ==  0 && cli.isAuthenticated() == false)
	{
		cli.setMsgType(0);
		std::string pass = cmd.substr(6);
		
		if (pass == "im_op-"+_password)
		{
			cli.setOp(true);
			resp = ":server 001 nick :Welcome to the IRC server as operator\r\n";
			cli.setAuthenticated(true);
		}
		else
			resp = ":server 464 :operator Password incorrect\r\n";
	}
	else if (cmd.find("PASS ") == 0 && cli.isAuthenticated() == false)
	{
		cli.setMsgType(0);
		std::string pass = cmd.substr(5);
		if (pass == _password)
		{
			resp = ":server 001 nick :Welcome to the IRC server\r\n";
			cli.setAuthenticated(true);
		}
		else
			resp = ":server 464 :Password incorrect\r\n";
	}
	else if (cli.username().empty())
	{
		cli.setMsgType(0);
		if (cmd.find("USER ") == 0)
		{
			std::string user = cmd.substr(5);
			for (int i = 0; i < (int)this->_clients.size(); i++)
			{
				if (this->_clients[i].nickname() == user || this->_clients[i].username() == user)
					resp = ":server 001: nickname unabailable\r\n";
			}
			if (resp.empty())
			{
				cli.setUsername(user);
				resp = ":server 002 " + user + " :User set\r\n";
			}
			resp = "welcome to the IRC server " + user + "\r\n";
		}
		else
			resp = ":server 464 : Please set username\tusage \"USER (username)\"\r\n";
	}
	return resp;
}

std::string Server::Commands(std::string cmd, Client &cli, std::string &privateMessageClient, int i)
{
	std::string resp;
	cli.setMsgType(1);
	i++;
	i--;
	std::cout << cmd;
	if (cmd == "PING")
	{
		resp = "PONG\r\n";
		cli.setMsgType(0);
	}
	else if (cmd.find("NICK ") == 0)
	{
		std::string nick = cmd.substr(5);
		for (int i = 0; i < (int)this->_clients.size(); i++)
		{
			if (this->_clients[i].nickname() == nick || this->_clients[i].username() == nick)
				resp = ":server 001: nickname unabailable\r\n";
		}
		if (resp.empty())
		{
			cli.setNickname(nick);
			resp = ":server 001 " + nick + " :Nickname set\r\n";
		}
		cli.setMsgType(0);
	}
	else if (cmd.find("USER ") == 0)
	{
		std::string user = cmd.substr(5);
		for (int i = 0; i < (int)this->_clients.size(); i++)
		{
			if (this->_clients[i].nickname() == user || this->_clients[i].username() == user)
				resp = ":server 001: nickname unabailable\r\n";
		}
		if (resp.empty())
		{
			cli.setUsername(user);
			resp = ":server 002 " + user + " :User set\r\n";
		}
		cli.setMsgType(0);
	}
	else if (cmd.find("LIST_CMD ") == 0)
		resp = ":server 323 :NICK | USER | LIST_CMD | LIST_USER\r\n";
	else if (cmd.find("LIST_USER ") == 0)
	{
		resp = ":server 353 * :\n";
		for (int i = 0; i < (int)_clients.size(); i++)
		{
			if (_clients[i].isConnected())
				resp.append(_clients[i].username()+"	|	"+_clients[i].nickname()+"\n");
		}
		resp.append("\r\n");
		cli.setMsgType(0);
	}
	else if (cmd.find("LIST_CHANNELS ") == 0)
	{
		resp = "NAME			|	TOPIC\n";
		for (int i = 0; i < (int)this->_channels.size(); i++)
		{
			resp.append(_channels[i].getName()+"	|	"+_channels[i].getTopic()+"\n");
		}
		resp.append("\r\n");
		cli.setMsgType(0);
	}
	else if (cmd.find("JOIN ") == 0)
	{
		std::string channel = cmd.substr(5);
		std::string oldChannel = cli.getChannel();
		for (int i = 0; i < (int)_channels.size(); i++)
			if (_channels[i].getName() == channel && _channels[i].getInvite() == false)
				cli.setChannel(channel);
		if (oldChannel != cli.getChannel())
			resp = "Channel changed succesfully\r\n";
		else
			resp = "Channel change failed\r\n";
		cli.setMsgType(0);
	}
	else if (cmd.find("INVITE ") == 0)
	{
		std::string user = cmd.substr(7);
		for (int i = 0; i < (int)_clients.size(); i++)
			if (_clients[i].username() == user)
			{
				_clients[i].setInvite(true);
				_clients[i].setInviteedBy(cli.getChannel());
				_clients[i].setMsgType(2);
				_clients[i].queueResponse(cli.username() + " has invited you to join the channel: " + cli.getChannel() + "(Press y or n)" + "\r\n");
			}
	}
	else if (cmd.find("WHISPER ") == 0)
	{
		std::size_t pos = cmd.find(' ', 8);
		if (pos != std::string::npos)
		{
			privateMessageClient = cmd.substr(8, pos - 8);
			resp = cmd.substr(8 + privateMessageClient.size() + 1) + "\r\n";
			cli.setMsgType(2);
		}
	}
	else if (cli.getInvite() == true)
	{
		cli.setMsgType(1);
		if (cmd.find("y") == 0)
		{
			cli.setChannel(cli.getInvitedBy());
			resp = "joined the channel " + cli.getChannel() + "\r\n";
			cli.setInvite(false);
		}
		else
		{
			for (int i = 0; i < (int)this->_clients.size();i++)
				if (this->_clients[i].username() == cli.getInvitedBy())
				{
					_clients[i].queueResponse("invite refused\r\n");
					_clients[i].setMsgType(0);
				}
			cli.setInvite(false);
		}
	}
	else if (cmd.find("KICK ") == 0 && cli.isOp())
	{
		std::string user = cmd.substr(5);
		for (int i = 0; i < (int)this->_clients.size();i++)
		{
			if (user == _clients[i].username())
			{
				_clients[i].queueResponse("you have been kicked from the channel: " + _clients[i].getChannel() + " you have been sent to purgatory\r\n");
				_clients[i].setChannel("purgatory");
				_clients[i].setMsgType(0);
				resp = "user: " + user + " has been sent to purgatory\r\n";
			}
			else
				resp = "client doesnt exist dumbass\r\n";
		}
		cli.setMsgType(1);
	}
	else if (cmd.find("TOPIC ") == 0)
	{
		for (int i = 0; i < (int)this->_channels.size();i++)
		{
			if (this->_channels[i].getName() == cli.getChannel())
			{
				if (this->_channels[i].getTopicOp() == false)
					this->_channels[i].setTopic(cmd.substr(6));
				else
					if (cli.isOp() == true)
						this->_channels[i].setTopic(cmd.substr(6));
				resp = "channel topic changed succesfully\r\n";
			}
		}
		if (resp.empty())
			resp = "channel topic could not be changed, you arel ikely not op\r\n";
	}
	else if (cmd.find("MODE ") == 0)
	{
		//TODO
	}
	else
		resp = cmd + "\r\n";
	return resp;
}

void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	std::string privateMessageClient;
	createChannel();
	this->_channels[5].setInvite(true);
	while (true) // Main server loop
	{
		if (poll(&fds[0], fds.size(), -1) < 0) // If poll fails
			throw std::runtime_error("Poll failed");
		for (size_t i = 0; i < fds.size(); ++i) // While there are fds to process
		{
			if (fds[i].revents & POLLIN) // Check for incoming data
			{
				/* Accept new connection */
				if (fds[i].fd == _sock_fd)
				{
					struct sockaddr_in addr; // Client address structure
					socklen_t len = sizeof(addr);
					int client_fd = accept(_sock_fd, (struct sockaddr*)&addr, &len);
					std::cout << "Client connected, fd=" << client_fd << std::endl; // DEBUG to be removed
					if (client_fd < 0) // If accept fails
						continue ;
					fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set client socket to non-blocking
					struct pollfd pfd = {client_fd, POLLIN, 0}; // Prepare pollfd for new client
					fds.push_back(pfd); // Add new client to poll fds
					_clients[client_fd] = Client(client_fd); // Initialize new client data with fd
				}
				/* Recv from client */
				else
				{
					Client &cli = _clients[fds[i].fd];
					int bytes = cli.receive(_clients[fds[i].fd]);
					if (bytes <= 0 && !cli.isConnected())
					{
						std::cout << "Client fd=" << fds[i].fd << " disconnected" << std::endl; // DEBUG to be removed
						cli.disconnect();
						_clients.erase(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i;
						continue ;
					}
					/* Loop to process complete commands */
					std::string cmd;
					while (cli.extractNextCommand(cmd)) //doesnt enter, todo
					{
						std::cout << cmd;
						if (cmd.empty())
							continue ;
						std::string resp;
						if (cli.isAuthenticated() == false || cli.username().empty())
							resp = welcomeCommands(cmd, cli);
						else if (cli.isAuthenticated() == true && !cli.username().empty())
							resp = Commands(cmd, cli, privateMessageClient, i);
						else if (cli.isAuthenticated() == false && cli.username().empty())
							resp = ":server 464 : Authenticate first\r\n";
						if (!resp.empty())
						{
							std::cout << "hji" << std::endl;
							if (cli.getMsgType() == 1)
								for (int i = 0; i < (int)_clients.size(); i++)
								{
									if (_clients[i].getFd() != cli.getFd() && _clients[i].getChannel() == cli.getChannel() && cli.isAuthenticated() == true && !cli.username().empty())
									{
										if (!cli.nickname().empty())
											_clients[i].queueResponse(cli.nickname()+": "+resp);
										else
											_clients[i].queueResponse(cli.username()+": "+resp);
									}
								}
							else if (cli.getMsgType() == 2)
							{
								for (int i = 0; i < (int)_clients.size(); i++)
									if (_clients[i].username() == privateMessageClient)
									{
										if (!cli.nickname().empty())
												_clients[i].queueResponse(cli.nickname()+": "+resp);
										else
												_clients[i].queueResponse(cli.username()+": "+resp);
									}
							}
							else if (cli.getMsgType() == 0)
								cli.queueResponse(resp);
						}
					}
				}
			}
			if (fds[i].revents & POLLOUT && fds[i].fd != _sock_fd) // Check for outgoing data
			{
					Client &cli = _clients[fds[i].fd];
					cli.flushSend();
					if (!cli.isConnected())
					{
						std::cout << "Client fd=" << fds[i].fd << " disconnected during send" << std::endl; // DEBUG to be removed
						cli.disconnect(); // Clean up client
						_clients.erase(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i;
						continue ;
					}
			}
		}
		// making sure POLLOUT is set only for clients that have pending data
		for (size_t j = 1; j < fds.size(); ++j)
		{
			std::map<int, Client>::iterator it = _clients.find(fds[j].fd);
			if (it == _clients.end())
				continue ;
			fds[j].events = POLLIN | (it->second.hasDataToSend() ? POLLOUT : 0);
		}
	}
}