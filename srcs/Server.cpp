#include "../includes/Server.hpp"
#include "../includes/ClientSession.hpp"
#include "../includes/Utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

Server::Server()
{
    cout << "Server set up" << endl;
}

Server::~Server()
{
    cout << "Server shut down" << endl;
}

/**
 * @brief Create a TCP socket, bind it to a local address, listen for incoming connections,
 *        and set the socket to non-blocking mode.
 *
 * This function initializes the server's listening socket:
 * - Creates a TCP socket
 * - Binds to port 12345 on all available interfaces (INADDR_ANY)
 * - Starts listening with a backlog of 10
 * - Sets the socket to non-blocking using fcntl
 *
 * On failure at any step, it logs the error and terminates the program.
 */
void Server::create_and_bind()
{
    struct sockaddr_in addr;

    // 1. Create a TCP socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket failed!");
        exit(EXIT_FAILURE);
    }

    // 2. Set up local address (IPv4, port 12345, any IP)
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);             // Host to network byte order
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all available interfaces

    // 3. Bind socket to the address
    int result = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (result == -1)
    {
        perror("bind failed!");
        exit(EXIT_FAILURE);
    }

    // 4. Start listening on the socket
    int listen_res = listen(listen_fd, 10);
    if (listen_res == -1)
    {
        perror("listen failed!");
        exit(EXIT_FAILURE);
    }

    // 5. Set socket to non-blocking mode
    set_Nonblocking(listen_fd);
}

/**
 * @brief Initialize epoll and register the listening socket for read events.
 *
 * Creates an epoll instance, and adds the listening socket (listen_fd)
 * to the epoll interest list for monitoring incoming connections (EPOLLIN).
 *
 * On failure, logs the error and exits the program.
 */
void Server::setup_epoll()
{
    struct epoll_event event;

    // 1. Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("create epoll failed!");
        exit(EXIT_FAILURE);
    }

    // 2. Set up event for the listening socket
    event.events = EPOLLIN;    // Interested in read events
    event.data.fd = listen_fd; // Monitor the listening socket

    // 3. Register the listening socket with epoll
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    if (res == -1)
    {
        perror("register events failed!");
        exit(EXIT_FAILURE);
    }
    cout << "Epoll setup complete. Waiting for events..." << endl;
}

/**
 * @brief Start the main event loop using epoll.
 *
 * Continuously waits for I/O events on registered file descriptors using epoll_wait.
 * Currently only the listening socket is registered. When epoll indicates it is ready
 * for reading (EPOLLIN), a new client is trying to connect.
 *
 * In the current version (Day 2), this loop only detects readiness and logs the events.
 * Connection acceptance and client handling will be added in Day 3.
 */
void Server::run_event_loop()
{
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    cout << "Entering event loop..." << endl;
    while (true)
    {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            perror("epoll_wait failed!");
            continue;
        }

        for (int i = 0; i < event_count; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listen_fd && (ev & EPOLLIN))
            {
                handle_new_connection();
            }
            else if ((ev & EPOLLRDHUP) || (ev & EPOLLHUP))
            {
                handle_client_disconnection(fd);
            }
            else if (ev & EPOLLIN)
            {
                handle_client_input(fd);
            }
        }
    }
}
void Server::handle_new_connection()
{
    while (1)
    {
        sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connect_fd = accept(listen_fd, (sockaddr*)&cliaddr, &len);
        if (connect_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept failed");
            return;
        }

        set_Nonblocking(connect_fd);
        clients[connect_fd] = ClientSession(connect_fd);

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = connect_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &ev);

        const char* welcome =
            "Welcome to ChatServer!\n"
            "Please choose an option:\n"
            "  /reg <username>    to register a new user\n"
            "  /login <username>  to log in with an existing user\n\n";
        send(connect_fd, welcome, strlen(welcome), 0);

        cout << "[INFO] New client connected: fd = " << connect_fd << endl;
    }
}

void Server::handle_client_disconnection(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clients.erase(fd);
    cout << "[INFO] Client disconnected: fd = " << fd << endl;
}

void Server::handle_client_input(int fd)
{
    char buffer[1024];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    if (n == 0)
    {
        handle_client_disconnection(fd);
        return;
    }
    else if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
        perror("recv failed");
        handle_client_disconnection(fd);
        return;
    }

    ClientSession& session = clients[fd];
    session.read_buffer += std::string(buffer, n);

    size_t pos;
    while ((pos = session.read_buffer.find('\n')) != std::string::npos)
    {
        std::string msg = session.read_buffer.substr(0, pos + 1);
        session.read_buffer.erase(0, pos + 1);
        std::string full_msg = "[fd: " + std::to_string(fd) + "] " + msg;
        cout << "[RECV] " << full_msg;
        broadcast_message(fd, full_msg);
    }
}
void Server::broadcast_message(int from_fd, const std::string& msg)
{
    for (auto& [fd, session] : clients)
    {
        if (fd == from_fd) continue;
        if (send(fd, msg.c_str(), msg.size(), 0) == -1)
        {
            perror("send failed");
            handle_client_disconnection(fd);
        }
    }
}

void Server::run_server()
{
    cout << "Server activited" << endl;
    create_and_bind();
    setup_epoll();
    run_event_loop();
}