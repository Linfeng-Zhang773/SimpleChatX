#include "../includes/ClientSession.hpp"

// constructor with socket fd
ClientSession::ClientSession(int fd)
    : fd(fd), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}

// default constructor(set fd as -1 which is invalid, others be NULL)
ClientSession::ClientSession() : fd(-1), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}