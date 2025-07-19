#include "../includes/UserManager.hpp"

UserManager::UserManager() {}

bool UserManager::isRegistered(const std::string& username) const
{
    return registered_users.count(username) > 0;
}

bool UserManager::registerUser(int fd, const std::string& username, const std::string& password)
{
    if (isRegistered(username)) return false;

    registered_users.insert(username);
    password_map[username] = password;
    ClientSession session(fd);
    session.nickname = username;
    session.status = AuthStatus::AUTHORIZED;

    clients[fd] = session;
    nickname_map[username] = fd;
    return true;
}

bool UserManager::loginUser(int fd, const std::string& username, const std::string& password)
{
    if (!isRegistered(username)) return false;
    if (nickname_map.count(username)) return false;
    if (password_map[username] != password) return false;
    ClientSession session(fd);
    session.nickname = username;
    session.status = AuthStatus::AUTHORIZED;

    clients[fd] = session;
    nickname_map[username] = fd;
    return true;
}

bool UserManager::isLoggedIn(int fd) const
{
    auto it = clients.find(fd);
    return it != clients.end() && it->second.status == AuthStatus::AUTHORIZED;
}

std::string UserManager::getNickname(int fd) const
{
    auto it = clients.find(fd);
    if (it != clients.end())
    {
        return it->second.nickname;
    }
    return "";
}

void UserManager::logoutUser(int fd)
{
    auto it = clients.find(fd);
    if (it != clients.end())
    {
        std::string name = it->second.nickname;
        nickname_map.erase(name);
        clients.erase(it);
    }
}
void UserManager::addClient(int fd)
{
    clients[fd] = ClientSession(fd);
}

void UserManager::removeClient(int fd)
{
    clients.erase(fd);
    nickname_map.erase(getNickname(fd));
}

bool UserManager::hasClient(int fd) const
{
    return clients.find(fd) != clients.end();
}

ClientSession& UserManager::getClientSession(int fd)
{
    return clients.at(fd);
}

std::unordered_map<int, ClientSession>& UserManager::getAllClients()
{
    return clients;
}

int UserManager::getFdByNickname(const std::string& nickname) const
{
    auto it = nickname_map.find(nickname);
    if (it != nickname_map.end())
    {
        return it->second;
    }
    return -1;
}

bool UserManager::createGroup(const std::string& groupname)
{
    if (groups.count(groupname)) return false;
    groups[groupname] = std::unordered_set<int>();
    return true;
}

bool UserManager::joinGroup(const std::string& groupname, int fd)
{
    auto it = groups.find(groupname);
    if (it == groups.end()) return false;

    if (it->second.count(fd)) return false;

    it->second.insert(fd);
    return true;
}

bool UserManager::isInGroup(const std::string& groupname, int fd) const
{
    auto it = groups.find(groupname);
    if (it == groups.end()) return false;
    return it->second.count(fd) > 0;
}

std::unordered_set<int> UserManager::getGroupMembers(const std::string& groupname) const
{
    auto it = groups.find(groupname);
    if (it != groups.end()) return it->second;
    return {};
}
