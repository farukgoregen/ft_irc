#include "Channel.hpp"
#include <algorithm>

Channel::Channel(const std::string& channelName) : name(channelName), topic(""), key(""), userLimit(0), inviteOnly(false), topicOpOnly(false) {}

Channel::~Channel() {}

std::string Channel::getName() const 
{ 
    return name; 
}

std::string Channel::getTopic() const 
{ 
    return topic; 
}

void Channel::setTopic(const std::string& newTopic) 
{ 
    topic = newTopic; 
}

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

bool Channel::isMember(Client* client) const 
{
    return std::find(members.begin(), members.end(), client) != members.end();
}

bool Channel::isOperator(Client* client) const 
{
    return std::find(operators.begin(), operators.end(), client) != operators.end();
}

size_t Channel::getUserLimit() const 
{ 
    return userLimit; 
}

void Channel::setUserLimit(size_t limit) 
{ 
    userLimit = limit; 
}

bool Channel::getInviteOnly() const 
{ 
    return inviteOnly; 
}

void Channel::setInviteOnly(bool invite) 
{ 
    inviteOnly = invite; 
}

bool Channel::getTopicOpOnly() const 
{ 
    return topicOpOnly; 
}

void Channel::setTopicOpOnly(bool opOnly) 
{ 
    topicOpOnly = opOnly; 
}

void Channel::broadcast(const std::string& message, Client* sender) 
{
    for (size_t i = 0; i < members.size(); ++i) 
    {
        if (members[i] != sender) 
            // Mesajı gönderen hariç herkese ilet
            send(members[i]->getFd(), message.c_str(), message.length(), 0);
    }
}