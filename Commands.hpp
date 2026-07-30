#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include <string>
#include <vector>
#include <map>
#include "Client.hpp"
#include "Channel.hpp"

class Server; // Sadece Server için Forward Declaration yeterlidir

class Commands
{
	private:
		Server* _server;
	
		// Yardımcı Parsing/Yanıt Fonksiyonları
		std::vector<std::string> split(const std::string& str);
		void sendReply(int fd, const std::string& reply);
	
		// Bireysel Komut Metotları
		void handlePass(Client* client, const std::vector<std::string>& args, const std::string& pass);
		void handleNick(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients);
		void handleUser(Client* client, const std::vector<std::string>& args);
		void handleJoin(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels);
		void handlePrivmsg(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels);
		void handleKick(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels);
		void handleInvite(Client* client, const std::vector<std::string>& args, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels);
		void handleTopic(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels);
		void handleMode(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels);
		void handlePing(Client* client, const std::vector<std::string>& args);
		void handlePart(Client* client, const std::vector<std::string>& args, std::map<std::string, Channel*>& channels);
	
	public:
		Commands(Server* server);
		~Commands();
	
		// Ana Çağrı
		void execute(int clientFd, const std::string& rawCmd, std::map<int, Client*>& clients, std::map<std::string, Channel*>& channels, const std::string& serverPass);
};

#endif