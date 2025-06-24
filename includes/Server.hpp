#ifndef SERVER_HPP
#define SERVER_HPP
#pragma once

class Server
{
    private:        
    public:
        int listen_fd;
        int epoll_fd;
        Server();
        ~Server();
        void create_and_bind();
        void setup_epoll();
        void run_event_loop();
        void run_server();    
};
#endif