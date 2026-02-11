#include "newServer.hpp"

void	Server::User(std::string cmd, Client &cli)
{
	std::string param = cmd.substr(5);
	std::istringstream iss(param);
	std::string user;
	iss >> user;

	if (!cli.getUserName().empty())
	{
		cli.queueResponse(":server 462 :You may not reregister\r\n");
		return;
	}

	if (user.empty())
	{
		cli.queueResponse(":server 461 USER :Not enough parameters\r\n");
		return;
	}
	if (isNameInUse(user, false, cli.getFd()))
	{
		std::string newUser = user;
		do
		{
			newUser += "_";
		} while (isNameInUse(newUser, false, cli.getFd()));
		std::cout << "[user dup -> adjusted] fd=" << cli.getFd() << " tried '" << user << "' -> assigned '" << newUser << "'\n";
		cli.setUserName(newUser);
		if (!cli.getNickName().empty())
			cli.queueResponse(":server NOTICE " + cli.getNickName() + " :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		else
			cli.queueResponse(":server NOTICE * :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		return;
	}
	std::cout << "[user set] fd=" << cli.getFd() << " -> user='" << user << "'\n";
	cli.setUserName(user);
}
