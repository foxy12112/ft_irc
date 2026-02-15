#include "newServer.hpp"

void	Server::Topic(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	std::string param;
	int channelIndex = cli.getChannelIndex();

	if (_channels.find(channelIndex) == _channels.end() || !_channels[channelIndex].hasMember(cli.getFd()))
	{
		cli.queueResponse(":" + host + " 442 " + (_channels.find(channelIndex) != _channels.end() ? _channels[channelIndex].getName() : std::string("*")) + " :You're not on that channel\r\n");
		return;
	}
	
	if (cmd.size() == 5)
	{
		cli.queueResponse(_channels[channelIndex].getTopic());
		return;
	}

	bool isChannelOp = _channels[channelIndex].isOperator(cli.getFd());
	bool canModifyTopic = isChannelOp || !_channels[channelIndex].getTopicOp();
	
	if (!canModifyTopic)
	{
		cli.queueResponse(":" + host + " 482 " + _channels[channelIndex].getName() + " :You're not channel operator\r\n");
		return;
	}

	std::string topicText = cmd.substr(6);
	
	if (topicText == "-delete")
	{
		_channels[channelIndex].setTopic("");
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host + " TOPIC " + _channels[channelIndex].getName() + " :\r\n", channelIndex);
	}
	else
	{
		_channels[channelIndex].setTopic(topicText);
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@" + host + " TOPIC " + _channels[channelIndex].getName() + " :" + topicText + "\r\n", channelIndex);
	}
}
