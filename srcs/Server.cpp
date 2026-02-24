#include "../includes/Server.hpp"
#include "../includes/Config.hpp"
#include "../includes/Utils.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

// ── Static members ──────────────────────────────────────────────────

std::atomic<bool> Server::quit{false};

// ── Lifecycle ───────────────────────────────────────────────────────

Server::Server()
    : userManager_(db_),
      threadPool_(Config::THREAD_POOL_SIZE)
{
    std::cout << "[Server] Initialised\n";
}

Server::~Server()
{
    if (epoll_fd_ >= 0) ::close(epoll_fd_);
    if (listen_fd_ >= 0) ::close(listen_fd_);
    std::cout << "[Server] Shut down\n";
}

void Server::run_server()
{
    // Open database once at startup
    if (!db_.open(Config::DB_FILENAME))
    {
        std::cerr << "[Server] Failed to open database, aborting.\n";
        return;
    }

    create_and_bind();
    setup_epoll();
    run_event_loop();

    db_.close();
}

// ── Socket setup ────────────────────────────────────────────────────

void Server::create_and_bind()
{
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow immediate port reuse after restart
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Config::SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd_, Config::LISTEN_BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(listen_fd_);
    std::cout << "[Server] Listening on port " << Config::SERVER_PORT << "\n";
}

void Server::setup_epoll()
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

// ── Event loop ──────────────────────────────────────────────────────

void Server::run_event_loop()
{
    epoll_event events[Config::MAX_EPOLL_EVENTS];
    std::cout << "[Server] Entering event loop...\n";

    while (!quit.load(std::memory_order_relaxed))
    {
        int n = epoll_wait(epoll_fd_, events, Config::MAX_EPOLL_EVENTS, 1000);
        if (n == -1)
        {
            if (errno == EINTR) continue; // interrupted by signal
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listen_fd_)
            {
                handle_new_connection();
            }
            else if (ev & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                handle_client_disconnection(fd);
            }
            else if (ev & EPOLLIN)
            {
                threadPool_.enqueue([this, fd]
                                    { handle_client_input(fd); });
            }
        }
    }

    std::cout << "[Server] Event loop exited.\n";
}

// ── Connection management ───────────────────────────────────────────

void Server::handle_new_connection()
{
    while (true)
    {
        sockaddr_in cliaddr{};
        socklen_t len = sizeof(cliaddr);
        int cfd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&cliaddr), &len);

        if (cfd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            return;
        }

        set_nonblocking(cfd);
        userManager_.addClient(cfd);

        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = cfd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, cfd, &ev);

        static const std::string kWelcome =
            "Welcome to SimpleChatX!\r\n"
            "Commands:\r\n"
            "  /reg   <user> <pass>          Register\r\n"
            "  /login <user> <pass>          Login\r\n"
            "  /to    <user> <msg>           Private message\r\n"
            "  /create <group>               Create group\r\n"
            "  /join   <group>               Join group\r\n"
            "  /group  <group> <msg>         Group message\r\n"
            "  /history                      Recent messages\r\n"
            "  /quit                         Disconnect\r\n";

        safe_send(cfd, kWelcome);
        std::cout << "[Server] Client connected: fd=" << cfd << "\n";
    }
}

void Server::handle_client_disconnection(int fd)
{
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    ::close(fd);
    userManager_.logoutUser(fd);
    userManager_.removeClient(fd);
    std::cout << "[Server] Client disconnected: fd=" << fd << "\n";
}

// ── History helper ──────────────────────────────────────────────────

std::string Server::formatHistory(const std::string& nickname, int fd, int limit)
{
    auto messages = db_.getRecentMessages(limit);

    std::ostringstream oss;
    oss << "=== Recent Messages ===\r\n";

    for (const auto& m : messages)
    {
        bool visible = false;

        if (m.type == "broadcast")
            visible = true;
        else if (m.type == "private")
            visible = (m.sender == nickname || m.receiver == nickname);
        else if (m.type == "group")
            visible = userManager_.isInGroup(m.receiver, fd);

        if (!visible) continue;

        if (m.type == "broadcast")
            oss << m.content << "\r\n";
        else if (m.type == "private")
            oss << "[Private] " << m.sender << " -> " << m.receiver << ": " << m.content << "\r\n";
        else if (m.type == "group")
            oss << "[Group " << m.receiver << "] " << m.sender << ": " << m.content << "\r\n";
    }

    std::string out = oss.str();
    if (out == "=== Recent Messages ===\r\n")
        out += "(no visible messages)\r\n";

    return out;
}

