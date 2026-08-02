#include "Commands.hpp"
#include "Server.hpp"

Commands::Commands(Server* server) : _server(server) {}

Commands::~Commands() {}

void Commands::sendReply(int fd, const std::string& reply)
{
	std::string msg = reply + "\r\n";
	_server->sendData(fd, msg);
}

std::vector<std::string> Commands::split(const std::string& str)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (ss >> token)
		tokens.push_back(token);
	return tokens;
}

void Commands::execute(int clientFd, const std::string& rawCmd, std::map<int, Client*>& clients,
			std::map<std::string, Channel*>& channels, const std::string& serverPass)
{
	if (rawCmd.empty() || clients.find(clientFd) == clients.end())
		return;

	Client* client = clients[clientFd];
	std::vector<std::string> args = split(rawCmd);

	if (args.empty())
		return;
	
	std::string cmd = args[0];

	for (size_t i = 0; i < cmd.length(); ++i)
		cmd[i] = std::toupper(cmd[i]);

	if (cmd == "PASS")
	{
		handlePass(client, args, serverPass);
		return;
	}

	if (cmd == "NICK")
	{
		handleNick(client, args, clients, channels);
		return;
	}

	if (cmd == "USER")
	{
		handleUser(client, args);
		return;
	}

	if (!client->getIsRegistered())
	{
		sendReply(clientFd, "451 * :You have not registered");
		return;
	}

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
	else if (cmd == "PING")
		handlePing(client, args);
	else if (cmd == "PART")
		handlePart(client, args, channels);
}

void Commands::handlePass(Client* client, const std::vector<std::string>& args, const std::string& pass)
{
	if (args.size() < 2)
	{
		sendReply(client->getFd(), "461 PASS :Not enough parameters");
		return;
	}

	if (client->getIsRegistered())
	{
		sendReply(client->getFd(), "462 :Unauthorized command (already registered)");
		return;
	}

	if (args[1] == pass)
		client->setIsAuthenticated(true);
	else
		sendReply(client->getFd(), "464 :Password incorrect");
}

void Commands::handleNick(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels)
{
	if (!client->getIsAuthenticated())
	{
		sendReply(client->getFd(), "451 :You have not registered (Password required first)");
		return;
	}

	if (args.size() < 2)
	{
		sendReply(client->getFd(), "431 :No nickname given");
		return;
	}

	std::string newNick = args[1];

	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second->getNickname() == newNick)
		{
			sendReply(client->getFd(), "433 * " + newNick + " :Nickname is already in use");
			return;
		}
	}

	std::string oldNick = client->getNickname();
	client->setNickname(newNick);

	if (!client->getUsername().empty() && !client->getIsRegistered())
	{
		client->setIsRegistered(true);
		sendReply(client->getFd(), "001 " + newNick + " :Welcome to the FT_IRC Network!");
	}
	else if (client->getIsRegistered())
	{
		std::string nickChangeMsg = ":" + oldNick + " NICK :" + newNick;
		sendReply(client->getFd(), nickChangeMsg);
		for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
		{
			if (it->second->isMember(client))
				it->second->broadcast(nickChangeMsg + "\r\n", client);
		}
	}
}

void Commands::handleUser(Client* client, const std::vector<std::string>& args)
{
	if (!client->getIsAuthenticated())
	{
		sendReply(client->getFd(), "451 :You have not registered (Password required first)");
		return; 
	}

	if (args.size() < 5) // RFC 2812 standardı: USER <username> <hostname> <servername> <realname> -> USER ahmet 0 * :Ahmet Yilmaz
	{
		sendReply(client->getFd(), "461 USER :Not enough parameters");
		return; 
	}

	if (client->getIsRegistered())
	{
		sendReply(client->getFd(), "462 :Unauthorized command (already registered)");
		return;
	}

	client->setUsername(args[1]);
	client->setHostname(args[3]);

	if (!client->getNickname().empty() && !client->getIsRegistered())
	{
		client->setIsRegistered(true);
		sendReply(client->getFd(), "001 " + client->getNickname() + " :Welcome to the FT_IRC Network!");
	}
}

