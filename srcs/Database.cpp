#include "../includes/Database.hpp"
#include <ctime>
#include <iostream>
Database::Database() : db(nullptr) {}
Database::~Database()
{
    close();
}
bool Database::open(const std::string& db_filename)
{
    std::string filename = db_filename;
    if(sqlite3_open(filename.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    std::cout << "SQLite opened successfully!\n";
    return init();
}
void Database::close()
{
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}
bool Database::init()
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT,"
        "receiver TEXT,"
        "content TEXT,"
        "type TEXT,"
        "timestamp TEXT"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Failed to create table: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}
bool Database::insertMessage(const std::string& sender,
                             const std::string& receiver,
                             const std::string& content,
                             const std::string& type)
{
    if (!db) return false;

    const char* sql = "INSERT INTO messages (sender, receiver, content, type, timestamp) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);

    // Get current time
    time_t now = time(nullptr);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%F %T", localtime(&now));
    sqlite3_bind_text(stmt, 5, time_str, -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}
std::vector<ChatMessage> Database::getRecentMessages(int limit) const
{
    std::vector<ChatMessage> result;
    if (!db) return result;

    const char* sql = "SELECT sender, receiver, content, type, timestamp FROM messages ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ChatMessage msg;
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        msg.receiver = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        result.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return result;
}