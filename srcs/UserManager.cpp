#include "../includes/UserManager.hpp"

// default constructor
UserManager::UserManager() {}

// a boolean method to check if user is already logged in
bool UserManager::isRegistered(const std::string& username) const
{
    // if username exists in registered set, return true,else false
    return registered_users.count(username) > 0;
}

// Register a new user with username and password
bool UserManager::registerUser(int fd, const std::string& username, const std::string& password)
{
    // username has been taken, return false
    if (isRegistered(username)) return false;

    // username and password insert to set
    registered_users.insert(username);
    password_map[username] = password;
    // create session for user and authorized it
    ClientSession session(fd);
    session.nickname = username;
    session.status = AuthStatus::AUTHORIZED;

    // insert into map
    clients[fd] = session;
    nickname_map[username] = fd;
    return true;
}

// Login with username and password, bind to fd
bool UserManager::loginUser(int fd, const std::string& username, const std::string& password)
{
    // Debug msgs
    // std::cout << "[LOGIN DEBUG] username = " << username << ", password = " << password << "\n";
    // std::cout << "[LOGIN DEBUG] isRegistered = " << isRegistered(username) << "\n";
    // std::cout << "[LOGIN DEBUG] nickname_map.count = " << nickname_map.count(username) << "\n";
    // std::cout << "[LOGIN DEBUG] stored password = " << password_map[username] << "\n";
    if (!isRegistered(username)) return false;            // registered
    if (nickname_map.count(username)) return false;       // Already logged in
    if (password_map[username] != password) return false; // password not matching
    ClientSession session(fd);
    session.nickname = username;
    session.status = AuthStatus::AUTHORIZED;

    clients[fd] = session;
    nickname_map[username] = fd;
    return true;
}

// a method to check if user status
bool UserManager::isLoggedIn(int fd) const
{
    auto it = clients.find(fd);
    return it != clients.end() && it->second.status == AuthStatus::AUTHORIZED;
}

// to search and get username from socket fd
std::string UserManager::getNickname(int fd) const
{
    auto it = clients.find(fd);
    if (it != clients.end())
    {
        return it->second.nickname;
    }
    return "";
}

// to log out user
void UserManager::logoutUser(int fd)
{
    auto it = clients.find(fd);
    if (it != clients.end())
    {
        // erase username in login map
        std::string name = it->second.nickname;
        nickname_map.erase(name);
        // set status back to NONE and clear its username
        it->second.status = AuthStatus::NONE;
        it->second.nickname.clear();
    }
}
// add a client seesion to map
void UserManager::addClient(int fd)
{
    clients[fd] = ClientSession(fd);
}

// remove a client session from map
void UserManager::removeClient(int fd)
{
    clients.erase(fd);
    nickname_map.erase(getNickname(fd));
}

// check if client session exists
bool UserManager::hasClient(int fd) const
{
    return clients.find(fd) != clients.end();
}

// get client session from socket fd
ClientSession& UserManager::getClientSession(int fd)
{
    return clients.at(fd);
}
// get all client sessions
std::unordered_map<int, ClientSession>& UserManager::getAllClients()
{
    return clients;
}

// get socket fd from nickname using map
int UserManager::getFdByNickname(const std::string& nickname) const
{
    auto it = nickname_map.find(nickname);
    if (it != nickname_map.end())
    {
        return it->second;
    }
    return -1;
}

// Create a new group
bool UserManager::createGroup(const std::string& groupname)
{
    // if group exists, return false
    if (groups.count(groupname)) return false;
    groups[groupname] = std::unordered_set<int>();
    return true;
}

// to join a group
bool UserManager::joinGroup(const std::string& groupname, int fd)
{
    auto it = groups.find(groupname);
    if (it == groups.end()) return false;

    if (it->second.count(fd)) return false;

    it->second.insert(fd);
    return true;
}

// check if this user is in a group
bool UserManager::isInGroup(const std::string& groupname, int fd) const
{
    auto it = groups.find(groupname);
    if (it == groups.end()) return false;
    return it->second.count(fd) > 0;
}
// to get all group memebers in a group
std::unordered_set<int> UserManager::getGroupMembers(const std::string& groupname) const
{
    auto it = groups.find(groupname);
    if (it != groups.end()) return it->second;
    return {};
}
