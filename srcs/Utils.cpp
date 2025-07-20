#include "../includes/Utils.hpp"
#include <fcntl.h>
#include <unistd.h>
/// Set file descriptor to non-blocking mode
void set_Nonblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);      // Get current flags
    fcntl(fd, F_SETFL, flag | O_NONBLOCK); // Add O_NONBLOCK flag
}