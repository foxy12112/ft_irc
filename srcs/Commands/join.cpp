#include "newServer.hpp"

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
