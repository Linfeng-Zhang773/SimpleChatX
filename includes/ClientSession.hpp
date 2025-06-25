#ifndef CLIENTSESSION_HPP
#define CLIENTSESSION_HPP
#include <string>

class ClientSession
{
public:
    int fd;                    
    std::string read_buffer;   

    ClientSession(int fd);   
    ClientSession();  
};

#endif 