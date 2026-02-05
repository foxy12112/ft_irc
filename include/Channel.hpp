/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bszikora <bszikora@student.42helbronn.d    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 02:51:03 by bszikora          #+#    #+#             */
/*   Updated: 2026/02/05 16:06:09 by bszikora         ###   ########.fr       */
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
	Channel();
	Channel(std::string name, std::string topic);
	~Channel();
	std::string getName();
	std::string getTopic();
	bool		getTopicOp();
	std::string getPass();
	int			getLimit();
	bool		getInvite();
	int		getUsers();

	// Membership management
	bool		hasMember(int fd) const;
	void		addMember(int fd);
	void		removeMember(int fd);
	const std::set<int>& members() const;

	// Operator management (per-channel)
	bool		isOperator(int fd) const;
	void		setOperator(int fd, bool op);
	const std::set<int>& operators() const;

	void		setUsers(int users);
	void		setTopic(std::string topic);
	void		setTopicOp(bool status);
	void		setPass(std::string pass);
	void		setLimit(int limit);
	void		setInvite(bool status);
	Channel		&operator=(const Channel &other);
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