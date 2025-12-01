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
	_isAuthenticated = other._isAuthenticated;
	_isConnected = other._isConnected;
	_isOp = other._isOp;
	_channelIndex = other._channelIndex;
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
		_isAuthenticated = other._isAuthenticated;
		_isConnected = other._isConnected;
		_isOp = other._isOp;
		_channelIndex = other._channelIndex;
	}
	return (*this);
}

void	Client::disconnect()
{
	if (_isConnected && _clientFd != -1)
		close(_clientFd);
	_isConnected = false;
}

int		Client::receive(Client Sender)
{
	if (_clientFd < 0)
		return (-1);
	if (_channelIndex == Sender._channelIndex)
	{
		char buf[512];
		ssize_t bytes = recv(_clientFd, buf, sizeof(buf) - 1, 0);
		if (bytes > 0)
		{
			buf[bytes] = '\0';
			_inBuffer.append(buf, bytes);
		}
		else if (bytes == 0)
			_isConnected = false;
		else
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				_isConnected = false;
		return (static_cast<int>(bytes));
	}
	return(0);
}

bool	Client::extractNextCommand(std::string &cmd)
{
	size_t pos = _inBuffer.find('\n');
	if (pos == std::string::npos)
		return false;
	cmd = _inBuffer.substr(0, pos);
	_inBuffer.erase(0, pos + 1);
	if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
		cmd.erase(cmd.size() - 1);
	return (true);
}

void	Client::queueRespone(const std::string &resp)
{
	_outBuffer += resp;
}

bool	Client::flushSend()
{
	if (_clientFd < 0 || _outBuffer.empty())
		return (true);
	ssize_t sent = send(_clientFd, _outBuffer.c_str(), _outBuffer.size(), 0);
	if (sent < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return (false);
		_isConnected = false;
		return (false);
	}
	_outBuffer.erase(0, sent);
	return (_outBuffer.empty());
}

bool	Client::hasDataToSend()
{
	return (!_outBuffer.empty());
}
