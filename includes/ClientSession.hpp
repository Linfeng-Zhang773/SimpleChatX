#pragma once
#include <string>

/// Authentication state of a connected client.
enum class AuthStatus : int
{
    NONE,      ///< Not yet authenticated
    AUTHORIZED ///< Successfully registered or logged in
};

/**
 * @brief Holds runtime state for one TCP connection.
 */
class ClientSession
{
public:
    int fd;                  ///< Socket file descriptor
    AuthStatus status;       ///< Current auth state
    std::string nickname;    ///< Username (empty until authenticated)
    std::string read_buffer; ///< Accumulates partial TCP reads

    explicit ClientSession(int fd = -1);
};