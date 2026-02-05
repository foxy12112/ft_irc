/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bszikora <bszikora@student.42helbronn.d    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 02:51:03 by bszikora          #+#    #+#             */
/*   Updated: 2026/01/21 00:10:45 by bszikora         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <string>
#include <set>

class Server;

class Channel
{
public:
	Channel(){};
	Channel(std::string name, std::string topic){this->_name = name, this->_topic = topic, this->_users = 0, this->_limit = -1, this->inviteOnly = false, this->topicOpOnly = false;}
	~Channel(){;}
	std::string getName(){return this->_name;}
	std::string getTopic(){return this->_topic;}
	bool		getTopicOp(){return this->topicOpOnly;}
	std::string getPass(){return this->_pass;}
	int			getLimit(){return this->_limit;}
	bool		getInvite(){return this->inviteOnly;}
	int			getUsers(){return (this->_users);}

	// Membership management
	bool		hasMember(int fd) const { return _memberFds.find(fd) != _memberFds.end(); }
	void		addMember(int fd) { _memberFds.insert(fd); _users = _memberFds.size(); }
	void		removeMember(int fd) { _memberFds.erase(fd); _users = _memberFds.size(); _operators.erase(fd); }
	const std::set<int>& members() const { return _memberFds; }

	// Operator management (per-channel)
	bool		isOperator(int fd) const { return _operators.find(fd) != _operators.end(); }
	void		setOperator(int fd, bool op) { if (op) _operators.insert(fd); else _operators.erase(fd); }
	const std::set<int>& operators() const { return _operators; }

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
	std::set<int> _memberFds;
	std::set<int> _operators;
};

#endif