// ── Input handling / command dispatch ────────────────────────────────

void Server::handle_client_input(int fd)
{
    char buffer[Config::RECV_BUFFER_SIZE];

    // Read until EAGAIN (drain all available data)
    while (true)
    {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n == 0)
        {
            handle_client_disconnection(fd);
            return;
        }
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("recv");
            handle_client_disconnection(fd);
            return;
        }

        if (!userManager_.hasClient(fd)) return;

        // We need to work with the session buffer, but since getSession returns
        // a copy for thread safety, we handle buffering at this level.
        // Append received bytes — we'll process complete lines below.
        // NOTE: For simplicity we store the read_buffer in the session via a
        //       scoped helper that reads + writes back under the lock.
        //       A production system would use per-fd buffers outside the shared map.
        ClientSession session = userManager_.getSession(fd);
        session.read_buffer += std::string(buffer, static_cast<std::size_t>(n));

        std::size_t pos;
        while ((pos = session.read_buffer.find('\n')) != std::string::npos)
        {
            std::string msg = session.read_buffer.substr(0, pos);
            session.read_buffer.erase(0, pos + 1);

            if (!msg.empty() && msg.back() == '\r') msg.pop_back();
            if (msg.empty()) continue;

            // ── /quit ───────────────────────────────────────────
            if (msg == "/quit")
            {
                safe_send(fd, "Bye!\r\n");
                handle_client_disconnection(fd);
                return;
            }

            // ── Not logged in: only /reg and /login allowed ─────
            if (!userManager_.isLoggedIn(fd))
            {
                if (msg.compare(0, 5, "/reg ") == 0)
                {
                    std::istringstream iss(msg.substr(5));
                    std::string user, pass;
                    iss >> user >> pass;

                    if (user.empty() || pass.empty())
                    {
                        safe_send(fd, "Usage: /reg <username> <password>\r\n");
                        break;
                    }

                    if (user.length() < Config::USERNAME_MIN_LEN ||
                        user.length() > Config::USERNAME_MAX_LEN)
                    {
                        safe_send(fd, "Username must be 2-20 characters.\r\n");
                        break;
                    }

                    if (pass.length() < Config::PASSWORD_MIN_LEN ||
                        pass.length() > Config::PASSWORD_MAX_LEN)
                    {
                        safe_send(fd, "Password must be 6-20 characters.\r\n");
                        break;
                    }

                    if (userManager_.registerUser(fd, user, pass))
                        safe_send(fd, "Registered as [" + user + "]\r\n");
                    else
                        safe_send(fd, "Username already taken.\r\n");
                }
                else if (msg.compare(0, 7, "/login ") == 0)
                {
                    std::istringstream iss(msg.substr(7));
                    std::string user, pass;
                    iss >> user >> pass;

                    if (user.empty() || pass.empty())
                    {
                        safe_send(fd, "Usage: /login <username> <password>\r\n");
                        break;
                    }

                    if (user.length() < Config::USERNAME_MIN_LEN ||
                        user.length() > Config::USERNAME_MAX_LEN)
                    {
                        safe_send(fd, "Username must be 2-20 characters.\r\n");
                        break;
                    }

                    if (pass.length() < Config::PASSWORD_MIN_LEN ||
                        pass.length() > Config::PASSWORD_MAX_LEN)
                    {
                        safe_send(fd, "Password must be 6-20 characters.\r\n");
                        break;
                    }

                    if (userManager_.loginUser(fd, user, pass))
                    {
                        safe_send(fd, "Logged in as [" + user + "]\r\n");
                        safe_send(fd, formatHistory(user, fd, Config::LOGIN_HISTORY));
                    }
                    else
                    {
                        safe_send(fd, "Login failed. Check username/password.\r\n");
                    }
                }
                else
                {
                    safe_send(fd, "Please /reg or /login first.\r\n");
                }
                break; // process one command per recv-cycle for unauthenticated clients
            }

            // ── Authenticated commands ──────────────────────────

            std::string nickname = userManager_.getNickname(fd);

            // /history
            if (msg == "/history")
            {
                safe_send(fd, formatHistory(nickname, fd, Config::DEFAULT_HISTORY));
            }
            // block duplicate auth
            else if (msg.compare(0, 5, "/reg ") == 0 || msg.compare(0, 7, "/login ") == 0)
            {
                safe_send(fd, "Already logged in.\r\n");
            }
            // /to <user> <msg>
            else if (msg.compare(0, 4, "/to ") == 0)
            {
                std::istringstream iss(msg.substr(4));
                std::string target;
                iss >> target;
                std::string content;
                std::getline(iss, content);
                if (!content.empty() && content[0] == ' ') content.erase(0, 1);

                if (target.empty() || content.empty())
                {
                    safe_send(fd, "Usage: /to <username> <message>\r\n");
                }
                else
                {
                    int tfd = userManager_.getFdByNickname(target);
                    if (tfd == -1 || !userManager_.isLoggedIn(tfd))
                    {
                        safe_send(fd, "User [" + target + "] not online.\r\n");
                    }
                    else
                    {
                        safe_send(tfd, "[Private from " + nickname + "]: " + content + "\r\n");
                        safe_send(fd, "[To " + target + "]: " + content + "\r\n");
                        db_.insertMessage(nickname, target, content, "private");
                    }
                }
            }
            // /create <group>
            else if (msg.compare(0, 8, "/create ") == 0)
            {
                std::string gname;
                std::istringstream(msg.substr(8)) >> gname;

                if (gname.empty())
                    safe_send(fd, "Usage: /create <groupname>\r\n");
                else if (userManager_.createGroup(gname))
                {
                    userManager_.joinGroup(gname, fd);
                    safe_send(fd, "Group [" + gname + "] created & joined.\r\n");
                }
                else
                    safe_send(fd, "Group [" + gname + "] already exists.\r\n");
            }
            // /join <group>
            else if (msg.compare(0, 6, "/join ") == 0)
            {
                std::string gname;
                std::istringstream(msg.substr(6)) >> gname;

                if (gname.empty())
                    safe_send(fd, "Usage: /join <groupname>\r\n");
                else if (userManager_.joinGroup(gname, fd))
                    safe_send(fd, "Joined [" + gname + "].\r\n");
                else
                    safe_send(fd, "Group [" + gname + "] not found or already joined.\r\n");
            }
            // /group <group> <msg>
            else if (msg.compare(0, 7, "/group ") == 0)
            {
                std::istringstream iss(msg.substr(7));
                std::string gname;
                iss >> gname;
                std::string content;
                std::getline(iss, content);
                if (!content.empty() && content[0] == ' ') content.erase(0, 1);

                if (gname.empty() || content.empty())
                {
                    safe_send(fd, "Usage: /group <groupname> <message>\r\n");
                }
                else if (!userManager_.isInGroup(gname, fd))
                {
                    safe_send(fd, "Not in group [" + gname + "]. Use /join first.\r\n");
                }
                else
                {
                    std::string gmsg = "[Group " + gname + "] [" + nickname + "]: " + content + "\r\n";
                    auto members = userManager_.getGroupMembers(gname);
                    for (int mfd : members)
                        if (mfd != fd) safe_send(mfd, gmsg);

                    safe_send(fd, gmsg);
                    db_.insertMessage(nickname, gname, content, "group");
                }
            }
            // default: broadcast
            else
            {
                std::string full = "[" + nickname + "]: " + msg + "\r\n";
                broadcast_message(fd, full);
            }
        }

        // If we broke out of the line-processing loop with remaining buffer,
        // we'd normally write it back. For simplicity, any incomplete line is
        // discarded here. A full implementation would write session.read_buffer
        // back via a setter.
        // TODO: persist leftover read_buffer back to session for partial-line support.
        break; // one recv batch per call is sufficient with LT epoll
    }
}

// ── Broadcast ───────────────────────────────────────────────────────

void Server::broadcast_message(int from_fd, const std::string& msg)
{
    std::string nickname = userManager_.getNickname(from_fd);
    db_.insertMessage(nickname, "ALL", msg, "broadcast");

    // Take a snapshot of fds to avoid iterator invalidation
    std::vector<int> fds = userManager_.getAllFds();
    std::vector<int> failed;

    for (int fd : fds)
    {
        if (!safe_send(fd, msg))
            failed.push_back(fd);
    }

    for (int fd : failed)
        handle_client_disconnection(fd);
}