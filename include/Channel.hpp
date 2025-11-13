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
	Channel(std::string name, std::string topic){this->_name = name, this->_topic = topic, this->_limit = 0;}
	~Channel(){;}
	std::string getName(){return this->_name;}
	std::string getTopic(){return this->_name;}
	std::string getPass(){return this->_pass;}
	int			getLimit(){return this->_limit;}

	void		setTopic(std::string topic){this->_topic = topic;}
	void		setPass(std::string pass){this->_pass = pass;}
	void		setLimit(int limit){this->_limit = limit;}

private:
	std::string	_name;
	std::string	_topic;
	std::string	_pass;
	int			_limit;
	int			_modes;
};

#endif