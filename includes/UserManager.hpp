#ifndef USERMANAGER_HPP
#define USERMANAGER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ClientSession.hpp"
class UserManager
{
private:
    std::unordered_map<int, ClientSession> clients;                  // fd->session
    std::unordered_map<std::string, int> nickname_map;               // nickname->fd
    std::unordered_set<std::string> registered_users;                // registered usernames
    std::unordered_map<std::string, std::string> password_map;       // username->password
    std::unordered_map<std::string, std::unordered_set<int>> groups; // groupname -> fds
public:
    UserManager();
    ~UserManager() = default;
    bool isRegistered(const std::string& username) const;
    bool registerUser(int fd, const std::string& username, const std::string& password);
    bool loginUser(int fd, const std::string& username, const std::string& password);
    bool isLoggedIn(int fd) const;
    std::string getNickname(int fd) const;
    void logoutUser(int fd);

    void addClient(int fd);
    void removeClient(int fd);

    bool hasClient(int fd) const;
    ClientSession& getClientSession(int fd);
    std::unordered_map<int, ClientSession>& getAllClients();

    int getFdByNickname(const std::string& nickname) const;

    bool createGroup(const std::string& groupname);
    bool joinGroup(const std::string& groupname, int fd);
    bool isInGroup(const std::string& groupname, int fd) const;
    std::unordered_set<int> getGroupMembers(const std::string& groupname) const;
};

#endif