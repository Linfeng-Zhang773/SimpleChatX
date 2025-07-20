#ifndef SERVER_HPP
#define SERVER_HPP

#include "../includes/ClientSession.hpp"
#include "Database.hpp"
#include "ThreadPool.hpp"
#include "UserManager.hpp"

/// Main server class: handles epoll, connections, and logic
class Server
{
private:
    UserManager userManager; // Manage users and sessions
    ThreadPool threadPool;   // Thread pool for async tasks

public:
    int listen_fd; // Listening socket fd
    int epoll_fd;  // epoll instance fd
    Database db;   // SQLite database for chat history

    Server();  // Constructor: init members
    ~Server(); // Destructor

    // Setup server
    void create_and_bind(); // Create listening socket and bind port
    void setup_epoll();     // Setup epoll and add listen_fd
    void run_server();      // Entry point: setup + start loop

    // Event loop
    void run_event_loop(); // Main epoll event loop

    // Connection handling
    void handle_new_connection();             // Accept and add new client
    void handle_client_disconnection(int fd); // Cleanup disconnected client

    // Data processing
    void handle_client_input(int fd);                            // Read & process client input
    void broadcast_message(int from_fd, const std::string& msg); // Send to all clients
};

#endif
