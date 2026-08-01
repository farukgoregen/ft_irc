#include "Server.hpp"
#include "Commands.hpp"

bool Server::signal = false;

Server::Server(int port, const std::string& password) : port(port), password(password), serverFd(-1)
{
	cmdHandler = new Commands(this);
}

Server::~Server()
{
	delete cmdHandler;

	for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
		delete it->second;

	for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
		delete it->second;

	for (size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].fd >= 0)
			close(pollFds[i].fd);
	}

	pollFds.clear();
}

void Server::signalHandler(int signum)
{
	(void)signum;
	std::cout << "\n[Signal Caught] Server shutting down..." << std::endl;
	Server::signal = true;
}

void Server::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		std::cerr << "Error: Failed to set fcntl O_NONBLOCK! (fd: " << fd << ")" << std::endl;
}

void Server::init()
{
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0)
		throw std::runtime_error("Failed to create socket!");
	
	int opt = 1;

	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt error!");
		
	setNonBlocking(serverFd);
	
	struct sockaddr_in addr;

	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	
	if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("Bind error! Port might be in use.");
		
	if (listen(serverFd, SOMAXCONN) < 0)
		throw std::runtime_error("Listen error!");

	pollfd serverPollFd;

	serverPollFd.fd = serverFd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	pollFds.push_back(serverPollFd);
	
	std::cout << "IRC Server listening on port " << port << "..." << std::endl;
}

void Server::run() {
    ::signal(SIGINT, Server::signalHandler);
    ::signal(SIGQUIT, Server::signalHandler);

    while (Server::signal == false)
    {
        if (pollFds.empty()) break;

        int pollCount = poll(pollFds.data(), pollFds.size(), -1);
        if (pollCount < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        for (size_t i = 0; i < pollFds.size();)
        {
            bool clientDisconnected = false;
            if (pollFds[i].revents & POLLIN)
            {
                if (pollFds[i].fd == serverFd)
                    acceptNewClient();
                else
                    clientDisconnected = handleClientData(pollFds[i].fd, i);
            }
            else if (pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                disconnectClient(pollFds[i].fd, i);
                clientDisconnected = true;
            }
            if (!clientDisconnected)
                ++i;
        }
    }
    std::cout << "Closing all sockets." << std::endl;
}

void Server::acceptNewClient()
{
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);

	if (clientFd < 0)
		return;

	setNonBlocking(clientFd);
	
	pollfd clientPollFd;

	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	clientPollFd.revents = 0;
	pollFds.push_back(clientPollFd);
	
	clients[clientFd] = new Client(clientFd);
	std::cout << "[Network] New client connected! Socket fd: " << clientFd << std::endl;
}

bool Server::handleClientData(int clientFd, size_t pollIdx)
{
	char buffer[1024];
	std::memset(buffer, 0, sizeof(buffer));
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	

	if (bytesRead <= 0)
	{
		disconnectClient(clientFd, pollIdx);
		return true; 
	}
	
	clients[clientFd]->appendBuffer(std::string(buffer, bytesRead));
	
	while (clients[clientFd]->hasCompleteCommand())
	{
		std::string cmd = clients[clientFd]->extractCommand();

		std::cout << "[Command Executing - fd " << clientFd << "]: " << cmd << std::endl;
		cmdHandler->execute(clientFd, cmd, clients, channels, password);
	}
	return false;
}

void Server::disconnectClient(int clientFd, size_t pollIdx)
{
	std::cout << "[Network] Client disconnected. Socket fd: " << clientFd << std::endl;
	close(clientFd);
	pollFds.erase(pollFds.begin() + pollIdx);
	
	if (clients.count(clientFd))
	{
		Client* clientToDisconnect = clients[clientFd];

		for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
		{
			if (it->second->isMember(clientToDisconnect))
				it->second->removeClient(clientToDisconnect);
		}
		
		delete clientToDisconnect;
		clients.erase(clientFd);
	}
}

void Server::sendData(int clientFd, const std::string& message)
{
	send(clientFd, message.c_str(), message.length(), 0);
}
