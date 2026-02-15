#include "newClient.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>


Client::Client(const Client &other)
{
	_clientFd = other._clientFd;
	_index = other._index;
	_inBuffer = other._inBuffer;
	_outBuffer = other._outBuffer;
	_nickName = other._nickName;
	_userName = other._userName;
	_realName = other._realName;
	_hostname = other._hostname;
	_isAuthenticated = other._isAuthenticated;
	_isConnected = other._isConnected;
	_isServerOp = other._isServerOp;
	_channelIndex = other._channelIndex;
	_invitedIndex = other._invitedIndex;
	_invitedClient = other._invitedClient;
	_wasInvited = other._wasInvited;
}

Client &Client::operator=(const Client &other)
{
	if (this != &other)
	{
		_clientFd = other._clientFd;
		_index = other._index;
		_inBuffer = other._inBuffer;
		_outBuffer = other._outBuffer;
		_nickName = other._nickName;
		_userName = other._userName;
		_realName = other._realName;
		_hostname = other._hostname;
		_isAuthenticated = other._isAuthenticated;
		_isConnected = other._isConnected;
		_isServerOp = other._isServerOp;
		_channelIndex = other._channelIndex;
		_invitedIndex = other._invitedIndex;
		_invitedClient = other._invitedClient;
		_wasInvited = other._wasInvited;
	}
	return (*this);
}

// Disconnects client and closes socket
void	Client::disconnect()
{
	if (_isConnected && _clientFd != -1)
		close(_clientFd);
	_isConnected = false;
}

// Receives data from client socket, appends to input buffer
int		Client::receive(Client Sender)
{
	if (_clientFd < 0)
		return (-1);
	Sender.getAuth();
	char buf[512];
	ssize_t bytes = recv(_clientFd, buf, sizeof(buf) - 1, 0);
	if (bytes > 0)
	{
		if (_inBuffer.size() + bytes > 512) // RFC max message size
		{
			queueResponse(":" + _hostname + " 500 :Input too long\r\n");
			return (bytes);
		}
		buf[bytes] = '\0';
		_inBuffer.append(buf, bytes);
	}
	else if (bytes == 0)
		_isConnected = false;
	else
		return (-1);
	return (static_cast<int>(bytes));
}

// Extracts next complete IRC command from input buffer
bool	Client::extractNextCommand(std::string &cmd)
{
	size_t pos = _inBuffer.find("\n");
	if (pos == std::string::npos)
		return (false);
	cmd = _inBuffer.substr(0, pos);
	_inBuffer.erase(0, pos + 1);
	if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
		cmd.erase(cmd.size() - 1);
	return (true);
}

// Queues a response message to output buffer
void	Client::queueResponse(const std::string &resp)
{
	_outBuffer += resp;
}

// Sends data from output buffer to client
bool	Client::flushSend()
{
	if (_clientFd < 0 || _outBuffer.empty())
		return (true);
	ssize_t sent = send(_clientFd, _outBuffer.c_str(), _outBuffer.size(), 0);
	if (sent < 0)
		return (false);
	_outBuffer.erase(0, sent);
	return (_outBuffer.empty());
}

// Checks if there is data pending to send
bool	Client::hasDataToSend()
{
	return (!_outBuffer.empty());
}