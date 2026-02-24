#pragma once

#include <string>

/**
 * @brief Represents one persisted chat message (DB row).
 */
struct ChatMessage
{
    std::string sender;
    std::string receiver;
    std::string content;
    std::string type;      ///< "private", "group", or "broadcast"
    std::string timestamp; ///< "YYYY-MM-DD HH:MM:SS"
};