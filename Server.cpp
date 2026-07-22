#include "Server.hpp"

// Static sinyal değişkeni başlatma
bool Server::_signal = false;

Server::Server(int port, const std::string& password) 
    : _port(port), _password(password), _serverFd(-1) {}

Server::~Server() {
    // Nesne yok edilirken veya sunucu kapanırken açık soketleri kapat
    for (size_t i = 0; i < _pollFds.size(); ++i) {
        if (_pollFds[i].fd >= 0)
            close(_pollFds[i].fd);
    }
    _pollFds.clear();
}

void Server::signalHandler(int signum) {
    (void)signum;
    std::cout << "\n[Sinyal Yakalandı] Sunucu kapatılıyor..." << std::endl;
    Server::_signal = true;
}

void Server::setNonBlocking(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Hata: fcntl O_NONBLOCK ayarlanamadı! (fd: " << fd << ")" << std::endl;
    }
}

void Server::init() {
    // 1. Soket Oluşturma
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("Soket oluşturulamadı!");

    // 2. SO_REUSEADDR (Portun hızlıca tekrar kullanılabilmesi için)
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt hatası!");

    // 3. Non-blocking Moda Alma
    setNonBlocking(_serverFd);

    // 4. IP ve Port Yapılandırması
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    // 5. Bind
    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Bind hatası! Port kullanımda olabilir.");

    // 6. Listen
    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("Listen hatası!");

    // 7. Master Soketi Poll Listesine Ekle
    pollfd serverPollFd;
    serverPollFd.fd = _serverFd;
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;
    _pollFds.push_back(serverPollFd);

    std::cout << "IRC Sunucusu " << _port << " portunda dinlemede..." << std::endl;
}

void Server::run() {
    // Ctrl+C (SIGINT) ve Ctrl+\ (SIGQUIT) sinyallerini yakala
    signal(SIGINT, Server::signalHandler);
    signal(SIGQUIT, Server::signalHandler);

    while (Server::_signal == false) {
        // I/O Multiplexing - Tek poll() döngüsü
        int pollCount = poll(&_pollFds[0], _pollFds.size(), -1);
        if (pollCount < 0 && Server::_signal == false) {
            std::cerr << "Poll hatası!" << std::endl;
            break;
        }

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

    // Döngü bittiğinde (Ctrl+C basıldığında) temizlik
    std::cout << "Tüm soketler kapatılıyor." << std::endl;
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

    /* -------------------------------------------------------------
     *  USER B ENTEGRASYON NOKTASI:
     *  User B Client sınıfını tamamladığında burayı açacak:
     *  _clients[clientFd] = new Client(clientFd);
     * ------------------------------------------------------------- */

    std::cout << "[Network] Yeni istemci bağlandı! Soket fd: " << clientFd << std::endl;
}

void Server::handleClientData(int clientFd, size_t pollIdx) {
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // errno KONTROLÜ YAPILMIYOR (Subject kuralı)
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        disconnectClient(clientFd, pollIdx);
        return;
    }

    /* -------------------------------------------------------------
     *  USER B VE C ENTEGRASYON NOKTASI:
     *  1. Gelen parçalı veriyi istemcinin tamponuna ekleme:
     *     _clients[clientFd]->appendBuffer(std::string(buffer, bytesRead));
     *  
     *  2. '\r\n' veya '\n' ayıklama:
     *     while (_clients[clientFd]->hasCompleteCommand()) {
     *         std::string cmd = _clients[clientFd]->extractCommand();
     *         // USER C -> Command Handler çalıştırma:
     *         executeCommand(clientFd, cmd);
     *     }
     * ------------------------------------------------------------- */

    std::cout << "[Ham Veri - fd " << clientFd << "]: " << buffer;
}

void Server::disconnectClient(int clientFd, size_t pollIdx) {
    std::cout << "[Network] İstemci ayrıldı. Soket fd: " << clientFd << std::endl;
    close(clientFd);
    _pollFds.erase(_pollFds.begin() + pollIdx);

    /* -------------------------------------------------------------
     *  USER B ENTEGRASYON NOKTASI:
     *  if (_clients.count(clientFd)) {
     *      delete _clients[clientFd];
     *      _clients.erase(clientFd);
     *  }
     * ------------------------------------------------------------- */
}

void Server::sendData(int clientFd, const std::string& message) {
    send(clientFd, message.c_str(), message.length(), 0);
}