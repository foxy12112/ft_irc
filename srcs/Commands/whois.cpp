#include "newServer.hpp"

// Handles WHOIS command: queries user information
void	Server::Whois(std::string cmd, Client &cli)
{
	std::string host = _hostname;
	// Strictly parse WHOIS: only one nickname allowed
	size_t spacePos = cmd.find(' ');
	std::string param = (spacePos == std::string::npos || spacePos + 1 >= cmd.size()) ? std::string() : cmd.substr(spacePos + 1);
	std::istringstream iss(param);
	std::string target, extra;
	iss >> target >> extra;
	if (target.empty()) {
		cli.queueResponse(":" + host + " 431 " + cli.getNickName() + " :No nickname given\r\n");
		return;
	}
	if (!extra.empty()) {
		cli.queueResponse(":" + host + " 461 WHOIS :Too many parameters\r\n");
		return;
	}
	Client *targetCli = NULL;
	try {
		Client &c = findClient(target);
		targetCli = &c;
	} catch (std::exception &e) {
		cli.queueResponse(":" + host + " 401 " + cli.getNickName() + " " + target + " :No such nick\r\n");
		return;
	}
	// RPL_WHOISUSER 311 (fix real name field)
	std::string tn = targetCli->getNickName();
	std::string tu = targetCli->getUserName();
	std::string tr = targetCli->getRealName();
	std::string hostname = host;
	cli.queueResponse(":" + host + " 311 " + cli.getNickName() + " " + tn + " " + tu + " " + hostname + " * :" + tr + "\r\n");

	// RPL_WHOISCHANNELS 319 - list channels the user is in
	std::string chlist;
	int cidx = targetCli->getChannelIndex();
	for (std::map<int, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		if (it->first == cidx) {
			if (!chlist.empty()) chlist += " ";
			chlist += it->second.getName();
		}
	}
	cli.queueResponse(":" + host + " 319 " + cli.getNickName() + " " + tn + " :" + chlist + "\r\n");

	// RPL_WHOISOP 313 if operator
	if (targetCli->getServerOp())
		cli.queueResponse(":" + host + " 313 " + cli.getNickName() + " " + tn + " :is an IRC operator\r\n");

	// RPL_ENDOFWHOIS 318
	cli.queueResponse(":" + host + " 318 " + cli.getNickName() + " " + tn + " :End of /WHOIS list\r\n");
}
