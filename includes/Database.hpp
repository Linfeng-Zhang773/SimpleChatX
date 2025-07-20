#ifndef DATABASE_HPP
#define DATABASE_HPP
// neccessary libs
#include "Message.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>

// Handle SQLite database for storing chat messages
class Database
{
private:
    sqlite3* db; // SQLite DB handle

public:
    Database();                                // constructor(db==nullptr)
    ~Database();                               // destructor to close db
    bool open(const std::string& db_filename); // open db file
    void close();                              // close db if needed
    bool init();                               // create msg table for msg persistance if needed
    // insert one chat message to DB with sender, receiver, content, type info
    bool insertMessage(const std::string& sender,
                       const std::string& receiver,
                       const std::string& content,
                       const std::string& type);
    // Method to get recent messages(default setting as 50 msgs)
    std::vector<ChatMessage> getRecentMessages(int limit = 50) const;
};

#endif