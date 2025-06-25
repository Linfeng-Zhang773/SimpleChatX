#ifndef SERVER_HPP
#define SERVER_HPP
#include <unordered_map>
#include "../includes/ClientSession.hpp"
class Server
{
    private:        
    public:
        int listen_fd;
        int epoll_fd;
        std::unordered_map<int, ClientSession> clients;
        Server();
        ~Server();
        void create_and_bind();
        void setup_epoll();
        void run_event_loop();
        void run_server();    
};
#endif