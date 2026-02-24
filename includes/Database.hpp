#pragma once
#include "Message.hpp"

#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>

/**
 * @brief Manages all SQLite operations with internal mutex protection.
 *
 * The database is opened once at startup via open() and closed on destruction.
 * All public methods are thread-safe.
 */
class Database
{
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * @brief Open the database file and initialise tables.
     * @param db_filename Path to the SQLite file.
     * @return true on success.
     */
    bool open(const std::string& db_filename);

    /// @brief Close the database handle (idempotent).
    void close();

    // ── Message operations ──────────────────────────────────────────

    /**
     * @brief Insert a chat message.
     * @return true on success.
     */
    bool insertMessage(const std::string& sender,
                       const std::string& receiver,
                       const std::string& content,
                       const std::string& type);

    /**
     * @brief Fetch the N most recent messages.
     * @param limit Max number of rows (default 50).
     */
    std::vector<ChatMessage> getRecentMessages(int limit = 50) const;

    // ── User operations ─────────────────────────────────────────────

    /**
     * @brief Persist a new user (username + hashed password).
     * @return true on success, false if username already exists.
     */
    bool insertUser(const std::string& username, const std::string& password_hash);

    /**
     * @brief Look up the stored password hash for a username.
     * @param[out] out_hash Receives the hash if found.
     * @return true if user exists.
     */
    bool getUserPasswordHash(const std::string& username, std::string& out_hash) const;

    /// @brief Check whether a username exists in the DB.
    bool userExists(const std::string& username) const;

private:
    sqlite3* db;            ///< SQLite handle (nullptr when closed)
    mutable std::mutex mtx; ///< Protects all database operations

    /// @brief Create tables if they don't exist.
    bool initTables();
};