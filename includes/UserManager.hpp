#pragma once

#include "ClientSession.hpp"
#include "Database.hpp"

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class UserManager
{
public:
    /**
     * @brief Construct with a reference to the shared database.
     * @param db Database used to persist / look-up user credentials.
     */
    explicit UserManager(Database& db);
    ~UserManager() = default;

    // ── Auth ────────────────────────────────────────────────────────

    /// @brief Register a new user (persisted to DB) and auto-login.
    bool registerUser(int fd, const std::string& username, const std::string& password);

    /// @brief Log in with existing credentials.
    bool loginUser(int fd, const std::string& username, const std::string& password);

    /// @brief Check if fd is authenticated.
    bool isLoggedIn(int fd) const;

    /// @brief Return the nickname bound to @p fd (empty if none).
    std::string getNickname(int fd) const;

    /// @brief Log the user out (clear session state but keep the connection).
    void logoutUser(int fd);

    // ── Session tracking ────────────────────────────────────────────

    void addClient(int fd);
    void removeClient(int fd);
    bool hasClient(int fd) const;

    /// @brief Get a snapshot of all currently connected fds.
    std::vector<int> getAllFds() const;

    ClientSession getSession(int fd) const;
    int getFdByNickname(const std::string& nickname) const;

    // ── Groups ──────────────────────────────────────────────────────

    bool createGroup(const std::string& groupname);
    bool joinGroup(const std::string& groupname, int fd);
    bool isInGroup(const std::string& groupname, int fd) const;
    std::unordered_set<int> getGroupMembers(const std::string& groupname) const;

private:
    Database& db_; ///< Shared database reference

    mutable std::shared_mutex mtx_; ///< Read-write lock for all containers below

    std::unordered_map<int, ClientSession> clients_;                  ///< fd → session
    std::unordered_map<std::string, int> nickname_map_;               ///< online nickname → fd
    std::unordered_map<std::string, std::unordered_set<int>> groups_; ///< group → member fds
};