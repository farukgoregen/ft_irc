#include "Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName), topic(""), key(""),
							userLimit(0), inviteOnly(false), topicOpOnly(false) {}

Channel::~Channel() {}

std::string Channel::getName() const { return name; }

std::string Channel::getTopic() const { return topic; }

std::string Channel::getKey() const { return key; }

size_t Channel::getUserLimit() const { return userLimit; }

size_t Channel::getMemberCount() const { return members.size(); }

bool Channel::getInviteOnly() const { return inviteOnly; }

bool Channel::getTopicOpOnly() const { return topicOpOnly; }

void Channel::setTopic(const std::string& newTopic) { topic = newTopic; }

void Channel::setUserLimit(size_t limit) { userLimit = limit; }

void Channel::setInviteOnly(bool invite) { inviteOnly = invite; }

void Channel::setTopicOpOnly(bool opOnly) { topicOpOnly = opOnly; }

void Channel::addClient(Client* client)
{
	if (!isMember(client))
		members.push_back(client);
}

void Channel::removeClient(Client* client)
{
	members.erase(std::remove(members.begin(), members.end(), client), members.end());
	removeOperator(client);
}

void Channel::addOperator(Client* client)
{
	if (!isOperator(client))
		operators.push_back(client);
}

void Channel::removeOperator(Client* client)
{
	operators.erase(std::remove(operators.begin(), operators.end(), client), operators.end());
}

Client* Channel::getMember(const std::string& nick)
{
	for (size_t i = 0; i < members.size(); ++i)
	{
		if (members[i]->getNickname() == nick)
			return members[i];
	}
	return NULL;
}

bool Channel::isMember(Client* client) const
{
	return std::find(members.begin(), members.end(), client) != members.end();
}

bool Channel::isOperator(Client* client) const
{
	return std::find(operators.begin(), operators.end(), client) != operators.end();
}

void Channel::setKey(const std::string& newKey)
{
	key = newKey;
}

void Channel::addInvite(const std::string& nick)
{
	if (!isInvited(nick))
		invitedNicknames.push_back(nick);
}

bool Channel::isInvited(const std::string& nick) const
{
	return std::find(invitedNicknames.begin(), invitedNicknames.end(), nick) != invitedNicknames.end();
}

void Channel::removeInvite(const std::string& nick)
{
	invitedNicknames.erase(std::remove(invitedNicknames.begin(), invitedNicknames.end(), nick), invitedNicknames.end());
}

void Channel::broadcast(const std::string& message, Client* sender)
{
	for (size_t i = 0; i < members.size(); ++i)
	{
		if (members[i] != sender)
			send(members[i]->getFd(), message.c_str(), message.length(), 0);
	}
}
