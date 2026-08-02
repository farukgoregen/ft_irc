#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <map>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <errno.h>
#include "Channel.hpp"

class Commands;

class Server {
private:
	int				port;
	std::string			password;
	int				serverFd;
	std::vector<pollfd>		pollFds;
	std::map<int, Client*>		clients;
	std::map<std::string, Channel*>	channels;
	Commands*			cmdHandler;
	static bool			signal;

	void setNonBlocking(int fd);
	void acceptNewClient();
	bool handleClientData(int clientFd, size_t pollIdx);
	void disconnectClient(int clientFd, size_t pollIdx);

public:
	Server(int port, const std::string& password);
	~Server();

	static void signalHandler(int signum);
	void init();
	void run();
	void sendData(int clientFd, const std::string& message);
};

#endif