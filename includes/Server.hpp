#ifndef SERVER_HPP
#define SERVER_HPP
#include "../includes/ClientSession.hpp"
#include <unordered_map>
class Server
{
private:
public:
    int listen_fd;
    int epoll_fd;
    std::unordered_map<int, ClientSession> clients;     // fd->session
    std::unordered_map<std::string, int> nickname_map;  // nickname->fd
    std::unordered_map<std::string> register_usernames; // registered usernames
    Server();
    ~Server();
    void create_and_bind();
    void setup_epoll();
    void run_event_loop();
    void run_server();
    void handle_new_connection();
    void handle_client_disconnection(int fd);
    void handle_client_input(int fd);
    void broadcast_message(int from_fd, const std::string& msg)
};
#endif