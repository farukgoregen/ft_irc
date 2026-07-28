#include "Server.hpp"
#include "Commands.hpp"

bool Server::_signal = false;

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverFd(-1) {
    _cmdHandler = new Commands(this);
}

Server::~Server() {
    delete _cmdHandler;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
        delete it->second;

    for (size_t i = 0; i < _pollFds.size(); ++i) {
        if (_pollFds[i].fd >= 0)
            close(_pollFds[i].fd);
    }
    _pollFds.clear();
}

void Server::signalHandler(int signum) {
    (void)signum;
    std::cout << "\n[Signal Caught] Server shutting down..." << std::endl;
    Server::_signal = true;
}

void Server::setNonBlocking(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: Failed to set fcntl O_NONBLOCK! (fd: " << fd << ")" << std::endl;
    }
}

void Server::init() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) throw std::runtime_error("Failed to create socket!");

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt error!");

    setNonBlocking(_serverFd);

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Bind error! Port might be in use.");

    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("Listen error!");

    pollfd serverPollFd;
    serverPollFd.fd = _serverFd;
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;
    _pollFds.push_back(serverPollFd);

    std::cout << "IRC Server listening on port " << _port << "..." << std::endl;
}

void Server::run() {
    signal(SIGINT, Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);

    while (Server::_signal == false) {
        int pollCount = poll(&_pollFds[0], _pollFds.size(), -1);
        if (pollCount < 0 && Server::_signal == false) break;

        for (size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].revents & POLLIN) {
                if (_pollFds[i].fd == _serverFd) {
                    acceptNewClient();
                } else {
                    handleClientData(_pollFds[i].fd, i);
                }
            }
        }
    }
    std::cout << "Closing all sockets." << std::endl;
}

void Server::acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) return;

    setNonBlocking(clientFd);

    pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);

    _clients[clientFd] = new Client(clientFd);
    std::cout << "[Network] New client connected! Socket fd: " << clientFd << std::endl;
}

void Server::handleClientData(int clientFd, size_t pollIdx) {
    (void)pollIdx;
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        disconnectClient(clientFd, pollIdx);
        return;
    }

    _clients[clientFd]->appendBuffer(std::string(buffer, bytesRead));

    while (_clients[clientFd]->hasCompleteCommand()) {
        std::string cmd = _clients[clientFd]->extractCommand();
        std::cout << "[Command Executing - fd " << clientFd << "]: " << cmd << std::endl;
        
        // USER C ENTEGRASYONU:
        _cmdHandler->execute(clientFd, cmd, _clients, channels, _password);
    }
}

void Server::disconnectClient(int clientFd, size_t pollIdx) {
    std::cout << "[Network] Client disconnected. Socket fd: " << clientFd << std::endl;
    close(clientFd);
    _pollFds.erase(_pollFds.begin() + pollIdx);

    if (_clients.count(clientFd)) {
        Client* clientToDisconnect = _clients[clientFd];

        // >>> EKSİK OLAN HAYATİ KISIM: Kullanıcıyı odalardan temizle (Crash Önleyici) <<<
        for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
            if (it->second->isMember(clientToDisconnect)) {
                it->second->removeClient(clientToDisconnect);
            }
        }
        // >>> TEMİZLİK BİTTİ <<<

        delete clientToDisconnect; // Artık güvenle silebiliriz
        _clients.erase(clientFd);
    }
}

void Server::sendData(int clientFd, const std::string& message) {
    send(clientFd, message.c_str(), message.length(), 0);
}
