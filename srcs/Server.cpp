// introduces all needed libaraies
#include "../includes/Server.hpp"
#include "../includes/ClientSession.hpp"
#include "../includes/Utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

/**
 * @brief A constructor for server class, print Server set up if succeed
 */
Server::Server() : threadPool(10)
{
    cout << "Server set up" << endl;
}

/**
 * @brief A destructor for destroying server class, print Server shut down afterward
 */
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
 * @brief Starts the main event loop of the server using epoll.
 *
 * This loop continuously waits for I/O events on all registered file descriptors (sockets),
 * using `epoll_wait`. It efficiently handles:
 * - New incoming connections (on `listen_fd`)
 * - Incoming data from clients
 * - Client disconnections
 *
 * Delegates actual logic to helper member functions:
 * - `handle_new_connection()` for accept()
 * - `handle_client_input()` for recv()
 * - `handle_client_disconnection()` for closing sockets
 *
 * The loop runs indefinitely and forms the backbone of the server’s non-blocking I/O model.
 */
void Server::run_event_loop()
{
    const int MAX_EVENTS = 10;             // Max number of events to wait for per epoll_wait call
    struct epoll_event events[MAX_EVENTS]; // Event buffer

    cout << "Entering event loop..." << endl;
    while (1)
    {
        // Wait for I/O events (blocking indefinitely until at least one event occurs)
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            perror("epoll_wait failed!");
            continue; // On error, skip this iteration and continue listening
        }

        // Process each triggered event
        for (int i = 0; i < event_count; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // Case 1: New incoming connection on listening socket
            if (fd == listen_fd && (ev & EPOLLIN))
            {
                handle_new_connection();
            }
            // Case 2: Client disconnected (peer hang up or error)
            else if ((ev & EPOLLRDHUP) || (ev & EPOLLHUP))
            {
                handle_client_disconnection(fd);
            }
            // Case 3: Incoming data from an already connected client
            else if (ev & EPOLLIN)
            {
                threadPool.enqueue([this, fd]()
                                   { handle_client_input(fd); });
            }
        }
    }
}

/**
 * @brief Accepts new incoming client connections in a non-blocking loop.
 *
 * Uses accept() repeatedly to handle all pending connections until EAGAIN.
 * Each new client:
 * - Is set to non-blocking mode
 * - Added to epoll monitoring
 * - Stored in the clients map as a ClientSession
 * - Receives a welcome message
 */
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

        set_Nonblocking(connect_fd);       // Make the socket non-blocking
        userManager.addClient(connect_fd); // Add new client session

        // Register the client socket to epoll
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = connect_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &ev);

        // Send welcome message with available commands
        std::string welcome =
            "Welcome to ChatServer!\r\n"
            "Please choose an option:\r\n"
            "  /reg <username> <password>   to register a new user\r\n"
            "  /login <username> <password> to log in with an existing user\r\n"
            "  /quit                        to close the chat\r\n\r\n"
            "  /to <username> <message>     to send private message to target user\r\n"
            "  /create <groupname>          to create a chat group\r\n"
            "  /join <groupname>            to join a group chat\r\n"
            "  /group <groupname> <message> to send messages in a group\r\n";
        send(connect_fd, welcome.c_str(), welcome.size(), 0);

        cout << "[INFO] New client connected: fd = " << connect_fd << endl;
    }
}

/**
 * @brief Handles a client's disconnection.
 *
 * Cleans up the client's session:
 * - Removes it from epoll
 * - Closes the socket
 * - Erases the session from the clients map
 */
void Server::handle_client_disconnection(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    userManager.logoutUser(fd);
    userManager.removeClient(fd);
    cout << "[INFO] Client disconnected: fd = " << fd << endl;
}

/**
 * @brief Handles incoming data from a client socket.
 *
 * Performs:
 * - Reading and buffering partial input
 * - Parsing complete lines ending with '\n'
 * - Processing commands like /reg, /login, /quit
 * - Blocking unauthenticated users from sending chat messages
 * - Broadcasting valid chat messages
 *
 * Uses newline as message delimiter; handles \r cleanup for Telnet.
 */
