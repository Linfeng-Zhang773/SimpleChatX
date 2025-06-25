#include "../includes/Utils.hpp"
#include <fcntl.h>
#include <unistd.h>
void set_Nonblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}