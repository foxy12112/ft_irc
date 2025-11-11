/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bszikora <bszikora@student.42helbronn.d    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 01:28:43 by bszikora          #+#    #+#             */
/*   Updated: 2025/11/09 14:48:08 by bszikora         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include <iostream>
#include <cerrno>

class Server; // forward declaration instead circular include
#include <string>
#include <iostream>

class Client
{
private:
    int         _clientFD;
    std::string _inBuffer;
    std::string _outBuffer;
	std::string _nickname;
	std::string _username;
    bool        _isAuthenticated;
    bool        _isConnected;
	bool		_isOp;
    int         msgType; //0 for command, 1 for message, 2 for whisper
    std::string _channel;

public:
    Client();
    Client(int fd);
    Client(const Client& other);
    Client& operator=(const Client& other);
    ~Client();

    bool isAuthenticated() const;
    void setAuthenticated(bool v);
	bool isOp() const;
	void setOp(bool v);
    void setMsgType(int type){this->msgType = type;}
    int  getMsgType(void){return this->msgType;}
    std::string getChannel(void){return this->_channel;}
    void        setChannel(std::string channel){this->_channel = channel;}
    bool isConnected() const;
    void disconnect();
    int receive(Client Sender);
    bool extractNextCommand(std::string &cmd);
    void queueResponse(const std::string &resp);
    bool flushSend();
    bool hasDataToSend() const;
    int getFd(void){return this->_clientFD;}
    const std::string& nickname() const;
	void setNickname(const std::string &nick);
    const std::string& username() const;
	void setUsername(const std::string &user);
};

#endif