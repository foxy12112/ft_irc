#include "newServer.hpp"

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
		cli.queueResponse(":server 403 " + cli.getNickName() + " " + channelName + " :No such channel\r\n");
		return;
	}
	Channel &channel = chIt->second;

	if (modeString.empty())
	{
		cli.queueResponse(":server 461 " + cli.getNickName() + " MODE :Not enough parameters\r\n");
		return;
	}

	if (!channel.hasMember(cli.getFd()))
	{
		cli.queueResponse(":server 442 " + cli.getNickName() + " " + channel.getName() + " :You're not on that channel\r\n");
		return;
	}
	if (!channel.isOperator(cli.getFd()))
	{
		cli.queueResponse(":server 482 " + channel.getName() + " :You're not channel operator\r\n");
		std::cout << "cant do\n";
		return;
	}
	std::string modes;
	std::string params;
	bool adding = false;

	char unknown = 0;
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
		else
		{
			unknown = c;
			break;
		}
	}

	if (unknown)
	{
		std::string m(1, unknown);
		cli.queueResponse(":server 472 " + cli.getNickName() + " " + m + " :is unknown mode char to me\r\n");
		return;
	}

	if (!modes.empty())
	{
		std::string modeMsg = ":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 MODE " + channel.getName() + " " + modes + params + "\r\n";
		sendToChannel(modeMsg, channelIndex);
	}
}
