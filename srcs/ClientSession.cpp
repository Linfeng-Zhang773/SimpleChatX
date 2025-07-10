#include "../includes/ClientSession.hpp"

ClientSession::ClientSession(int fd)
    : fd(fd), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}

ClientSession::ClientSession() : fd(-1), read_buffer(""), nickname(""), status(AuthStatus::NONE) {}