#ifndef NEWCLIENT_HPP
# define NEWCLIENT_HPP

#include <string>
#include <iostream>

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
		bool		_isServerOp;
		int			_channelIndex;
		int			_invitedIndex;
		std::string	_invitedClient;
		bool		_wasInvited;
		//figure out how to deal with invited

	public:
		Client()
			: _clientFd(-1), _index(-1), _inBuffer(), _outBuffer(), _nickName(), _userName(), _realName(),
				_isAuthenticated(false), _isConnected(false), _isServerOp(false), _channelIndex(-1), _invitedIndex(-1), _invitedClient(), _wasInvited(false)
		{
		}

		Client(int fd)
			: _clientFd(fd), _index(-1), _inBuffer(), _outBuffer(), _nickName(), _userName(), _realName(),
				_isAuthenticated(false), _isConnected(true), _isServerOp(false), _channelIndex(-1), _invitedIndex(-1), _invitedClient(), _wasInvited(false)
		{
		}
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
		bool			getServerOp(){return _isServerOp;}
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
		void			setServerOp(bool status){_isServerOp = status;}
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
		void	queueResponse(const std::string &resp);
		bool	flushSend();
		bool	hasDataToSend();
};

#endif