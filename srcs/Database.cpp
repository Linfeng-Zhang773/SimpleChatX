#include "../includes/Database.hpp"
#include <ctime>
#include <iostream>

// Constructor: init db handle to null
Database::Database() : db(nullptr) {}

// Destructor: close DB if open
Database::~Database()
{
    close();
}

// Open SQLite DB and create table if needed
bool Database::open(const std::string& db_filename)
{
    std::string filename = db_filename;
    // check if open succeed, else will return error message
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    // return succeed feedback
    std::cout << "SQLite opened successfully!\n";
    return init();
}

// Close SQLite DB if open
void Database::close()
{
    // if db not nullptr, call sqlite_close() to close and reset db to nullptr
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

// Create "messages" table if it doesn't exist
bool Database::init()
{

    // SQL statement to create messages table with 6 fields:
    // id: auto-incremented primary key
    // sender: sender name
    // receiver: receiver name
    // content: chat message
    // type: "private" / "group"
    // timestamp: when message was sent (as text)
    const char* sql =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT,"
        "receiver TEXT,"
        "content TEXT,"
        "type TEXT,"
        "timestamp TEXT"
        ");";
    // Run SQL; on failure, print error message
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Failed to create table: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// Insert one message into DB with timestamp
bool Database::insertMessage(const std::string& sender,
                             const std::string& receiver,
                             const std::string& content,
                             const std::string& type)
{
    // If database is not opened
    if (!db) return false;

    // Prepare SQL insert statement with placeholders
    const char* sql = "INSERT INTO messages (sender, receiver, content, type, timestamp) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    // Compile SQL to a prepared statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    // Bind values to SQL placeholders (?):
    sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);

    // Get current time in "YYYY-MM-DD HH:MM:SS" format
    time_t now = time(nullptr);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%F %T", localtime(&now)); // Format current time
    sqlite3_bind_text(stmt, 5, time_str, -1, SQLITE_TRANSIENT);     // Bind timestamp
    // Execute SQL statement and check if it succeeds
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    // Free prepared statement resources
    sqlite3_finalize(stmt);
    return success;
}
// Return N most recent chat messages from DB
std::vector<ChatMessage> Database::getRecentMessages(int limit) const
{
    std::vector<ChatMessage> result;
    // If DB not open, return empty result
    if (!db) return result;
    // SQL to select recent messages (most recent first)
    const char* sql = "SELECT sender, receiver, content, type, timestamp FROM messages ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    // Prepare the SQL statement
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;
    // Bind the limit (number of messages to fetch)
    sqlite3_bind_int(stmt, 1, limit);
    // Fetch rows one by one and convert to ChatMessage objects
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
    // Clean up prepared statement
    sqlite3_finalize(stmt);
    return result;
}