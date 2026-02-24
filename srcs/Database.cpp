#include "../includes/Database.hpp"

#include <ctime>
#include <iostream>

Database::Database() : db(nullptr) {}

Database::~Database() { close(); }

bool Database::open(const std::string& db_filename)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (db) return true; // already open

    if (sqlite3_open(db_filename.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "[DB] Failed to open: " << sqlite3_errmsg(db) << "\n";
        db = nullptr;
        return false;
    }

    // Enable WAL mode for better concurrent read performance
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

    std::cout << "[DB] Opened " << db_filename << "\n";
    return initTables();
}

void Database::close()
{
    std::lock_guard<std::mutex> lock(mtx);
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::initTables()
{
    const char* sql_messages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender   TEXT NOT NULL,"
        "  receiver TEXT NOT NULL,"
        "  content  TEXT NOT NULL,"
        "  type     TEXT NOT NULL,"
        "  timestamp TEXT NOT NULL"
        ");";

    const char* sql_users =
        "CREATE TABLE IF NOT EXISTS users ("
        "  username      TEXT PRIMARY KEY,"
        "  password_hash TEXT NOT NULL"
        ");";

    char* err = nullptr;
    if (sqlite3_exec(db, sql_messages, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::cerr << "[DB] Create messages table: " << err << "\n";
        sqlite3_free(err);
        return false;
    }
    if (sqlite3_exec(db, sql_users, nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::cerr << "[DB] Create users table: " << err << "\n";
        sqlite3_free(err);
        return false;
    }
    return true;
}

// ── Messages ────────────────────────────────────────────────────────

bool Database::insertMessage(const std::string& sender,
                             const std::string& receiver,
                             const std::string& content,
                             const std::string& type)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (!db) return false;

    const char* sql =
        "INSERT INTO messages (sender, receiver, content, type, timestamp) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);

    char ts[20];
    std::time_t now = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%F %T", std::localtime(&now));
    sqlite3_bind_text(stmt, 5, ts, -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<ChatMessage> Database::getRecentMessages(int limit) const
{
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<ChatMessage> result;
    if (!db) return result;

    const char* sql =
        "SELECT sender, receiver, content, type, timestamp "
        "FROM messages ORDER BY id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ChatMessage m;
        m.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        m.receiver = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        m.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        m.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        m.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        result.push_back(std::move(m));
    }

    sqlite3_finalize(stmt);
    return result;
}

// ── Users ───────────────────────────────────────────────────────────

bool Database::insertUser(const std::string& username,
                          const std::string& password_hash)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (!db) return false;

    const char* sql = "INSERT OR IGNORE INTO users (username, password_hash) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::getUserPasswordHash(const std::string& username,
                                   std::string& out_hash) const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (!db) return false;

    const char* sql = "SELECT password_hash FROM users WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool Database::userExists(const std::string& username) const
{
    std::string dummy;
    return getUserPasswordHash(username, dummy);
}