#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Message.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>

class Database
{
private:
    sqlite3* db;

public:
    Database();
    ~Database();
    bool open(const std::string& db_filename);
    void close();
    bool init();
    bool insertMessage(const std::string& sender,
                       const std::string& receiver,
                       const std::string& content,
                       const std::string& type);
    std::vector<ChatMessage> getRecentMessages(int limit = 50) const;
};

#endif