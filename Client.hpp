#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int         fd;
    std::string nickname;
    std::string username;
    std::string hostname;
    std::string buffer;
    bool        isRegistered;
    bool        isOperator;
    bool        isAuthenticated; // YENİ EKLENDİ

public:
    Client(int clientFd);
    ~Client();

    // Getter'lar
    int getFd() const;
    std::string getNickname() const;
    std::string getUsername() const;
    std::string getHostname() const;
    bool getIsRegistered() const;
    bool getIsOperator() const;
    bool getIsAuthenticated() const; // YENİ EKLENDİ

    // Setter'lar
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    void setHostname(const std::string& host);
    void setIsRegistered(bool reg);
    void setIsOperator(bool op);
    void setIsAuthenticated(bool auth); // YENİ EKLENDİ

    // Buffer (Tampon) Yönetimi
    void appendBuffer(const std::string& data);
    bool hasCompleteCommand() const;
    std::string extractCommand();
};

#endif