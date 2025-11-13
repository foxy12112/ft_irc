/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julcalde <julcalde@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 01:29:09 by bszikora          #+#    #+#             */
/*   Updated: 2025/11/11 16:04:51 by julcalde         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>

Client::Client()
: _clientFD(-1), _inBuffer(), _outBuffer(), _nickname(), _username(), _isAuthenticated(false), _isConnected(false), _isOp(false), _channel("#General"), _invited(false)
{}

Client::Client(int fd)
: _clientFD(fd), _inBuffer(), _outBuffer(), _nickname(), _username(), _isAuthenticated(false), _isConnected(true), _isOp(false), _channel("#General"), _invited(false)
{}

Client::Client(const Client& other)
: _clientFD(other._clientFD), _inBuffer(other._inBuffer), _outBuffer(other._outBuffer), _nickname(other._nickname), _username(other._username), _isAuthenticated(other._isAuthenticated), _isConnected(other._isConnected), _isOp(other._isOp), _channel(other._channel), _invited(other._invited), _invitedBy(other._invitedBy)
{}

Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		_clientFD = other._clientFD;
		_inBuffer = other._inBuffer;
		_outBuffer = other._outBuffer;
		_nickname = other._nickname;
		_username = other._username;
		_isAuthenticated = other._isAuthenticated;
		_isConnected = other._isConnected;
		_channel = other._channel;
		_invited = other._invited;
		_invitedBy = other._invitedBy;
	}
	return *this;
}

Client::~Client() {}

bool Client::isAuthenticated() const { return _isAuthenticated; }
void Client::setAuthenticated(bool v) { _isAuthenticated = v; }

bool Client::isOp() const {return _isOp; }
void Client::setOp(bool v) { _isOp = v; }

bool Client::isConnected() const { return _isConnected; }
bool Client::hasDataToSend() const { return !_outBuffer.empty(); }
const std::string& Client::nickname() const { return _nickname; }
const std::string& Client::username() const { return _username; }

void Client::setNickname(const std::string &nick) { _nickname = nick; }
void Client::setUsername(const std::string &user) { _username = user; }

void Client::disconnect()
{
	if (_isConnected && _clientFD != -1)
		close(_clientFD);
	_isConnected = false;
}

int Client::receive(Client Sender)
{
	if (_clientFD < 0)
		return -1;
	if (this->getChannel() == Sender.getChannel())
	{
		char buf[512]; // RFC 1459 recommends max 512 bytes messages
		ssize_t bytes = recv(_clientFD, buf, sizeof(buf) - 1, 0);
		if (bytes > 0)
		{
			char buf[1024];
			ssize_t bytes = recv(_clientFD, buf, sizeof(buf) - 1, 0);
			if (bytes > 0)
			{
				buf[bytes] = '\0';
				_inBuffer.append(buf, bytes);
			}
			else if (bytes == 0)
				_isConnected = false;
			else
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					_isConnected = false;
			}
			return static_cast<int>(bytes);
		}
	}
	return (0);
}

bool Client::extractNextCommand(std::string &cmd)
{
	size_t pos = _inBuffer.find('\n');
	if (pos == std::string::npos)
		return false;
	cmd = _inBuffer.substr(0, pos);
	_inBuffer.erase(0, pos + 1);
	if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
		cmd.erase(cmd.size() - 1);
	return true;
}

void Client::queueResponse(const std::string &resp)
{
	_outBuffer += resp;
}

bool Client::flushSend()
{
	if (_clientFD < 0 || _outBuffer.empty())
		return true;
	ssize_t sent = send(_clientFD, _outBuffer.c_str(), _outBuffer.size(), 0);
	if (sent < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return false; // try later
		_isConnected = false; // fatal
		return false;
	}
	_outBuffer.erase(0, sent);
	return _outBuffer.empty();
}
