#ifndef SERVER_HPP
#define SERVER_HPP
#include "../includes/ClientSession.hpp"
#include "ThreadPool.hpp"
#include "UserManager.hpp"
/**
 * @class Server
 * @brief A server class for handling server logics
 */
class Server
{
private:
    UserManager userManager;
    ThreadPool threadPool;

public:
    int listen_fd;
    int epoll_fd;
    Server();
    ~Server();
    void create_and_bind();
    void setup_epoll();
    void run_event_loop();
    void run_server();
    void handle_new_connection();
    void handle_client_disconnection(int fd);
    void handle_client_input(int fd);
    void broadcast_message(int from_fd, const std::string& msg);
};
#endif