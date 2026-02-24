#include "../includes/UserManager.hpp"
#include "../includes/Utils.hpp"

UserManager::UserManager(Database& db) : db_(db) {}

// ── Auth ────────────────────────────────────────────────────────────

bool UserManager::registerUser(int fd, const std::string& username,
                               const std::string& password)
{
    std::string pw_hash = hash_password(password);

    // Persist to DB first (DB has its own mutex)
    if (!db_.insertUser(username, pw_hash))
        return false; // username already taken

    std::unique_lock lock(mtx_);
    ClientSession& s = clients_[fd];
    s.nickname = username;
    s.status = AuthStatus::AUTHORIZED;
    nickname_map_[username] = fd;
    return true;
}

bool UserManager::loginUser(int fd, const std::string& username,
                            const std::string& password)
{
    // Verify credentials against DB
    std::string stored_hash;
    if (!db_.getUserPasswordHash(username, stored_hash))
        return false; // user doesn't exist

    if (stored_hash != hash_password(password))
        return false; // wrong password

    std::unique_lock lock(mtx_);
    if (nickname_map_.count(username))
        return false; // already logged in elsewhere

    ClientSession& s = clients_[fd];
    s.nickname = username;
    s.status = AuthStatus::AUTHORIZED;
    nickname_map_[username] = fd;
    return true;
}

bool UserManager::isLoggedIn(int fd) const
{
    std::shared_lock lock(mtx_);
    auto it = clients_.find(fd);
    return it != clients_.end() && it->second.status == AuthStatus::AUTHORIZED;
}

std::string UserManager::getNickname(int fd) const
{
    std::shared_lock lock(mtx_);
    auto it = clients_.find(fd);
    return (it != clients_.end()) ? it->second.nickname : "";
}

void UserManager::logoutUser(int fd)
{
    std::unique_lock lock(mtx_);
    auto it = clients_.find(fd);
    if (it == clients_.end()) return;

    nickname_map_.erase(it->second.nickname);
    it->second.status = AuthStatus::NONE;
    it->second.nickname.clear();
}

// ── Session tracking ────────────────────────────────────────────────

void UserManager::addClient(int fd)
{
    std::unique_lock lock(mtx_);
    clients_[fd] = ClientSession(fd);
}

void UserManager::removeClient(int fd)
{
    std::unique_lock lock(mtx_);
    auto it = clients_.find(fd);
    if (it == clients_.end()) return;

    // Remove nickname mapping BEFORE erasing the session
    nickname_map_.erase(it->second.nickname);
    clients_.erase(it);
}

bool UserManager::hasClient(int fd) const
{
    std::shared_lock lock(mtx_);
    return clients_.count(fd) > 0;
}

std::vector<int> UserManager::getAllFds() const
{
    std::shared_lock lock(mtx_);
    std::vector<int> fds;
    fds.reserve(clients_.size());
    for (const auto& [fd, _] : clients_)
        fds.push_back(fd);
    return fds;
}

ClientSession UserManager::getSession(int fd) const
{
    std::shared_lock lock(mtx_);
    return clients_.at(fd); // returns a copy — safe across threads
}

int UserManager::getFdByNickname(const std::string& nickname) const
{
    std::shared_lock lock(mtx_);
    auto it = nickname_map_.find(nickname);
    return (it != nickname_map_.end()) ? it->second : -1;
}

// ── Groups ──────────────────────────────────────────────────────────

bool UserManager::createGroup(const std::string& groupname)
{
    std::unique_lock lock(mtx_);
    if (groups_.count(groupname)) return false;
    groups_[groupname] = {};
    return true;
}

bool UserManager::joinGroup(const std::string& groupname, int fd)
{
    std::unique_lock lock(mtx_);
    auto it = groups_.find(groupname);
    if (it == groups_.end()) return false;
    return it->second.insert(fd).second; // false if already a member
}

bool UserManager::isInGroup(const std::string& groupname, int fd) const
{
    std::shared_lock lock(mtx_);
    auto it = groups_.find(groupname);
    return it != groups_.end() && it->second.count(fd);
}

std::unordered_set<int> UserManager::getGroupMembers(const std::string& groupname) const
{
    std::shared_lock lock(mtx_);
    auto it = groups_.find(groupname);
    return (it != groups_.end()) ? it->second : std::unordered_set<int>{};
}