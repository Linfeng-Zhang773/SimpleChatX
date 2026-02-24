# SimpleChatX Design Document

## 1. System Architecture

SimpleChatX follows the **Reactor Pattern** with a pool of worker threads to handle business logic. This decoupling ensures that the networking layer remains highly responsive even under heavy database load.

### 1.1 The Reactor (I/O Loop)
The main loop utilizes Linux `epoll` to monitor multiple file descriptors simultaneously.
- **Connection Handling**: When `listen_fd` is ready, the server performs a non-blocking `accept()` in a loop to handle thundering herd scenarios.
- **Event Distribution**: Upon receiving `EPOLLIN`, the raw data is read into a session buffer, and the processing task is dispatched to the `ThreadPool`.

### 1.2 Threading Model
- **Main Thread**: Responsible for `epoll_wait` and initial socket I/O.
- **Worker Threads**: Execute command parsing, password hashing (`std::hash`), and SQLite operations.
- **Concurrency Control**: 
  - `UserManager` uses `std::shared_mutex` to support multiple concurrent readers (history lookups) and exclusive writers (logins/registrations).
  - `Database` uses a standard `std::mutex` to protect the SQLite handle.

## 2. Key Implementation Details

### 2.1 Handling TCP Stream Fragmentation
Since TCP is a stream protocol, messages may arrive fragmented or joined (sticky packets). 
- **Solution**: Each `ClientSession` maintains a `read_buffer`. The server searches for `\n` as a message delimiter. If a complete line is found, it is extracted; otherwise, the partial data remains in the buffer until more data arrives.

### 2.2 Database Persistence
We use **SQLite3** for message and user storage.
- **Optimization**: The `journal_mode` is set to `WAL` (Write-Ahead Logging). This allows readers to access the database without being blocked by writers, significantly improving performance for `/history` queries during active chat sessions.
- **Security**: Passwords are never stored in plaintext. They are transformed using a hex-encoded hash before being persisted.

### 2.3 Message Visibility Logic
The `/history` command employs a visibility filter:
- **Broadcast**: Visible to everyone.
- **Private**: Visible only if `uid == sender` or `uid == receiver`.
- **Group**: Visible only if the user is a verified member of that specific group in the `UserManager`'s group map.

## 3. Configuration

Key constants are managed in `Config.hpp`:
- `THREAD_POOL_SIZE`: Defaulted to 4 (tunable for CPU cores).
- `MAX_EPOLL_EVENTS`: Defines the batch size for event processing.
- `DB_FILENAME`: Path to the SQLite storage file.

## 4. Future Roadmap
- **Heartbeat mechanism**: Close idle connections using a timer-wheel or priority queue.
- **Protobuf Integration**: Move from text-based parsing to structured binary protocols.
- **TLS/SSL**: Add an encryption layer using OpenSSL for secure communication.