void Server::handle_client_input(int fd)
{
    std::cout << "[Thread " << std::this_thread::get_id() << "] Handling input from fd: " << fd << std::endl;
    char buffer[1024];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    // Handle disconnection or error
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

    // Append new data to session buffer
    if (!userManager.hasClient(fd)) return;
    ClientSession& session = userManager.getClientSession(fd);

    session.read_buffer += std::string(buffer, n);

    size_t pos;
    // Process all complete lines in the buffer
    while ((pos = session.read_buffer.find('\n')) != std::string::npos)
    {
        std::string msg = session.read_buffer.substr(0, pos);
        session.read_buffer.erase(0, pos + 1);

        // Remove \r if present (Telnet sends \r\n)
        if (!msg.empty() && msg.back() == '\r')
        {
            msg.pop_back();
        }

        // Handle quit command
        if (msg == "/quit")
        {
            std::string reply = "Bye! Disconnecting...\r\n";
            send(fd, reply.c_str(), reply.size(), 0);
            handle_client_disconnection(fd);
            return;
        }

        // Authentication required before messaging
        if (!userManager.isLoggedIn(fd))
        {
            if (msg.compare(0, 5, "/reg ") == 0)
            {
                std::istringstream iss(msg.substr(5));
                std::string username, password;
                iss >> username >> password;

                if (username.empty() || password.empty())
                {
                    std::string reply = "Usage: /reg <username> <password>\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (username.length() < 2 || username.length() > 20)
                {
                    std::string reply = "Username must be 2~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (password.length() < 6 || password.length() > 20)
                {
                    std::string reply = "Password must be 6~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (userManager.registerUser(fd, username, password))
                {
                    session.nickname = username;
                    session.status = AuthStatus::AUTHORIZED;
                    std::string reply = "Registered successfully as [" + username + "]\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                else
                {
                    std::string reply = "Username already taken or invalid.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
            }
            else if (msg.compare(0, 7, "/login ") == 0)
            {
                std::istringstream iss(msg.substr(7));
                std::string username, password;
                iss >> username >> password;

                if (username.empty() || password.empty())
                {
                    std::string reply = "Usage: /login <username> <password>\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (username.length() < 2 || username.length() > 20)
                {
                    std::string reply = "Username must be 2~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (password.length() < 6 || password.length() > 20)
                {
                    std::string reply = "Password must be 6~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }
                if (userManager.loginUser(fd, username, password))
                {
                    session.nickname = username;
                    session.status = AuthStatus::AUTHORIZED;
                    std::string reply = "Logged in successfully as [" + username + "]\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                else
                {
                    std::string reply = "Login failed. Please check your username and password.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
            }

            else
            {
                const char* reply = "Please register (/reg <username> <password>) or login (/login <username> <password>) first.\r\n";
                send(fd, reply, strlen(reply), 0);
            }
            return;
        }
        else
        {
            // Block redundant auth commands
            if (msg.compare(0, 5, "/reg ") == 0 || msg.compare(0, 7, "/login ") == 0)
            {
                const char* reply = "You are already logged in. Cannot use /reg or /login again.\r\n";
                send(fd, reply, strlen(reply), 0);
                return;
            }

            // Handle private message
            else if (msg.compare(0, 4, "/to ") == 0)
            {
                std::istringstream iss(msg.substr(4));
                std::string target_username;
                iss >> target_username;

                std::string content;
                std::getline(iss, content);
                if (!content.empty() && content[0] == ' ') content.erase(0, 1);

                if (target_username.empty() || content.empty())
                {
                    const char* reply = "Usage: /to <username> <message>\r\n";
                    send(fd, reply, strlen(reply), 0);
                    return;
                }

                int target_fd = userManager.getFdByNickname(target_username);
                if (target_fd == -1 || !userManager.isLoggedIn(target_fd))
                {
                    std::string reply = "User [" + target_username + "] not found or not online.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                std::string private_msg = "[Private from " + session.nickname + "]: " + content + "\r\n";
                send(target_fd, private_msg.c_str(), private_msg.size(), 0);

                std::string confirm = "[To " + target_username + "]: " + content + "\r\n";
                send(fd, confirm.c_str(), confirm.size(), 0);
                return;
            }

            else if (msg.compare(0, 8, "/create ") == 0)
            {
                std::string groupname;
                std::istringstream iss(msg.substr(8));
                iss >> groupname;

                if (groupname.empty())
                {
                    const char* reply = "Usage: /create <groupname>\r\n";
                    send(fd, reply, strlen(reply), 0);
                    return;
                }

                if (userManager.createGroup(groupname))
                {
                    userManager.joinGroup(groupname, fd);
                    std::string reply = "Group [" + groupname + "] created and joined.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                else
                {
                    std::string reply = "Group [" + groupname + "] already exists.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                return;
            }
            else if (msg.compare(0, 6, "/join ") == 0)
            {
                std::string groupname;
                std::istringstream iss(msg.substr(6));
                iss >> groupname;

                if (groupname.empty())
                {
                    const char* reply = "Usage: /join <groupname>\r\n";
                    send(fd, reply, strlen(reply), 0);
                    return;
                }

                if (userManager.joinGroup(groupname, fd))
                {
                    std::string reply = "Joined group [" + groupname + "] successfully.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                else
                {
                    std::string reply = "Group [" + groupname + "] does not exist or already joined.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
                return;
            }
            else if (msg.compare(0, 7, "/group ") == 0)
            {
                std::istringstream iss(msg.substr(7));
                std::string groupname;
                iss >> groupname;

                std::string content;
                std::getline(iss, content);
                if (!content.empty() && content[0] == ' ') content.erase(0, 1);

                if (groupname.empty() || content.empty())
                {
                    const char* reply = "Usage: /group <groupname> <message>\r\n";
                    send(fd, reply, strlen(reply), 0);
                    return;
                }

                if (!userManager.isInGroup(groupname, fd))
                {
                    std::string reply = "You are not in group [" + groupname + "]. Use /join to join.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                std::string group_msg = "[Group " + groupname + "] [" + session.nickname + "]: " + content + "\r\n";
                std::unordered_set<int> members = userManager.getGroupMembers(groupname);
                for (int member_fd : members)
                {
                    if (member_fd != fd)
                    {
                        send(member_fd, group_msg.c_str(), group_msg.size(), 0);
                    }
                }

                // Echo back to sender
                send(fd, group_msg.c_str(), group_msg.size(), 0);
                return;
            }
            // Broadcast message to all users (including sender)
            std::string full_msg = "[" + session.nickname + "]: " + msg + "\r\n";
            std::cout << "[RECV] " << full_msg;
            broadcast_message(fd, full_msg);
        }
    }
}

/**
 * @brief Sends a message to all connected clients.
 *
 * Broadcasts messages to every client in `clients`, including the sender.
 * On send failure (e.g., broken pipe), the client is disconnected.
 *
 * @param from_fd The sender's file descriptor (used for logging or future exclusions).
 * @param msg     The formatted message to send.
 */
void Server::broadcast_message(int from_fd, const std::string& msg)
{
    for (auto& [fd, session] : userManager.getAllClients())
    {
        // Uncomment to exclude sender: if (fd == from_fd) continue;
        if (send(fd, msg.c_str(), msg.size(), 0) == -1)
        {
            perror("send failed");
            handle_client_disconnection(fd);
        }
    }
}

/**
 * @brief Initializes and starts the server.
 *
 * - Binds to the server port (create_and_bind)
 * - Sets up epoll (setup_epoll)
 * - Starts the main event loop (run_event_loop)
 */
void Server::run_server()
{
    cout << "Server activited" << endl;
    create_and_bind();
    setup_epoll();
    run_event_loop();
}
