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
	Channel(){};
	Channel(std::string name, std::string topic){this->_name = name, this->_topic = topic, this->_users = 0, this->_limit = 0, this->inviteOnly = false, this->topicOpOnly = false;}
	~Channel(){;}
	std::string getName(){return this->_name;}
	std::string getTopic(){return this->_topic;}
	bool		getTopicOp(){return this->topicOpOnly;}
	std::string getPass(){return this->_pass;}
	int			getLimit(){return this->_limit;}
	bool		getInvite(){return this->inviteOnly;}
	int			getUsers(){return (this->_users);}

	void		setUsers(int users){this->_users = users;}
	void		setTopic(std::string topic){this->_topic = topic;}
	void		setTopicOp(bool status){this->topicOpOnly = status;}
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
			this->topicOpOnly = other.topicOpOnly;
			this->_users = other._users;
		}
		return (*this);
	}
private:
	std::string	_name;
	std::string	_topic;
	std::string	_pass;
	bool		topicOpOnly;
	bool		inviteOnly;
	int			_limit;
	int			_users;
	int			_modes;
};

#endif