#include "newServer.hpp"

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
