#include "Client.hpp"

Client::Client(int clientFd): fd(clientFd), nickname(""), username(""), hostname(""),
				buffer(""), isRegistered(false), isOperator(false), isAuthenticated(false) {}

Client::~Client() {}

int Client::getFd() const { return fd; }
std::string Client::getNickname() const { return nickname; }
std::string Client::getUsername() const { return username; }
std::string Client::getHostname() const { return hostname; }
bool Client::getIsRegistered() const { return isRegistered; }
bool Client::getIsOperator() const { return isOperator; }
bool Client::getIsAuthenticated() const { return isAuthenticated; }

void Client::setNickname(const std::string& nick) { nickname = nick; }
void Client::setUsername(const std::string& user) { username = user; }
void Client::setHostname(const std::string& host) { hostname = host; }
void Client::setIsRegistered(bool reg) { isRegistered = reg; }
void Client::setIsOperator(bool op) { isOperator = op; }
void Client::setIsAuthenticated(bool auth) { isAuthenticated = auth; }

void Client::appendBuffer(const std::string& data)
{
	buffer += data;
}

bool Client::hasCompleteCommand() const
{
	return (buffer.find("\n") != std::string::npos);
}

std::string Client::extractCommand()
{
	size_t pos = buffer.find("\n");

	if (pos == std::string::npos)
		return "";

	std::string cmd = buffer.substr(0, pos);

	if (!cmd.empty() && cmd[cmd.length() - 1] == '\r')
		cmd.erase(cmd.length() - 1);

	buffer.erase(0, pos + 1);
	return cmd;
}