void Commands::handleJoin(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 2)
	{
		sendReply(client->getFd(), "461 JOIN :Not enough parameters");
		return;
	}

	std::string chName = args[1];

	if (chName[0] != '#')
	{
		sendReply(client->getFd(), "476 " + chName + " :Bad Channel Mask");
		return;
	}

	if (channels.find(chName) == channels.end())
	{
		channels[chName] = new Channel(chName);
		channels[chName]->addClient(client);
		channels[chName]->addOperator(client);
	}
	else
	{
		Channel* ch = channels[chName];

		if (ch->getInviteOnly() && !ch->isInvited(client->getNickname()))
		{
			sendReply(client->getFd(), "473 " + chName + " :Cannot join channel (+i)");
			return;  
		}

		if (!ch->getKey().empty())
		{
			if (args.size() < 3 || args[2] != ch->getKey())
			{
				sendReply(client->getFd(), "475 " + chName + " :Cannot join channel (+k)");
				return;
			}
		}
		if (ch->getUserLimit() > 0 && ch->getMemberCount() >= ch->getUserLimit())
		{
			sendReply(client->getFd(), "471 " + chName + " :Cannot join channel (+l)");
			return;
		}
		
		ch->addClient(client);

		if (ch->getInviteOnly())
			ch->removeInvite(client->getNickname());
	}

	std::string joinMsg = ":" + client->getNickname() + " JOIN :" + chName;
	channels[chName]->broadcast(joinMsg + "\r\n", client);
	sendReply(client->getFd(), joinMsg);
}

void Commands::handlePrivmsg(Client* client, const std::vector<std::string>& args,
				std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 3)
	{
		sendReply(client->getFd(), "461 PRIVMSG :Not enough parameters");
		return;
	}

	std::string target = args[1];
	std::string msg = args[2];

	for (size_t i = 3; i < args.size(); ++i)
		msg += " " + args[i];

	std::string senderPrefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();

	if (target[0] == '#')
	{
		if (channels.find(target) != channels.end())
		{
			Channel* ch = channels[target];

			if (!ch->isMember(client))
			{
				sendReply(client->getFd(), "404 " + target + " :Cannot send to channel");
				return;
			}
			
			ch->broadcast(senderPrefix + " PRIVMSG " + target + " " + msg + "\r\n", client);
		}
		else
			sendReply(client->getFd(), "403 " + target + " :No such channel");
	}
	else
	{
		bool found = false;

		for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->second->getNickname() == target)
			{
				sendReply(it->second->getFd(), senderPrefix + " PRIVMSG " + target + " " + msg);
				found = true;
				break;
			}
		}
		if (!found)
			sendReply(client->getFd(), "401 " + target + " :No such nick/channel");
	}
}

void Commands::handleKick(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 3)
	{
		sendReply(client->getFd(), "461 KICK :Not enough parameters");
		return;
	}

	std::string chName = args[1];
	std::string targetNick = args[2];
	
	if (channels.find(chName) == channels.end())
	{
		sendReply(client->getFd(), "403 " + chName + " :No such channel");
		return;
	}

	Channel* ch = channels[chName];

	if (!ch->isOperator(client))
	{
		sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
		return;
	}

	Client* targetToKick = ch->getMember(targetNick);

	if (!targetToKick)
	{
		sendReply(client->getFd(), "441 " + targetNick + " " + chName + " :They aren't on that channel");
		return;
	}

	if (client->getNickname() == targetNick)
	{
		sendReply(client->getFd(), "481 :Cannot kick yourself");
		return;
	}

	std::string reason = "Kicked by operator";
	
	if (args.size() > 3)
	{
		reason = args[3];
		for (size_t i = 4; i < args.size(); ++i)
			reason += " " + args[i];
		
		if (reason[0] == ':')
			reason.erase(0, 1);
	}

	std::string senderPrefix = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
	std::string kickMsg = senderPrefix + " KICK " + chName + " " + targetNick + " :" + reason;

	ch->broadcast(kickMsg + "\r\n", NULL);
	ch->removeClient(targetToKick);
}

