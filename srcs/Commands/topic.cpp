#include "newServer.hpp"

void	Server::Topic(std::string cmd, Client &cli)
{
	std::string param;
	int channelIndex = cli.getChannelIndex();
	
	if (cmd.size() == 5)
	{
		cli.queueResponse(_channels[channelIndex].getTopic());
		return;
	}

	bool isChannelOp = _channels[channelIndex].isOperator(cli.getFd());
	bool canModifyTopic = isChannelOp || !_channels[channelIndex].getTopicOp();
	
	if (!canModifyTopic)
	{
		cli.queueResponse(":server 482 " + _channels[channelIndex].getName() + " :You must be a channel operator\r\n");
		return;
	}

	std::string topicText = cmd.substr(6);
	
	if (topicText == "-delete")
	{
		_channels[channelIndex].setTopic("");
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 TOPIC " + _channels[channelIndex].getName() + " :\r\n", channelIndex);
	}
	else
	{
		_channels[channelIndex].setTopic(topicText);
		sendToChannel(":" + cli.getNickName() + "!~" + cli.getUserName() + "@127.0.0.1 TOPIC " + _channels[channelIndex].getName() + " :" + topicText + "\r\n", channelIndex);
	}
}
