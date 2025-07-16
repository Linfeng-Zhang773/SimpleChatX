#include "../includes/ClientSession.hpp"

//constructor for client class
ClientSession::ClientSession(int fd)
    : fd(fd), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}

//default constructor
ClientSession::ClientSession() : fd(-1), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}