// introduces all needed libaraies
#include "../includes/Server.hpp"
#include "../includes/ClientSession.hpp"
#include "../includes/Database.hpp"
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

// Constructor: create thread pool
Server::Server() : threadPool(10)
{
    cout << "Server set up" << endl;
}

// Destructor: cleanup log
Server::~Server()
{
    cout << "Server shut down" << endl;
}

// Create a listening TCP socket, bind to port 12345, set non-blocking
void Server::create_and_bind()
{
    struct sockaddr_in addr;

    // 1. Create a TCP socket (IPv4, stream)
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket failed!");
        exit(EXIT_FAILURE);
    }

    // 2. Set up address: IPv4, port 12345, bind to 0.0.0.0 (all interfaces)
    addr.sin_family = AF_INET;                // Use IPv4
    addr.sin_port = htons(12345);             // Convert port to network byte order
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections on any local IP

    // 3. Bind socket to the address
    int result = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (result == -1)
    {
        perror("bind failed!");
        exit(EXIT_FAILURE);
    }

    // 4. Mark socket as passive (ready to accept connections)
    int listen_res = listen(listen_fd, 10); // backlog = 10
    if (listen_res == -1)
    {
        perror("listen failed!");
        exit(EXIT_FAILURE);
    }

    // 5. Set the listening socket to non-blocking mode
    set_Nonblocking(listen_fd);
}

// Set up epoll instance and register listening socket
void Server::setup_epoll()
{
    struct epoll_event event;

    // 1. Create an epoll instance
    epoll_fd = epoll_create1(0); // Use modern epoll API
    if (epoll_fd == -1)
    {
        perror("create epoll failed!");
        exit(EXIT_FAILURE);
    }

    // 2. Prepare event for the listening socket
    event.events = EPOLLIN;    // Watch for read (accept) events
    event.data.fd = listen_fd; // Associate the event with listen_fd

    // 3. Add the listening socket to the epoll interest list
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    if (res == -1)
    {
        perror("register events failed!");
        exit(EXIT_FAILURE);
    }

    std::cout << "Epoll setup complete. Waiting for events..." << std::endl;
}

// Main epoll event loop: wait for I/O and dispatch events
void Server::run_event_loop()
{
    const int MAX_EVENTS = 10;             // Max events per epoll_wait call
    struct epoll_event events[MAX_EVENTS]; // Buffer to store triggered events

    std::cout << "Entering event loop..." << std::endl;

    while (1)
    {
        // Wait for I/O events (blocks until at least one event is ready)
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1)
        {
            perror("epoll_wait failed!");
            continue; // Skip this round if epoll_wait failed
        }

        // Handle each ready event
        for (int i = 0; i < event_count; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            // Case 1: New client connection
            if (fd == listen_fd && (ev & EPOLLIN))
            {
                handle_new_connection(); // Accept and register client
            }
            // Case 2: Client disconnected (or error)
            else if ((ev & EPOLLRDHUP) || (ev & EPOLLHUP))
            {
                handle_client_disconnection(fd); // Cleanup session and epoll
            }
            // Case 3: Client sent data
            else if (ev & EPOLLIN)
            {
                // Offload to thread pool for async handling
                threadPool.enqueue([this, fd]()
                                   { handle_client_input(fd); });
            }
        }
    }
}

// Accept and register all incoming client connections
void Server::handle_new_connection()
{
    while (1)
    {
        sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        // Accept a new client (non-blocking)
        int connect_fd = accept(listen_fd, (sockaddr*)&cliaddr, &len);

        if (connect_fd == -1)
        {
            // No more clients to accept (EAGAIN/EWOULDBLOCK = expected in non-blocking mode)
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("accept failed");
            return;
        }

        // 1. Set the new client socket to non-blocking
        set_Nonblocking(connect_fd);

        // 2. Add to UserManager (creates a new ClientSession)
        userManager.addClient(connect_fd);

        // 3. Register client fd to epoll
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP; // Read and disconnect events
        ev.data.fd = connect_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &ev);

        // 4. Send welcome message to the new client
        std::string welcome =
            "Welcome to ChatServer!\r\n"
            "Please choose an option:\r\n"
            "  /reg <username> <password>   to register a new user\r\n"
            "  /login <username> <password> to log in with an existing user\r\n"
            "  /quit                        to close the chat\r\n\r\n"
            "  /to <username> <message>     to send private message to target user\r\n"
            "  /create <groupname>          to create a chat group\r\n"
            "  /join <groupname>            to join a group chat\r\n"
            "  /group <groupname> <message> to send messages in a group\r\n"
            "  /history                     to check recent 50 history messages\r\n";

        send(connect_fd, welcome.c_str(), welcome.size(), 0);

        std::cout << "[INFO] New client connected: fd = " << connect_fd << std::endl;
    }
}

// Clean up when a client disconnects
// Removes client from epoll, closes socket, and clears user session
void Server::handle_client_disconnection(int fd)
{
    // 1. Remove fd from epoll interest list
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

    // 2. Close the client socket
    close(fd);

    // 3. Log out user (clear nickname, status)
    userManager.logoutUser(fd);

    // 4. Remove client session from UserManager
    userManager.removeClient(fd);

    std::cout << "[INFO] Client disconnected: fd = " << fd << std::endl;
}

