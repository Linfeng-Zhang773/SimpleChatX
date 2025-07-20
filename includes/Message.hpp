#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
struct ChatMessage
{
    std::string sender;
    std::string receiver;
    std::string content;
    std::string type;
    std::string timestamp;
};

#endif