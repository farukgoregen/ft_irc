#include "Commands.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>

Commands::Commands(Server* server) : _server(server) {}
Commands::~Commands() {}

void Commands::sendReply(int fd, const std::string& reply) {
    std::string msg = reply + "\r\n";
    _server->sendData(fd, msg);
}

std::vector<std::string> Commands::split(const std::string& str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (ss >> token)
        tokens.push_back(token);
    return tokens;
}

void Commands::execute(int clientFd, const std::string& rawCmd, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels, const std::string& serverPass) {
    if (rawCmd.empty() || clients.find(clientFd) == clients.end())
        return;

    Client* client = clients[clientFd];
    std::vector<std::string> args = split(rawCmd);
    if (args.empty()) return;

    std::string cmd = args[0];
    for (size_t i = 0; i < cmd.length(); ++i)
        cmd[i] = std::toupper(cmd[i]);

    // Kayıt Öncesi Komutlar
    if (cmd == "PASS") {
        handlePass(client, args, serverPass);
        return;
    }
    if (cmd == "NICK") {
        handleNick(client, args, clients);
        return;
    }
    if (cmd == "USER") {
        handleUser(client, args);
        return;
    }

    // Kullanıcı doğrulanmadıysa diğer komutları engelle
    if (!client->getIsRegistered()) {
        sendReply(clientFd, "451 * :You have not registered");
        return;
    }

    // Kayıt Sonrası Operasyonel Komutlar
    if (cmd == "JOIN")
        handleJoin(client, args, channels);
    else if (cmd == "PRIVMSG")
        handlePrivmsg(client, args, clients, channels);
    else if (cmd == "KICK")
        handleKick(client, args, channels);
    else if (cmd == "INVITE")
        handleInvite(client, args, clients, channels);
    else if (cmd == "TOPIC")
        handleTopic(client, args, channels);
    else if (cmd == "MODE")
        handleMode(client, args, channels);
}

void Commands::handlePass(Client* client, const std::vector<std::string>& args, const std::string& pass) {
    if (args.size() < 2) {
        sendReply(client->getFd(), "461 PASS :Not enough parameters");
        return;
    }
    if (client->getIsRegistered()) {
        sendReply(client->getFd(), "462 :Unauthorized command (already registered)");
        return;
    }
    if (args[1] == pass) {
        client->setIsAuthenticated(true); // ŞİFRE ONAYLANDI!
    } else {
        sendReply(client->getFd(), "464 :Password incorrect");
    }
}

void Commands::handleNick(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients) {
    // ŞİFRE KONTROLÜ
    if (!client->getIsAuthenticated()) {
        sendReply(client->getFd(), "451 :You have not registered (Password required first)");
        return;
    }

    if (args.size() < 2) {
        sendReply(client->getFd(), "431 :No nickname given");
        return;
    }
    std::string newNick = args[1];
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getNickname() == newNick) {
            sendReply(client->getFd(), "433 * " + newNick + " :Nickname is already in use");
            return;
        }
    }
    client->setNickname(newNick);
    if (!client->getUsername().empty() && !client->getIsRegistered()) {
        client->setIsRegistered(true);
        sendReply(client->getFd(), "001 " + newNick + " :Welcome to the FT_IRC Network!");
    }
}

void Commands::handleUser(Client* client, const std::vector<std::string>& args) {
    // ŞİFRE KONTROLÜ
    if (!client->getIsAuthenticated()) {
        sendReply(client->getFd(), "451 :You have not registered (Password required first)");
        return;
    }

    if (args.size() < 5) {
        sendReply(client->getFd(), "461 USER :Not enough parameters");
        return;
    }
    if (client->getIsRegistered()) {
        sendReply(client->getFd(), "462 :Unauthorized command (already registered)");
        return;
    }
    client->setUsername(args[1]);
    client->setHostname(args[3]);
    if (!client->getNickname().empty() && !client->getIsRegistered()) {
        client->setIsRegistered(true);
        sendReply(client->getFd(), "001 " + client->getNickname() + " :Welcome to the FT_IRC Network!");
    }
}

