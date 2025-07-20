#ifndef CLIENTSESSION_HPP
#define CLIENTSESSION_HPP
#include <string>

// status of client authentication
enum class AuthStatus : int
{
    NONE,      // Not authorized
    AUTHORIZED // authorized
};

// store session info for a connected client
class ClientSession
{
public:
    int fd;                  // socket file descriptor
    AuthStatus status;       // Auth status
    std::string nickname;    // Nickname after register or login
    std::string read_buffer; // buffer for incoming messages

    ClientSession(int fd); // constructor init with socket fd
    ClientSession();       // default destructor
};

#endif