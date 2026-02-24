#pragma once

#include <cstddef>

/**
 * @brief Configuration constants for the chat server application.
 *
 */
namespace Config
{
    constexpr int SERVER_PORT = 12345;
    constexpr int LISTEN_BACKLOG = 128;
    constexpr std::size_t THREAD_POOL_SIZE = 4;
    constexpr int MAX_EPOLL_EVENTS = 64;
    constexpr int RECV_BUFFER_SIZE = 4096;
    constexpr int DEFAULT_HISTORY = 50;
    constexpr int LOGIN_HISTORY = 10;
    constexpr int USERNAME_MIN_LEN = 2;
    constexpr int USERNAME_MAX_LEN = 20;
    constexpr int PASSWORD_MIN_LEN = 6;
    constexpr int PASSWORD_MAX_LEN = 20;
    constexpr const char* DB_FILENAME = "chat.db";
}