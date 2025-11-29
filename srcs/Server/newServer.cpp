#include "newServer.hpp"

void Server::createSocket()
{
	_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock_fd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	fcntl(_sock_fd, F_SETFL, O_NONBLOCK);
}

void Server::bindAndListen()
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr= INADDR_ANY;

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("Bind failed");
	if (listen(_sock_fd, 5) < 0)
		throw std::runtime_error("Listen failed");
	std::cout << "Listening on port " << _port << std::endl;
}

Server::Server(int port, const std::string &password)
{
	_sock_fd = -1;
	_port = port;
	_password = password;

	signal(SIGPIPE, SIG_IGN);
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



void	Server::Mode(std::string cmd, Client &cli)
{
//needed
}

void	Server::Topic(std::string cmd, Client &cli)
{
//needed
}

void	Server::Kick(std::string cmd, Client &cli)
{
//needed
}

void	Server::wasInvited(std::string cmd, Client &cli)
{
//TODO later
}

void	Server::Whisper(std::string cmd, Client &cli)
{
//needed
}

void	Server::Invite(std::string cmd, Client &cli)
{
//TODO later
}

void	Server::Join(std::string cmd, Client &cli)
{
//needed
}

void	Server::ListChannel(Client &cli)
{
//not needed
}

void	Server::ListUser(Client &cli)
{
//not needed
}

void	Server::ListCommands()
{
//not needed
}

void	Server::Nick(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::istringstream iss(param);
	std::string nick;
	iss >> nick;
	for (int i = 0; i < (int)_clients.size(); i++)
	{
		if (nick == _clients[i].getNickName() || nick == _clients[i].getUserName())
		{
			cli.queueRespone(":Server 443* " +nick+ " : Nickname is already in use\r\n");
			return;
		}
	}
	cli.setUserName(nick);
}

void	Server::User(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::istringstream iss(param);
	std::string user;
	iss >> user;
	for (int i = 0; i < (int)_clients.size(); i++)
	{
		if (user == _clients[i].getNickName() || user == _clients[i].getUserName())
		{
			cli.queueRespone(":Server 443* " +user+ " : Nickname is already in use\r\n");
			return;
		}
	}
	cli.setUserName(user);
}

void	Server::Message(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(8);
	std::istringstream iss(param);
	std::string target;
	ssize_t msg_pos = param.find(':');
	iss >> target;
	if (target[0] == '#')
		sendToChannel(cli.getUserName() + ": " + param.substr(msg_pos), cli.getChannelIndex());
	else
		for (int i = 0; i < (int)_clients.size();i++)
			if (_clients[i].getUserName() == target)
				_clients[i].queueRespone(cli.getUserName() + ": " + param.substr(msg_pos));
}

void	Server::sendToChannel(std::string msg, int channelIndex)
{
	for (int i = 0; i < (int)this->_clients.size();i++)
		if (this->_clients[i].getChannelIndex() == channelIndex)
			_clients[i].queueRespone(msg);
}

//CAP LS needs :your.server.name CAP * LS : as response from server


//after nick and user have been correctly send, and cap ls,
//send welcoming sequence,

//example

//:Server 001 user :Welcome to ft_irc user!hostname

// /kick sends											KICK (optional)channel <usernames> (optional)reason:
												//example:  KICK #General bob,sarah stop flodding!

void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	//create the channels;
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
					if (bytes <= 0 && !cli._isConnected())
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
						if (cmd.empty())
							continue;
						if (!cli.getAuth() || cli.getUserName().empty())
							WelcomeCommands(cmd, cli);
						else if (cli.getAuth() && !cli.getUserName().empty())
							Commands(cmd, cli);
						else if (!cli.getAuth() && cli.getUserName().empty())
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