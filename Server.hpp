#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include "Client.hpp"
#include "Channel.hpp"

// User B'nin yazacağı sınıflar için Forward Declaration

class Server {
private:
    int                     _port;
    std::string             _password;
    int                     _serverFd;
    std::vector<pollfd>     _pollFds;
    std::map<int, Client*>          _clients;
    std::map<std::string, Channel*> channels;


    // Sinyal yönetimi (Ctrl+C için)
    static bool             _signal;

    // İç Network Metotları
    void setNonBlocking(int fd);
    void acceptNewClient();
    void handleClientData(int clientFd, size_t pollIdx);
    void disconnectClient(int clientFd, size_t pollIdx);

public:
    Server(int port, const std::string& password);
    ~Server();

    static void signalHandler(int signum);

    void init(); // Soket oluşturma, bind, listen
    void run();  // Ana poll() döngüsü
    
    // I/O Yardımcı Fonksiyonları
    void sendData(int clientFd, const std::string& message);
};

#endif