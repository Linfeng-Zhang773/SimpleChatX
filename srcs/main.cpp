#include "../includes/Server.hpp"
#include <csignal>
#include <iostream>

/// @brief Signal handler for graceful shutdown.
static void signal_handler(int sig)
{
    std::cout << "\n[Signal] Caught signal " << sig << ", shutting down...\n";
    Server::quit.store(true, std::memory_order_relaxed);
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN); // ignore broken pipe from disconnected clients

    Server server;
    server.run_server();

    return 0;
}