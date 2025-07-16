#ifndef CLIENTSESSION_HPP
#define CLIENTSESSION_HPP
#include <string>

/**
 * @enum AuthStatus
 * @brief A enum class represents user's current status, 
 * NONE for user hasn't registerd or logged in
 * AUTHORIZED for user has registed or logged in
 */
enum class AuthStatus : int
{
    NONE,
    AUTHORIZED
};
/**
 * @class ClientSession
 * @brief Represents a single client session connected to the chat server.
 *
 * This class manages per-client data including:
 * - The client's socket file descriptor.
 * - The current authentication status (NONE or AUTHORIZED).
 * - The nickname (set after registration or login).
 * - A read buffer to accumulate incoming message data until newline-delimited.
 */
class ClientSession
{
public:
    int fd;
    AuthStatus status;
    std::string nickname;
    std::string read_buffer;

    ClientSession(int fd);
    ClientSession();
};

#endif