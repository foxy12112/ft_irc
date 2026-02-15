#include "newServer.hpp"

// Handles KICK command: removes user from channel
void Server::Kick(std::string cmd, Client &cli)
{
	std::string host = _hostname;
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

	if (channel.empty() || targets.empty())
	{
		cli.queueResponse(":" + host + " 461 KICK :Not enough parameters\r\n");
		return;
	}
	// Validate channel format
	if (channel[0] != '#')
	{
		cli.queueResponse(":" + host + " 403 " + cli.getNickName() + " " + channel + " :No such channel\r\n");
		return;
	}

	// Validate channel exists and issuing user is on it
	int channelIndex = findChannel(channel);
	if (channelIndex == -1)
	{
		cli.queueResponse(":" + host + " 403 " + cli.getNickName() + " " + channel + " :No such channel\r\n");
		return;
	}
	if (!_channels[channelIndex].hasMember(cli.getFd()))
	{
		cli.queueResponse(":" + host + " 442 " + cli.getNickName() + " " + channel + " :You're not on that channel\r\n");
		return;
	}
	if (!_channels[channelIndex].isOperator(cli.getFd()))
	{
		cli.queueResponse(":" + host + " 482 " + channel + " :You're not channel operator\r\n");
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
				cli.queueResponse(":" + host + " 441 " + cli.getNickName() + " " + target + " " + channel + " :They aren't on that channel\r\n");
				continue;
			}
			
			// Build KICK message with full prefix
			std::string kickPrefix = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host;
			std::string kickMsg = kickPrefix + " KICK " + channel + " " + target + " :" + reason + "\r\n";

			// First, remove from channel membership
			_channels[channelIndex].removeMember(kickedClient.getFd());
			// Then, broadcast KICK to channel members (target excluded now)
			sendToChannel(kickMsg, channelIndex);
		// Deliver explicit KICK and NOTICE to the kicked user
		kickedClient.queueResponse(kickMsg);
		kickedClient.queueResponse(":" + host + " NOTICE " + kickedClient.getNickName() + " :You were kicked from " + channel + " (" + reason + ")\r\n");
		}
		catch (...)
		{
			cli.queueResponse(":" + host + " 401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
		}
	}
}
