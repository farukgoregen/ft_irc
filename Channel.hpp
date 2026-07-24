#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include "Client.hpp"

class Channel {
private:
    std::string name;
    std::string topic;
    std::string key;
    size_t      userLimit;
    bool        inviteOnly;
    bool        topicOpOnly;

    std::vector<Client*> members;
    std::vector<Client*> operators;
    std::vector<std::string> invitedNicknames;

public:
    Channel(const std::string& channelName);
    ~Channel();

    std::string getName() const;
    std::string getTopic() const;
    void setTopic(const std::string& newTopic);

    // Kanal Üye Yönetimi
    void addClient(Client* client);
    void removeClient(Client* client);
    void addOperator(Client* client);
    void removeOperator(Client* client);
    
    bool isMember(Client* client) const;
    bool isOperator(Client* client) const;

    // Kanal Modları (Getter ve Setter)
    size_t getUserLimit() const;
    void setUserLimit(size_t limit);

    bool getInviteOnly() const;
    void setInviteOnly(bool invite);

    bool getTopicOpOnly() const;
    void setTopicOpOnly(bool opOnly);

    // Kanaldaki Herkese Mesaj Gönderme
    void broadcast(const std::string& message, Client* sender);
};

#endif