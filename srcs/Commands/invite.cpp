#include "newServer.hpp"

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

	if (!_channels[chIdx].hasMember(cli.getFd()))
	{
		cli.queueResponse(":server 442 " + channelName + " :You're not on that channel\r\n");
		return;
	}

	if (!_channels[chIdx].isOperator(cli.getFd()))
	{
		cli.queueResponse(":server 482 " + channelName + " :You're not channel operator\r\n");
		return;
	}

	if (_channels[chIdx].hasMember(invitee->getFd()))
	{
		cli.queueResponse(":server 443 " + cli.getNickName() + " " + invitee->getNickName() + " " + channelName + " :is already on channel\r\n");
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
