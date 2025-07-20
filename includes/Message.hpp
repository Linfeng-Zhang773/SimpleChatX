#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

/// Represents a single chat message (used for DB and display)
struct ChatMessage
{
    std::string sender;    // Username of the sender
    std::string receiver;  // Username of the receiver (or group name)
    std::string content;   // The text content of the message
    std::string type;      // Message type: "private", "group", etc.
    std::string timestamp; // When the message was sent (as string)
};

#endif
