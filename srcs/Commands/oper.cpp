#include "newServer.hpp"

void	Server::oper(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	std::string param = cmd.substr(5);
	std::string name;
	std::string password;
	std::istringstream iss(param);
	iss >> name;
	iss >> password;
	if (name.empty() || password.empty())
	{
		cli.queueResponse(":" + host + " 461 OPER :Not enough parameters\r\n");
		return;
	}
	Client *clintPtr = NULL;
	try
	{
		Client &clint = findClient(name);
		clintPtr = &clint;
	}
	catch (std::exception &)
	{
		cli.queueResponse(":" + host + " 401 " + cli.getNickName() + " " + name + " :No such nick\r\n");
		return;
	}
	// If the target client is already a server operator, just inform and exit
	if (clintPtr->getServerOp() == true)
	{
		clintPtr->queueResponse(":" + host + " 381 " + clintPtr->getNickName() + " :You already have IRC operator permissions\r\n");
		return;
	}
	if (password == "operpass")
	{
		bool newStatus = !clintPtr->getServerOp();
		clintPtr->setServerOp(newStatus);
		for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
			it->second.setOperator(clintPtr->getFd(), newStatus);
	}
	else
		clintPtr->queueResponse(":" + host + " 464 "+clintPtr->getNickName()+" :Password incorrect\r\n");
	if (clintPtr->getServerOp() == true)
		clintPtr->queueResponse(":" + host + " 381 " + clintPtr->getNickName() + " :You are now an IRC operator\r\n");
}
