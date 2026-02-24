#pragma once

#include "Database.hpp"
#include "ThreadPool.hpp"
#include "UserManager.hpp"

#include <atomic>
#include <string>
#include <vector>

/**
 * @brief TCP chat server using Linux epoll and a thread pool.
 *
 * Lifecycle: construct → run_server() (blocks until SIGINT/SIGTERM).
 */
class Server
{
public:
    Server();
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// @brief One-call entry point: bind, epoll, event loop.
    void run_server();

    /// @brief Global flag set by the signal handler for graceful shutdown.
    static std::atomic<bool> quit;

private:
    Database db_;
    UserManager userManager_;
    ThreadPool threadPool_;

    int listen_fd_ = -1;
    int epoll_fd_ = -1;

    // ── Setup ───────────────────────────────────────────────────────

    void create_and_bind();
    void setup_epoll();

    // ── Event loop ──────────────────────────────────────────────────

    void run_event_loop();
    void handle_new_connection();
    void handle_client_disconnection(int fd);
    void handle_client_input(int fd);

    // ── Messaging helpers ───────────────────────────────────────────

    void broadcast_message(int from_fd, const std::string& msg);

    /**
     * @brief Format a filtered history block for the given user.
     * @param nickname Viewer's username (for visibility filtering).
     * @param fd       Viewer's fd (for group membership checks).
     * @param limit    Number of messages to fetch.
     * @return Ready-to-send string including the header.
     */
    std::string formatHistory(const std::string& nickname, int fd, int limit);
};
