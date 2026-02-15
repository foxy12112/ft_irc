#include "newServer.hpp"

// Handles PRIVMSG command: sends messages to channels/users
void	Server::Message(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	std::string param = cmd.substr(8);
	std::istringstream iss(param);
	std::string target;
	iss >> target;
	ssize_t msg_pos = param.find(':');
	if (target.empty())
	{
		cli.queueResponse(":" + host + " 411 PRIVMSG :No recipient given\r\n");
		return;
	}
	if (msg_pos == (ssize_t)std::string::npos)
	{
		cli.queueResponse(":" + host + " 412 :No text to send\r\n");
		return;
	}
	std::string message = param.substr(msg_pos + 1);
	if (message.empty())
	{
		cli.queueResponse(":" + host + " 412 :No text to send\r\n");
		return;
	}
	std::string prefix = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host;
	if (!target.empty() && target[0] == '#')
	{
		int chidx = findChannel(target);
		if (chidx == -1)
		{
			cli.queueResponse(":" + host + " 403 " + cli.getNickName() + " " + target + " :No such channel\r\n");
			return;
		}
		// Prevent non-members from speaking in the channel using membership set
		if (!_channels[chidx].hasMember(cli.getFd()))
		{
			cli.queueResponse(":" + host + " 404 " + target + " :Cannot send to channel\r\n");
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
		// If no matching user found, respond with ERR_NOSUCHNICK
		bool found = false;
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.getNickName() == target || it->second.getUserName() == target)
			{
				found = true;
				break;
			}
		}
		if (!found)
			cli.queueResponse(":" + host + " 401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
	}
}
