#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <vector>
#include <sys/socket.h>
#include <algorithm>

#include "Client.hpp"

class Channel
{
	private:
		std::string			name;
		std::string			topic;
		std::string			key;
		size_t				userLimit;
		bool				inviteOnly;
		bool				topicOpOnly;
		std::vector<Client*>		members;
		std::vector<Client*>		operators;
		std::vector<std::string>	invitedNicknames;
	
	public:
		Channel(const std::string& channelName);
		~Channel();
	
		std::string getName() const;
		std::string getTopic() const;
		void setTopic(const std::string& newTopic);
		std::string getKey() const;

		void addClient(Client* client);
		void removeClient(Client* client);
		void addOperator(Client* client);
		void removeOperator(Client* client);
		Client* getMember(const std::string& nick);
		std::vector<Client*> getMembers() const;

		bool isMember(Client* client) const;
		bool isOperator(Client* client) const;

		size_t getUserLimit() const;
		size_t getMemberCount() const;
		void setUserLimit(size_t limit);
		bool getInviteOnly() const;
		void setInviteOnly(bool invite);
		bool getTopicOpOnly() const;
		void setTopicOpOnly(bool opOnly);
		void setKey(const std::string& newKey);
		void addInvite(const std::string& nick);
		bool isInvited(const std::string& nick) const;
		void removeInvite(const std::string& nick);

		void broadcast(const std::string& message, Client* sender);
};

#endif