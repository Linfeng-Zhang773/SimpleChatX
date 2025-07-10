#ifndef CLIENTSESSION_HPP
#define CLIENTSESSION_HPP
#include <string>

enum class AuthStatus : int
{
    NONE,
    AUTHORIZED
};
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