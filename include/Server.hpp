#ifndef SERVER_HPP
# define SERVER_HPP

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
#include <algorithm>
#include <arpa/inet.h>
#include "Client.hpp"
#include "Channel.hpp"

enum Command{
	CMD_KICK,
	CMD_INVITE,
	CMD_TOPIC,
	CMD_MODE,
	CMD_WHISPER,
	CMD_JOIN,
	CMD_LIST_CHANNEL,
	CMD_LIST_USER,
	CMD_LIST_COMMANDS,
	CMD_NICK,
	CMD_USER,
	CMD_UNKNOWN
};

class Server
{
	private:
		int _sock_fd;
		int _port;
		std::string _password;
		std::map<int, Client> _clients; // Map of client FDs to Client data
		std::map<int, Channel> _channels;


		void createSocket();
		void bindAndListen();
		void createChannel(void);

	public:
		Server(int port, const std::string &password);
		~Server();
		void run();
		std::map<int, Client>& getClients();
		const std::string& getPassword() const;
		void broadcast(const std::string& msg, int exclude_fd = -1);
		std::string welcomeCommands(std::string cmd, Client &cli);
		std::string Commands(std::string cmd, Client &cli, int i);
		void	Mode(std::string cmd, std::string &resp, Client &cli);
		void	Topic(std::string &resp, std::string cmd, Client &cli);
		void	Kick(std::string &resp, std::string cmd, Client &cli);
		void	wasInvited(std::string cmd, std::string &resp, Client &cli);
		void	Whisper(std::string cmd, Client &cli);
		void	Invite(std::string cmd, Client &cli);
		void	Join(std::string &resp, std::string cmd, Client &cli);
		void	ListChannel(std::string &resp, Client &cli);
		void	ListUser(std::string &resp, Client &cli);
		void	ListCommands(std::string &resp);
		void	Nick(std::string cmd, std::string &resp, Client &cli);
		void	User(std::string cmd, std::string &resp, Client &cli);
};

inline Command stringToCommand(const std::string &s){

	std::string cmd;
	int i = 0;
	for (i = 0; i < (int)s.size() && s[i] != ' ';i++)
		cmd += s[i];
	cmd += s[i];
	static std::map<std::string, Command> table;
	if (table.empty())
	{
		table["KICK "]			= CMD_KICK;
		table["INVITE "]		= CMD_INVITE;
		table["TOPIC "]			= CMD_TOPIC;
		table["MODE "]			= CMD_MODE;
		table["WHISPER "]		= CMD_WHISPER;
		table["JOIN "]			= CMD_JOIN;
		table["LIST_CHANNEL "]	= CMD_LIST_CHANNEL;
		table["LIST_USER "]		= CMD_LIST_USER;
		table["LIST_COMMANDS "]	= CMD_LIST_COMMANDS;
		table["NICK "]			= CMD_NICK;
		table["USER "]			= CMD_USER;
	}
	std::map<std::string, Command>::const_iterator it = table.find(cmd);
	if (it != table.end())
		return (it->second);
	return (CMD_UNKNOWN);
}

#endif