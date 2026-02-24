#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Set a file descriptor to non-blocking mode (O_NONBLOCK).
 * @param fd File descriptor to modify.
 */
void set_nonblocking(int fd);

/**
 * @brief Write all bytes to a socket, handling partial writes.
 * @param fd   Target socket.
 * @param data Pointer to data buffer.
 * @param len  Number of bytes to send.
 * @return true if all bytes were sent successfully.
 */
bool safe_send(int fd, const char* data, std::size_t len);

/// @overload Convenience wrapper for std::string.
bool safe_send(int fd, const std::string& msg);

/**
 * @brief Produce a simple hex-encoded hash of a password string.
 *
 * @note This uses std::hash — NOT cryptographically secure.
 *       Production systems should use bcrypt / argon2.
 * @param password  Raw password.
 * @return Hex string of the hash.
 */
std::string hash_password(const std::string& password);
