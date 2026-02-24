#include "../includes/Utils.hpp"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        perror("fcntl F_SETFL");
}

bool safe_send(int fd, const char* data, std::size_t len)
{
    std::size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = ::send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("send");
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool safe_send(int fd, const std::string& msg)
{
    return safe_send(fd, msg.c_str(), msg.size());
}

std::string hash_password(const std::string& password)
{
    // NOTE: std::hash is NOT cryptographic. Use bcrypt/argon2 in production.
    std::size_t h = std::hash<std::string>{}(password);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << h;
    return oss.str();
}