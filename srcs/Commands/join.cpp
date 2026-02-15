#include "newServer.hpp"

// Handles JOIN command: joins or creates channel
void	Server::Join(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	std::istringstream iss(cmd.substr(5));
	std::string channel;
	std::string password;
	iss >> channel >> password;

	if (channel.empty())
	{
		cli.queueResponse(":" + host + " 461 JOIN :Not enough parameters\r\n");
		return;
	}

	int channelIndex = findChannel(channel);

	if (channelIndex == -1)
	{
		// Create new channel
		int newIndex = _channels.empty() ? 0 : _channels.rbegin()->first + 1;
		_channels[newIndex] = Channel(channel, "");
		channelIndex = newIndex;
	}

	// Already on channel, will never happen from irssi as it handles this case on the client program
	if (_channels[channelIndex].hasMember(cli.getFd()))
	{
		cli.queueResponse(":" + host + " 443 " + cli.getNickName() + " " + cli.getNickName() + " " + channel + " :is already on channel\r\n");
		return ;
	}

	if (_channels[channelIndex].getInvite())
	{
		if (!cli.getInvited() || cli.getInvitedIndex() != channelIndex)
		{
			cli.queueResponse(":" + host + " 473 " + cli.getNickName() + " " + channel + " :Cannot join channel (+i)\r\n");
			return ;
		}
		cli.setWasInvited(false);
		cli.setInvitedIndex(-1);
	}
	
	int currentUsers = _channels[channelIndex].members().size();
	if (_channels[channelIndex].getLimit() != -1 && currentUsers >= _channels[channelIndex].getLimit())
	{
		cli.queueResponse(":" + host + " 471 " + cli.getNickName() + " " + channel + " :Cannot join channel (+l)\r\n");
		return ;
	}
	
	if (!_channels[channelIndex].getPass().empty())
	{
		if (password != _channels[channelIndex].getPass())
		{
			cli.queueResponse(":" + host + " 475 " + cli.getNickName() + " " + channel + " :Cannot join channel (+k)\r\n");
			return ;
		}
	}
	_channels[channelIndex].addMember(cli.getFd());
	cli.setChannelIndex(channelIndex);
	// If first member, set as operator
	if (_channels[channelIndex].members().size() == 1)
		_channels[channelIndex].setOperator(cli.getFd(), true);
	// Send JOIN to all members
	sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host + " JOIN " + channel + "\r\n", channelIndex);
	// Send topic if set
	if (!_channels[channelIndex].getTopic().empty())
		cli.queueResponse(":" + host + " 332 " + cli.getNickName() + " " + channel + " :" + _channels[channelIndex].getTopic() + "\r\n");
	// Send names list
	std::string names;
	const std::set<int> &members = _channels[channelIndex].members();
	for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		std::map<int, Client>::iterator cit = _clients.find(*it);
		if (cit != _clients.end())
		{
			if (_channels[channelIndex].isOperator(*it))
				names += "@";
			names += cit->second.getNickName() + " ";
		}
	}
	if (!names.empty()) names.erase(names.size() - 1); // remove trailing space
	cli.queueResponse(":" + host + " 353 " + cli.getNickName() + " = " + channel + " :" + names + "\r\n");
	cli.queueResponse(":" + host + " 366 " + cli.getNickName() + " " + channel + " :End of /NAMES list\r\n");
}
