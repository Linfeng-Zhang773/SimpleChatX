# SimpleChatX Design Document

## 1. System Architecture

SimpleChatX follows a **Single Reactor + Thread Pool** pattern. The main thread runs a single `epoll` event loop (the Reactor) that accepts connections and reads raw data, then dispatches business logic to a pool of worker threads. This decoupling ensures the networking layer remains responsive even under heavy database or computation load.

### 1.1 The Reactor (I/O Loop)

The main loop uses Linux `epoll` (Level-Triggered mode) to monitor multiple file descriptors simultaneously.

- **Connection Handling**: When `listen_fd` becomes readable, the server performs a non-blocking `accept()` in a loop to drain all pending connections in a single epoll notification.
- **Event Distribution**: Upon receiving `EPOLLIN` on a client fd, raw data is read into a per-session buffer, and the command-processing task is dispatched to the `ThreadPool`.
- **Graceful Shutdown**: A `SIGINT` / `SIGTERM` handler sets an `std::atomic<bool>` flag. The event loop checks this flag on each iteration (with a 1-second `epoll_wait` timeout) and exits cleanly when signalled.

### 1.2 Threading Model

- **Main Thread**: Runs `epoll_wait`, accepts new connections, and performs initial `recv()` before handing off to workers.
- **Worker Threads**: Execute command parsing, password hashing, database operations, and `send()` calls.
- **Concurrency Control**:
  - `UserManager` uses `std::shared_mutex` to allow multiple concurrent readers (e.g., history lookups, login-status checks) while serialising writers (logins, registrations, group mutations).
  - `Database` uses a standard `std::mutex` to protect the single SQLite handle. SQLite is configured in WAL mode so that read queries do not block behind write transactions.

## 2. Key Implementation Details

### 2.1 TCP Message Framing

TCP is a byte-stream protocol — there is no inherent message boundary. Data may arrive fragmented across multiple `recv()` calls, or multiple messages may be concatenated in a single read (often called "partial reads" or, informally in some communities, "sticky packets").

**Solution**: Each `ClientSession` maintains a `read_buffer`. After every `recv()`, the server appends the new bytes to this buffer and then scans for `\n` as the message delimiter. Complete lines are extracted and processed; any remaining partial line stays in the buffer until the next read completes it.

### 2.2 Database Persistence

SQLite3 is used for both message history and user credential storage.

- **Single-Open Lifecycle**: The database is opened once at server startup and closed on shutdown. This avoids the overhead and concurrency hazards of opening/closing the handle on every operation.
- **WAL Mode**: `PRAGMA journal_mode=WAL` is set at startup. Write-Ahead Logging allows readers to proceed without being blocked by an active writer, which improves `/history` query latency during active chat sessions.
- **Tables**:
  - `messages` — stores sender, receiver, content, type (`broadcast` / `private` / `group`), and timestamp.
  - `users` — stores username and password hash.

### 2.3 Password Handling

Passwords are never stored in plaintext. Before persistence, each password is transformed via `std::hash<std::string>` and stored as a hex-encoded string.

> **Disclaimer**: `std::hash` is a non-cryptographic hash function — it is fast but not resistant to brute-force or rainbow-table attacks. A production system should use a dedicated password-hashing algorithm such as **bcrypt** or **Argon2**. The current approach is chosen to demonstrate the hashing workflow without adding an external dependency.

### 2.4 Message Visibility Logic

The `/history` command applies a per-user visibility filter before sending results:

| Message Type | Visible To |
|---|---|
| `broadcast` | All authenticated users |
| `private` | Only the sender and the receiver |
| `group` | Only verified members of that group (checked against `UserManager`'s group map) |

### 2.5 Safe Send

All outbound data passes through a `safe_send()` helper that:

1. Loops until all bytes are written, handling partial `send()` returns.
2. Uses `MSG_NOSIGNAL` to prevent `SIGPIPE` from crashing the server when a client disconnects mid-write.
3. Returns a boolean so callers can detect and clean up failed connections.

## 3. Configuration

Key compile-time constants are centralised in `Config.hpp`:

| Constant | Default | Purpose |
|---|---|---|
| `SERVER_PORT` | 12345 | TCP listen port |
| `LISTEN_BACKLOG` | 128 | `listen()` backlog size |
| `THREAD_POOL_SIZE` | 4 | Number of worker threads (tune to CPU core count) |
| `MAX_EPOLL_EVENTS` | 64 | Batch size for `epoll_wait` |
| `RECV_BUFFER_SIZE` | 4096 | Per-`recv()` buffer size |
| `DB_FILENAME` | `"chat.db"` | SQLite file path |

## 4. Known Limitations & Trade-offs

- **`read_buffer` Ownership**: Because `UserManager::getSession()` returns a copy for thread safety, partial lines that span two `recv()` calls on the same fd may be lost under high concurrency. A production fix would move per-fd buffers outside the shared session map into a separate structure with per-fd locking.
- **In-Memory Groups**: Group membership is stored only in memory. A server restart clears all groups (though message history is preserved in SQLite).
- **No TLS**: All traffic is plaintext. Adding OpenSSL or a reverse proxy would be required for secure deployment.

## 5. Future Roadmap

- **Heartbeat / Idle Timeout**: Detect and close dead connections using a timer wheel or `EPOLL` timeout tracking.
- **Protobuf / Binary Protocol**: Replace the text-based command parsing with a structured, versioned binary protocol for better extensibility and performance.
- **TLS/SSL**: Integrate OpenSSL (or use a TLS-terminating reverse proxy) for encrypted communication.
- **Per-fd Buffer Isolation**: Move `read_buffer` out of `ClientSession` into a dedicated, per-fd-locked structure to fully support partial-line accumulation under concurrent access.
- **Persistent Groups**: Store group membership in SQLite so groups survive server restarts.