void Commands::handleInvite(Client* client, const std::vector<std::string>& args,
				std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 3)
	{
		sendReply(client->getFd(), "461 INVITE :Not enough parameters");
		return;
	}

	std::string targetNick = args[1];
	std::string chName = args[2];

	if (channels.find(chName) == channels.end())
	{
		sendReply(client->getFd(), "403 " + chName + " :No such channel");
		return;
	}

	Channel* ch = channels[chName];

	if (!ch->isMember(client))
	{
		sendReply(client->getFd(), "442 " + chName + " :You're not on that channel");
		return;
	}

	if (ch->getInviteOnly() && !ch->isOperator(client))
	{
		sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
		return;
	}

	bool targetExists = false;
	Client* targetClient = NULL;

	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second->getNickname() == targetNick)
		{
			targetExists = true;
			targetClient = it->second;
			break;
		}
	}

	if (!targetExists)
	{
		sendReply(client->getFd(), "401 " + targetNick + " :No such nick/channel");
		return;
	}

	if (ch->isMember(targetClient))
	{
		sendReply(client->getFd(), "443 " + targetNick + " " + chName + " :is already on channel");
		return;
	}

	ch->addInvite(targetNick);
	sendReply(client->getFd(), "341 " + client->getNickname() + " " + targetNick + " " + chName);
	sendReply(targetClient->getFd(), ":" + client->getNickname() + " INVITE " + targetNick + " :" + chName);
}

void Commands::handleTopic(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels)
{ 
	if (args.size() < 2)
		return;

	std::string chName = args[1];

	if (channels.find(chName) == channels.end())
		return;

	Channel* ch = channels[chName];

	if (args.size() == 2)
		sendReply(client->getFd(), "332 " + client->getNickname() + " " + chName + " :" + ch->getTopic());
	else
	{
		if (ch->getTopicOpOnly() && !ch->isOperator(client))
		{
			sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
			return;
		}
		ch->setTopic(args[2]);
		ch->broadcast(":" + client->getNickname() + " TOPIC " + chName + " :" + args[2] + "\r\n", NULL);
	}
}

void Commands::handleMode(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 3)
		return;

	std::string chName = args[1];
	std::string mode = args[2];

	if (channels.find(chName) == channels.end())
	{
		sendReply(client->getFd(), "403 " + chName + " :No such channel");
		return;
	}

	Channel* ch = channels[chName];

	if (!ch->isOperator(client))
	{
		sendReply(client->getFd(), "482 " + chName + " :You're not channel operator");
		return;
	}

	if (mode == "+i")
		ch->setInviteOnly(true);
	else if (mode == "-i")
		ch->setInviteOnly(false);
	else if (mode == "+t")
		ch->setTopicOpOnly(true);
	else if (mode == "-t")
		ch->setTopicOpOnly(false);
	else if (mode == "+k")
	{
		if (args.size() > 3)
			ch->setKey(args[3]);
	}
	else if (mode == "-k")
		ch->setKey("");
	else if (mode == "+l")
	{
		if (args.size() > 3)
			ch->setUserLimit(std::atoi(args[3].c_str()));
	}
	else if (mode == "+o")
	{
		if (args.size() > 3) 
		{
			Client* target = ch->getMember(args[3]);

			if (target)
				ch->addOperator(target);
			else
				sendReply(client->getFd(), "441 " + args[3] + " " + chName + " :They aren't on that channel");
		}
	}
	else if (mode == "-o")
	{
		if (args.size() > 3)
		{
			Client* target = ch->getMember(args[3]);

			if (target)
				ch->removeOperator(target);
			else
				sendReply(client->getFd(), "441 " + args[3] + " " + chName + " :They aren't on that channel");
		}
	}
	else if (mode == "-l")
		ch->setUserLimit(0);

	std::string modeMsg = ":" + client->getNickname() + " MODE " + chName + " " + mode;

	if (args.size() > 3 && (mode == "+k" || mode == "+l" || mode == "+o" || mode == "-o"))
		modeMsg += " " + args[3];
	ch->broadcast(modeMsg + "\r\n", NULL);
}

void Commands::handlePing(Client* client, const std::vector<std::string>& args)
{
	if (args.size() < 2)
		return;
	sendReply(client->getFd(), "PONG " + args[1]); 
}

void Commands::handlePart(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels)
{
	if (args.size() < 2)
	{
		sendReply(client->getFd(), "461 PART :Not enough parameters");
		return;
	}

	std::string chName = args[1];
	
	if (channels.find(chName) == channels.end())
	{
		sendReply(client->getFd(), "403 " + chName + " :No such channel");
		return;
	}

	Channel* ch = channels[chName];
	
	if (!ch->isMember(client))
	{
		sendReply(client->getFd(), "442 " + chName + " :You're not on that channel");
		return;
	}

	std::string reason = (args.size() > 2) ? args[2] : "Leaving";
	std::string partMsg = ":" + client->getNickname() +
				"!" + client->getUsername() +
				"@" + client->getHostname() +
				" PART " + chName + " :" + reason;

	ch->broadcast(partMsg + "\r\n", NULL);
	ch->removeClient(client);
}
