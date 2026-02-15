#ifndef NEWSERVER_HPP
# define NEWSERVER_HPP

#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>
#include <stdexcept>
#include <signal.h>
#include <poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <map>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include "newClient.hpp"
#include "Channel.hpp"

enum Command{
	CMD_CAP_LS,
	CMD_KICK,
	CMD_INVITE,
	CMD_TOPIC,
	CMD_MODE,
	CMD_WHISPER,
	CMD_JOIN,
	CMD_NICK,
	CMD_USER,
	CMD_MSG,
	CMD_WHOIS,
	CMD_UNKNOWN,
	CMD_OPER
};

class Server
{
	private:
		int	_sock_fd;
		int	_port;
		std::string _password;
		std::string _hostname;
		std::map<int, Client> _clients;
		std::map<int, Channel> _channels;

	public:
		Server(int port, const std::string &password);
		~Server();
		
		//getters
		std::string getPasswd(){return _password;}
		std::map<int, Client> &getClients(){return _clients;}
		std::map<int, Channel> &getChannels(){return _channels;}


		//main commands
		void	WelcomeCommands(std::string cmd, Client &cli);
		void	Commands(std::string cmd, Client &cli);
		void	createSocket();
		void	bindAndListen();
		void	run();
		//core commands
		void	Mode(std::string cmd, Client &cli);
		void	Topic(std::string cmd, Client &cli);
		void	Kick(std::string cmd, Client &cli);
		void	Invite(std::string cmd, Client &cli);
		void	Join(std::string cmd, Client &cli);
		void	Nick(std::string cmd, Client &cli);
		void	User(std::string cmd, Client &cli);
		void	Message(std::string cmd, Client &cli);
		void	Whois(std::string cmd, Client &cli);
		void	oper(std::string cmd, Client &cli);
		// helpers for checking name collisions
		bool	isNameInUse(const std::string &name, bool checkNick, int requesterFd);
		//helpers
		void	sendToChannel(std::string msg, int channelIndex);
		int		findChannel(std::string channel);
		Client	&findClient(std::string client);
		void	createChannel();
		void	sendWelcome(Client &cli);

};

inline Command stringToCommand(const std::string &s){
	std::istringstream iss(s);
	std::string token;
	if (!(iss >> token))
		return CMD_UNKNOWN;

	std::string cmd = token + ' ';
		static std::map<std::string, Command> table;
		if (table.empty())
		{
			table["KICK "]			= CMD_KICK;
			table["INVITE "]		= CMD_INVITE;
			table["TOPIC "]			= CMD_TOPIC;
			table["MODE "]			= CMD_MODE;
			table["JOIN "]			= CMD_JOIN;
			table["NICK "]			= CMD_NICK;
			table["USER "]			= CMD_USER;
			table["PRIVMSG "]		= CMD_MSG;
			table["WHOIS "]			= CMD_WHOIS;
			table["OPER "]			= CMD_OPER;
		}
		std::map<std::string, Command>::const_iterator it = table.find(cmd);
		if (it != table.end())
			return (it->second);
		return (CMD_UNKNOWN);
}


#endif