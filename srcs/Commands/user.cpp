#include "newServer.hpp"

// Handles USER command: sets username and real name
void	Server::User(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	std::string param = cmd.substr(5);
	std::string user, unused1, unused2, realname;
	size_t colon = param.find(':');
	if (colon != std::string::npos) {
		realname = param.substr(colon + 1);
		param = param.substr(0, colon);
	}
	std::istringstream iss(param);
	iss >> user >> unused1 >> unused2;
	std::string extra;
	iss >> extra;

	if (!cli.getUserName().empty()) {
		cli.queueResponse(":" + host + " 462 :You may not reregister\r\n");
		return;
	}
	if (user.empty() || realname.empty()) {
		cli.queueResponse(":" + host + " 461 USER :Not enough parameters\r\n");
		return;
	}
	if (!extra.empty()) {
		cli.queueResponse(":" + host + " 461 USER :Too many parameters\r\n");
		return;
	}
	if (isNameInUse(user, false, cli.getFd())) {
		std::string newUser = user;
		do {
			newUser += "_";
		} while (isNameInUse(newUser, false, cli.getFd()));
		std::cout << "[user dup -> adjusted] fd=" << cli.getFd() << " tried '" << user << "' -> assigned '" << newUser << "'\n";
		cli.setUserName(newUser);
		cli.setRealName(realname);
		if (!cli.getNickName().empty())
			cli.queueResponse(":" + host + " NOTICE " + cli.getNickName() + " :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		else
			cli.queueResponse(":" + host + " NOTICE * :Your username was changed to " + newUser + " because the requested name was already in use\r\n");
		return;
	}
	std::cout << "[user set] fd=" << cli.getFd() << " -> user='" << user << "' realname='" << realname << "'\n";
	cli.setUserName(user);
	cli.setRealName(realname);
	// After setting user, check registration and send welcome numerics
	Server *srv = this;
	srv->sendWelcome(cli);
}
