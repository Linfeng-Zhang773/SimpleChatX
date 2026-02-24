#include "../includes/ClientSession.hpp"

ClientSession::ClientSession(int fd)
    : fd(fd), status(AuthStatus::NONE) {}