// Receive and process input from a client.
// Supports command parsing, authentication, and various messaging types.
// Automatically handles login/registration if needed.
// Commands: /reg, /login, /to, /create, /join, /group, /history, /quit
void Server::handle_client_input(int fd)
{
    // Print which thread is handling this request (for debugging)
    std::cout << "[Thread " << std::this_thread::get_id() << "] Handling input from fd: " << fd << std::endl;

    // Prepare buffer for receiving client data
    char buffer[1024];

    // Try to receive data from the client socket
    // Returns:
    //   >0: number of bytes received
    //   =0: client has closed connection (graceful disconnect)
    //   <0: error occurred (EAGAIN is OK in non-blocking mode)
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

    // Handle client disconnection (recv returns 0 = peer closed socket)
    if (n == 0)
    {
        handle_client_disconnection(fd);
        return;
    }

    // Handle receive error
    else if (n < 0)
    {
        // Non-blocking mode: no data available now (not an error)
        if (errno == EAGAIN || errno == EWOULDBLOCK) return;

        // Other error occurred
        perror("recv failed");
        handle_client_disconnection(fd);
        return;
    }

    // Append received data into the client’s read buffer
    if (!userManager.hasClient(fd)) return;
    ClientSession& session = userManager.getClientSession(fd);
    session.read_buffer += std::string(buffer, n); // Accumulate partial message

    size_t pos; // Will be used to find message delimiters ('\n')

    // Process all complete lines in the buffer
    while ((pos = session.read_buffer.find('\n')) != std::string::npos)
    {
        // get lines and assign to msg
        std::string msg = session.read_buffer.substr(0, pos);
        // erase read buffer
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
            // if command is register
            if (msg.compare(0, 5, "/reg ") == 0)
            {
                // Extract username and password from the message
                std::istringstream iss(msg.substr(5)); // Skip "/reg "
                std::string username, password;
                iss >> username >> password;

                // Check if input is missing
                if (username.empty() || password.empty())
                {
                    std::string reply = "Usage: /reg <username> <password>\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Username length check
                if (username.length() < 2 || username.length() > 20)
                {
                    std::string reply = "Username must be 2~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Password length check
                if (password.length() < 6 || password.length() > 20)
                {
                    std::string reply = "Password must be 6~20 characters.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Try to register the user
                if (userManager.registerUser(fd, username, password))
                {
                    // Update session info after successful registration
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
            // Handle "/login <username> <password>" command
            else if (msg.compare(0, 7, "/login ") == 0)
            {
                // Parse username and password
                std::istringstream iss(msg.substr(7));
                std::string username, password;
                iss >> username >> password;

                // Check for missing input
                if (username.empty() || password.empty())
                {
                    std::string reply = "Usage: /login <username> <password>\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Validate username and password length
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

                // Try to login
                if (userManager.loginUser(fd, username, password))
                {
                    // Set session info after successful login
                    session.nickname = username;
                    session.status = AuthStatus::AUTHORIZED;

                    std::string reply = "Logged in successfully as [" + username + "]\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);

                    // Show 10 recent messages
                    db.open("chat.db");
                    auto messages = db.getRecentMessages(10);

                    std::ostringstream oss;
                    oss << "=== Recent Messages ===\r\n";

                    // Filter and show only visible messages to this user
                    for (const auto& m : messages)
                    {
                        bool visible = false;
                        if (m.type == "broadcast")
                            visible = true;
                        else if (m.type == "private")
                            visible = (m.sender == session.nickname || m.receiver == session.nickname);
                        else if (m.type == "group")
                            visible = userManager.isInGroup(m.receiver, fd);

                        if (visible)
                        {
                            std::string line;
                            if (m.type == "broadcast")
                                line = m.content;
                            else if (m.type == "private")
                                line = "[Private] " + m.sender + " -> " + m.receiver + ": " + m.content;
                            else if (m.type == "group")
                                line = "[Group " + m.receiver + "] " + m.sender + ": " + m.content;

                            oss << line << "\r\n";
                        }
                    }

                    std::string history_output = oss.str();
                    if (history_output == "=== Recent Messages ===\r\n")
                        history_output += "(no visible message)\r\n";

                    send(fd, history_output.c_str(), history_output.size(), 0);
                }
                else
                {
                    std::string reply = "Login failed. Please check your username and password.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                }
            }
            // user not authorized, reminding feedback
            else
            {
                const char* reply = "Please register (/reg <username> <password>) or login (/login <username> <password>) first.\r\n";
                send(fd, reply, strlen(reply), 0);
            }
            return;
        }
        // user is authorized and able to send other commands
        else
        {
            // Handle "/history" command - show recent 50 messages
            if (msg == "/history")
            {
                std::cout << "[DEBUG] /history triggered by " << session.nickname << "\n";

                // Open database and fetch recent messages
                db.open("chat.db");
                auto messages = db.getRecentMessages(50);

                std::ostringstream oss;
                oss << "=== Recent Messages ===\r\n";

                // Go through each message and check if current user can see it
                for (const auto& m : messages)
                {
                    bool visible = false;

                    if (m.type == "broadcast")
                    {
                        visible = true;
                    }
                    else if (m.type == "private")
                    {
                        if (m.sender == session.nickname || m.receiver == session.nickname)
                            visible = true;
                    }
                    else if (m.type == "group")
                    {
                        if (userManager.isInGroup(m.receiver, fd))
                            visible = true;
                    }

                    // If message is visible to the user, format and add to output
                    if (visible)
                    {
                        std::string line;
                        if (m.type == "broadcast")
                            line = m.content;
                        else if (m.type == "private")
                            line = "[Private] " + m.sender + " -> " + m.receiver + ": " + m.content;
                        else if (m.type == "group")
                            line = "[Group " + m.receiver + "] " + m.sender + ": " + m.content;

                        oss << line << "\r\n";
                    }
                }

                // If no visible message, show fallback
                std::string output = oss.str();
                if (output == "=== Recent Messages ===\r\n")
                    output += "(no visible message)\r\n";

                // Send result to client
                send(fd, output.c_str(), output.size(), 0);
                return;
            }

            // Block redundant auth commands
            if (msg.compare(0, 5, "/reg ") == 0 || msg.compare(0, 7, "/login ") == 0)
            {
                const char* reply = "You are already logged in. Cannot use /reg or /login again.\r\n";
                send(fd, reply, strlen(reply), 0);
                return;
            }

            // Handle private message: /to <username> <message>
            else if (msg.compare(0, 4, "/to ") == 0)
            {
                std::istringstream iss(msg.substr(4));
                std::string target_username;
                iss >> target_username;

                std::string content;
                std::getline(iss, content);
                if (!content.empty() && content[0] == ' ') content.erase(0, 1);

                // Check arguments
                if (target_username.empty() || content.empty())
                {
                    const char* reply = "Usage: /to <username> <message>\r\n";
                    send(fd, reply, strlen(reply), 0);
                    return;
                }

                // Find target user
                int target_fd = userManager.getFdByNickname(target_username);
                if (target_fd == -1 || !userManager.isLoggedIn(target_fd))
                {
                    std::string reply = "User [" + target_username + "] not found or not online.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Send message to target and confirm to sender
                std::string private_msg = "[Private from " + session.nickname + "]: " + content + "\r\n";
                send(target_fd, private_msg.c_str(), private_msg.size(), 0);

                std::string confirm = "[To " + target_username + "]: " + content + "\r\n";
                send(fd, confirm.c_str(), confirm.size(), 0);

                // Save to database
                db.open("chat.db");
                db.insertMessage(session.nickname, target_username, content, "private");
                return;
            }

            // Handle group creation: /create <groupname>
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

                // Create and join group
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

            // Handle joining a group: /join <groupname>
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

                // Join existing group
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

            // Handle group message: /group <groupname> <message>
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

                // Check if user is in the group
                if (!userManager.isInGroup(groupname, fd))
                {
                    std::string reply = "You are not in group [" + groupname + "]. Use /join to join.\r\n";
                    send(fd, reply.c_str(), reply.size(), 0);
                    return;
                }

                // Broadcast message to all group members except sender
                std::string group_msg = "[Group " + groupname + "] [" + session.nickname + "]: " + content + "\r\n";
                std::unordered_set<int> members = userManager.getGroupMembers(groupname);
                for (int member_fd : members)
                {
                    if (member_fd != fd)
                    {
                        send(member_fd, group_msg.c_str(), group_msg.size(), 0);
                    }
                }

                // Save to database and echo back to sender
                db.open("chat.db");
                db.insertMessage(session.nickname, groupname, content, "group");
                send(fd, group_msg.c_str(), group_msg.size(), 0);
                return;
            }

            // Broadcast message to all users (including sender echo)
            std::string full_msg = "[" + session.nickname + "]: " + msg + "\r\n";
            std::cout << "[RECV] " << full_msg;
            broadcast_message(fd, full_msg);
        }
    }
}

// Send a message to all connected clients and store it in DB
// from_fd Sender's socket fd
// msg The message to broadcast (already formatted)
void Server::broadcast_message(int from_fd, const std::string& msg)
{
    // 1. Open DB and log the broadcast message
    db.open("chat.db");
    ClientSession& session = userManager.getClientSession(from_fd);
    db.insertMessage(session.nickname, "ALL", msg, "broadcast");

    // 2. Send to all connected clients
    for (auto& [fd, session] : userManager.getAllClients())
    {
        // Uncomment below line to exclude sender from receiving their own message
        // if (fd == from_fd) continue;

        if (send(fd, msg.c_str(), msg.size(), 0) == -1)
        {
            perror("send failed");
            handle_client_disconnection(fd); // Remove if send failed
        }
    }
}

// Start the server: bind socket, setup epoll, enter main loop
void Server::run_server()
{
    std::cout << "Server activated" << std::endl;

    create_and_bind(); // Step 1: create socket and bind to port
    setup_epoll();     // Step 2: setup epoll and add listen_fd
    run_event_loop();  // Step 3: start main epoll loop
}