void Commands::handleJoin(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels) {
    if (args.size() < 2) {
        sendReply(client->getFd(), "461 JOIN :Not enough parameters");
        return;
    }
    std::string chName = args[1];
    if (chName[0] != '#') {
        sendReply(client->getFd(), "476 " + chName + " :Bad Channel Mask");
        return;
    }

    if (channels.find(chName) == channels.end()) {
        channels[chName] = new Channel(chName);
        channels[chName]->addClient(client);
        channels[chName]->addOperator(client); // Ilk giren operatör olur
    } else {
        Channel* ch = channels[chName];
        if (ch->getInviteOnly()) {
            sendReply(client->getFd(), "473 " + chName + " :Cannot join channel (+i)");
            return;
        }
        ch->addClient(client);
    }
    std::string joinMsg = ":" + client->getNickname() + " JOIN :" + chName;
    channels[chName]->broadcast(joinMsg + "\r\n", NULL);
}

void Commands::handlePrivmsg(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels) {
    if (args.size() < 3) {
        sendReply(client->getFd(), "461 PRIVMSG :Not enough parameters");
        return;
    }
    std::string target = args[1];
    std::string msg = args[2];
    for (size_t i = 3; i < args.size(); ++i) msg += " " + args[i];

    if (target[0] == '#') {
        if (channels.find(target) != channels.end()) {
            channels[target]->broadcast(":" + client->getNickname() + " PRIVMSG " + target + " " + msg + "\r\n", client);
        } else {
            sendReply(client->getFd(), "403 " + target + " :No such channel");
        }
    } else {
        bool found = false;
        for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
            if (it->second->getNickname() == target) {
                sendReply(it->second->getFd(), ":" + client->getNickname() + " PRIVMSG " + target + " " + msg);
                found = true;
                break;
            }
        }
        if (!found) sendReply(client->getFd(), "401 " + target + " :No such nick/channel");
    }
}

void Commands::handleKick(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels) {
    if (args.size() < 3) {
        sendReply(client->getFd(), "461 KICK :Not enough parameters");
        return;
    }
    std::string chName = args[1];
    std::string targetNick = args[2];

    if (channels.find(chName) == channels.end()) {
        sendReply(client->getFd(), "403 " + chName + " :No such channel");
        return;
    }
    Channel* ch = channels[chName];
    if (!ch->isOperator(client)) {
        sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
        return;
    }
    // Atılacak kullanıcıyı bul ve çıkar
    ch->broadcast(":" + client->getNickname() + " KICK " + chName + " " + targetNick + "\r\n", NULL);
}

void Commands::handleInvite(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels) {
    if (args.size() < 3) return;
    (void)clients; (void)channels;
    sendReply(client->getFd(), "341 " + client->getNickname() + " " + args[1] + " " + args[2]);
}

void Commands::handleTopic(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels) {
    if (args.size() < 2) return;
    std::string chName = args[1];
    if (channels.find(chName) == channels.end()) return;
    
    Channel* ch = channels[chName];
    if (args.size() == 2) {
        sendReply(client->getFd(), "332 " + client->getNickname() + " " + chName + " :" + ch->getTopic());
    } else {
        if (ch->getTopicOpOnly() && !ch->isOperator(client)) {
            sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
            return;
        }
        ch->setTopic(args[2]);
        ch->broadcast(":" + client->getNickname() + " TOPIC " + chName + " :" + args[2] + "\r\n", NULL);
    }
}

void Commands::handleMode(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels) {
    if (args.size() < 3) return;
    std::string chName = args[1];
    std::string mode = args[2];

    if (channels.find(chName) == channels.end()) return;
    Channel* ch = channels[chName];
    if (!ch->isOperator(client)) {
        sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
        return;
    }
    if (mode == "+i") ch->setInviteOnly(true);
    else if (mode == "-i") ch->setInviteOnly(false);
    else if (mode == "+t") ch->setTopicOpOnly(true);
    else if (mode == "-t") ch->setTopicOpOnly(false);
}