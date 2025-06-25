#include "../includes/ClientSession.hpp"

ClientSession::ClientSession(int fd)
    : fd(fd), read_buffer("") {}

ClientSession::ClientSession() : fd(-1), read_buffer("") {}