#ifndef NEWCLIENT_HPP
# define NEWCLIENT_HPP

#include <string>
#include <iostream>
#include <cerrno>

class	Server;
class	Channel;

class Client
{
	private:
		int			_clientFd;
		int			_index;
		std::string	_inBuffer;
		std::string	_outBuffer;
		std::string	_nickName;
		std::string	_userName;
		std::string	_realName;
		bool		_isAuthenticated;
		bool		_isConnected;
		bool		_isOp;
		int			_channelIndex;
		int			_invitedIndex;
		std::string	_invitedClient;
		bool		_wasInvited;
		//figure out how to deal with invited

	public:
		Client(): _clientFd(-1), _isAuthenticated(false), _isConnected(false), _isOp(false){;}
		Client(int fd): _clientFd(fd), _isAuthenticated(false), _isConnected(true), _isOp(false){;}
		Client(const Client &other);
		Client &operator=(const Client &other);
		~Client(){;}

		//getters
		int				getFd(){return _clientFd;}
		int				getClientIndex(){return _index;}
		int				getChannelIndex(){return _channelIndex;}
		int				getInvitedIndex(){return _invitedIndex;}
		bool			getAuth(){return _isAuthenticated;}
		bool			getCon(){return _isConnected;}
		bool			getOp(){return _isOp;}
		std::string		&getInBuffer(){return _inBuffer;}
		std::string		&getOutBuffer(){return _outBuffer;}
		std::string		&getNickName(){return _nickName;}
		std::string		&getUserName(){return _userName;}
		std::string		&getRealName(){return _realName;}
		std::string		&getInvitedClient(){return _invitedClient;}
		bool			getInvited(){return _wasInvited;}

		//setters

		void			setFd(int fd){_clientFd = fd;}
		void			setClientIndex(int index){_index = index;}
		void			setChannelIndex(int index){_channelIndex = index;}
		void			setInvitedIndex(int index){_invitedIndex = index;}
		void			setAuth(bool status){_isAuthenticated = status;}
		void			setCon(bool status){_isConnected = status;}
		void			setOp(bool status){_isOp = status;}
		void			setInBuffer(std::string buffer){_inBuffer = buffer;}
		void			setOutBuffer(std::string buffer){_outBuffer = buffer;}
		void			setNickName(std::string nickname){_nickName = nickname;}
		void			setUserName(std::string username){_userName = username;}
		void			setRealName(std::string realname){_realName = realname;}
		void			setInvitedClient(std::string client){_invitedClient = client;}
		void			setWasInvited(bool status){_wasInvited = status;}
		//true functions

		void	disconnect();
		int		receive(Client Sender);
		bool	extractNextCommand(std::string &cmd);
		void	queueRespone(const std::string &resp);
		bool	flushSend();
		bool	hasDataToSend();
};

#endif