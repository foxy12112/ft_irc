/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bszikora <bszikora@student.42helbronn.d    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 02:51:03 by bszikora          #+#    #+#             */
/*   Updated: 2025/11/09 02:51:33 by bszikora         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <string>

class Server;

class Channel
{
public:
	enum ModeFlags
	{
		MODE_INVITE_ONLY = 1 << 0,
		MODE_TOPIC_OPONLY = 1 << 1,
		MODE_PASSWORD = 1 << 2,
		MODE_USER_LIMIT = 1 << 3,
	};
	Channel(){};
	Channel(std::string name, std::string topic){this->_name = name, this->_topic = topic, this->_limit = 0, this->inviteOnly = false;}
	~Channel(){;}
	std::string getName(){return this->_name;}
	std::string getTopic(){return this->_name;}
	std::string getPass(){return this->_pass;}
	int			getLimit(){return this->_limit;}
	bool		getInvite(){return this->inviteOnly;}

	void		setTopic(std::string topic){this->_topic = topic;}
	void		setPass(std::string pass){this->_pass = pass;}
	void		setLimit(int limit){this->_limit = limit;}
	void		setInvite(bool status){this->inviteOnly = status;}
	Channel &operator=(const Channel &other){
		if (this != &other)
		{
			this->_name = other._name;
			this->_topic = other._topic;
			this->_pass = other._pass;
			this->_limit = other._limit;
			this->_modes = other._modes;
			this->inviteOnly = other.inviteOnly;
		}
		return (*this);
	}
private:
	std::string	_name;
	std::string	_topic;
	std::string	_pass;
	bool		inviteOnly;
	int			_limit;
	int			_modes;
};

#endif