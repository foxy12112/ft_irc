
#include "Channel.hpp"

// Default constructor
Channel::Channel()	: _name(), _topic(), _pass(), topicOpOnly(false), inviteOnly(false), _limit(-1), _users(0), _modes(0)
{}

// Constructor with name and topic
Channel::Channel(std::string name, std::string topic)
	: _name(name), _topic(topic), _pass(), topicOpOnly(false), inviteOnly(false), _limit(-1), _users(0), _modes(0)
{}

// Destructor
Channel::~Channel()
{}

std::string Channel::getName()
{
	return this->_name;
}

std::string Channel::getTopic()
{
	return this->_topic;
}

bool Channel::getTopicOp()
{
	return this->topicOpOnly;
}

std::string Channel::getPass()
{
	return this->_pass;
}

int Channel::getLimit()
{
	return this->_limit;
}

bool Channel::getInvite()
{
	return this->inviteOnly;
}

int Channel::getUsers()
{
	return (this->_users);
}

bool Channel::hasMember(int fd) const
{
	return _memberFds.find(fd) != _memberFds.end();
}

void Channel::addMember(int fd)
{
	_memberFds.insert(fd);
	_users = _memberFds.size();
}

void Channel::removeMember(int fd)
{
	_memberFds.erase(fd);
	_users = _memberFds.size();
	_operators.erase(fd);
}

const std::set<int> &Channel::members() const
{
	return _memberFds;
}

bool Channel::isOperator(int fd) const
{
	return _operators.find(fd) != _operators.end();
}

void Channel::setOperator(int fd, bool op)
{
	if (op)
		_operators.insert(fd);
	else
		_operators.erase(fd);
}

const std::set<int> &Channel::operators() const
{
	return _operators;
}

void Channel::setUsers(int users)
{
	this->_users = users;
}

void Channel::setTopic(std::string topic)
{
	this->_topic = topic;
}

void Channel::setTopicOp(bool status)
{
	this->topicOpOnly = status;
}

void Channel::setPass(std::string pass)
{
	this->_pass = pass;
}

void Channel::setLimit(int limit)
{
	this->_limit = limit;
}

void Channel::setInvite(bool status)
{
	this->inviteOnly = status;
}

Channel &Channel::operator=(const Channel &other)
{
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
