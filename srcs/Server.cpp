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
    while (1)
    {
        // Wait for events indefinitely (-1 timeout)
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            perror("epoll_wait failed!");
            continue; // Continue loop even if epoll_wait fails
        }
        cout << "epoll_wait returned " << event_count << " event(s)" << endl;
        // Loop through triggered events
        for (int i = 0; i < event_count; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // New client trying to connect to the listening socket
            if (fd == listen_fd && (ev & EPOLLIN))
            {
                cout << "[EPOLL] Ready to accept new connection on listen_fd = " << listen_fd << endl;
                // Declare variables for accepting a new client connection
                int connect_fd;
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);
                // Accept an incoming client connection
                connect_fd = accept(listen_fd, (struct sockaddr*)&cliaddr, &len);
                if (connect_fd == -1)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        perror("accept failed");
                    continue;
                }
                // if accepted, create a pair as {connect_fd,Client} and stored into map
                clients[connect_fd] = ClientSession(connect_fd);
                // Set the accepted socket to non-blocking mode
                set_Nonblocking(connect_fd);
                // Prepare the epoll_event structure to monitor the new socket
                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLRDHUP; // Monitor for read events and disconnection
                client_event.data.fd = connect_fd;          // Store the socket fd in the event data
                // Add the new socket to the epoll instance
                int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &client_event);
                if (res == -1)
                {
                    perror("register events failed!");
                    exit(EXIT_FAILURE);
                }
                cout << "[INFO] Accepted new client: fd = " << connect_fd << endl;
            }
            else
            {
                // Handle client disconnection
                if ((ev & EPOLLRDHUP) || (ev & EPOLLHUP))
                {
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1)
                    {
                        perror("epoll_ctl EPOLL_CTL_DEL failed");
                    }

                    close(fd);

                    clients.erase(fd);
                    cout << "[INFO] Client disconnected fd = :" << fd << endl;
                }
                else
                {
                    // Handle readable client socket (EPOLLIN)
                    char buff[1024];
                    int n = recv(fd, buff, sizeof(buff), 0);

                    if (n == 0)
                    {
                        // Client closed the connection
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                        close(fd);
                        clients.erase(fd);
                        cout << "[INFO] Client closed connection normally, fd = " << fd << endl;
                        continue;
                    }
                    else if (n < 0)
                    {
                        // Error occurred during recv
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            continue;
                        else
                        {
                            perror("recv failed");
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                            clients.erase(fd);
                            cout << "[INFO] Client disconnected due to recv error, fd = " << fd << endl;
                            continue;
                        }
                    }
                    // Construct and broadcast the message to other clients
                    string message = "[fd: " + to_string(fd) + "]: " + string(buff, n);
                    cout << "[RECV]: " << message;

                    for (auto& [other_fd, session] : clients)
                    {
                        if (other_fd != fd)
                        {
                            send(other_fd, message.c_str(), message.size(), 0);
                        }
                    }
                }

                // To be extended later: handle events on client sockets
                cout << "[EPOLL] Event on unknown fd = " << fd << endl;
            }
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