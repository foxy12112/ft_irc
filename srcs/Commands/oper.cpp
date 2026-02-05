#include "newServer.hpp"

void	Server::oper(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::string name;
	std::string password;
	std::istringstream iss(param);
	iss >> name;
	iss >> password;
	Client *clintPtr = NULL;
	try
	{
		Client &clint = findClient(name);
		clintPtr = &clint;
	}
	catch (std::exception &)
	{
		cli.queueResponse(":server 401 " + cli.getNickName() + " " + name + " :No such nick\r\n");
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
		clintPtr->queueResponse(":server 464 "+clintPtr->getNickName()+" :Password incorrect\r\n");
	if (clintPtr->getServerOp() == true)
		clintPtr->queueResponse(":server 381 " + clintPtr->getNickName() + " :You are now an IRC operator\r\n");
}
