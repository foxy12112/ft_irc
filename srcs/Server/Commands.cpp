#include "Server.hpp"


void	Server::Nick(std::string cmd, std::string &resp, Client &cli)
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

void	Server::User(std::string cmd, std::string &resp, Client &cli)
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

void	Server::ListCommands(std::string &resp)
{
	resp = ":server 323 :NICK | USER | LIST_CMD | LIST_USER\r\n";
}

void	Server::ListUser(std::string &resp, Client &cli)
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

void	Server::ListChannel(std::string &resp, Client &cli)
{
	resp = "NAME			|	TOPIC\n";
	for (int i = 0; i < (int)this->_channels.size(); i++)
	{
		resp.append(_channels[i].getName()+"			|			"+_channels[i].getTopic()+"\n");
	}
	resp.append("\r\n");
	cli.setMsgType(0);
}

void	Server::Join(std::string &resp, std::string cmd, Client &cli)
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

void	Server::Invite(std::string cmd, Client &cli)
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

void	Server::Whisper(std::string cmd, Client &cli)
{
	std::size_t pos = cmd.find(' ');
	std::size_t pos2 = cmd.find(' ', pos);
	std::cout << pos << "  " << pos2 << "  " << cmd.substr(8, pos -8) << std::endl;
	if (pos != std::string::npos)
	{
		for (int i = 0; i < (int)_clients.size(); i++)
			if (_clients[i].username() == cmd.substr(8, pos - 8))
				_clients[i].queueResponse(cli.username() + ": " + cmd.substr(pos2) + "\r\n");
	}

	/*TODO
	1. get the first and sercond space
	2.m get the lenght of the username + 1
	3. send msg to client
	*/
}

void	Server::wasInvited(std::string cmd, std::string &resp, Client &cli)
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

void	Server::Kick(std::string &resp, std::string cmd, Client &cli)
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

void	Server::Topic(std::string &resp, std::string cmd, Client &cli)
{
	for (int i = 0; i < (int)this->_channels.size();i++)
	{
		if (this->_channels[i].getName() == cli.getChannel())
		{
			if (this->_channels[i].getTopicOp() == false)
			{
				this->_channels[i].setTopic(cmd.substr(6));
				resp = "channel topic changed succesfully\r\n";
			}
			else
				if (cli.isOp() == true)
				{
					this->_channels[i].setTopic(cmd.substr(6));
					resp = "channel topic changed succesfully\r\n";
				}
		}
	}
	if (resp.empty())
		resp = "channel topic could not be changed, you arel ikely not op\r\n";
}

void	Server::Mode(std::string cmd, std::string &resp, Client &cli)
{
	int ind;
	std::string mode = cmd.substr(5, 2);
	for (int i = 0; i < (int)this->_channels.size();i++)
		if (cli.getChannel() == this->_channels[i].getName())
			ind = i;
	Channel &channel = this->_channels[ind];
	if (mode == "-i")
	{
		channel.setInvite(!channel.getInvite());
		resp = "Channel: " + channel.getName() + " has been set to " + (channel.getInvite() ? "invite only\r\n" : "no invite needed\r\n");
	}
	else if (mode == "-t")
	{
		channel.setTopicOp(!channel.getTopicOp());
		resp = "Channel: " + channel.getName() + " Topic can be edited by " + (channel.getTopicOp() ? "operators only\r\n" : "anyone\r\n");
	}
	else if (mode == "-k")
	{
		if (!channel.getPass().empty())
		{
			channel.setPass(NULL);
			resp = "Channel: password has been removed\r\n";
		}
		else if (channel.getPass().empty())
		{
			std::string pass = cmd.substr(8);
			if (!pass.empty())
			{
				channel.setPass(pass);
				resp = "Channel: password has been set\r\n";
			}
			else
				resp = "Channel: password could not be set: error empty password\r\n";
		}
	}
	else if (mode == "-o")
	{
		std::string user = cmd.substr(8);
		for (int i = 0; i < (int)this->_clients.size() ; i++)
			if (user == this->_clients[i].username())
			{
				this->_clients[i].setOp(!this->_clients[i].isOp());
				resp = "Client: " + _clients[i].username() + (this->_clients[i].isOp() ? "is now an operator\r\n" : "is not an operator anymore\r\n");
			}
	}
	else if (mode == "-l")
	{
		std::string limit = cmd.substr(8);
		if (limit.empty())
		{
			channel.setLimit(-1);
			resp = "Channel: Limit has been removed\r\n"; //TODO change join/invite to care about channel limit
		}
		else
		{
			channel.setLimit(std::atoi(limit.c_str()));
			resp = "Channel: Limit has been set to " + limit + "\r\n";
		}
	}
}