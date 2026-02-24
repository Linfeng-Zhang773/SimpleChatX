# SimpleChatX — High-Performance Event-Driven Chat Server

A high-performance C++17 chat server built on Linux `epoll` and a worker thread pool. Supports multi-client messaging, persistent user authentication, and SQLite-based chat history.

## Overview

| Feature | Description |
|---------|-------------|
| **I/O Strategy** | Non-blocking I/O with Linux `epoll` (Level-Triggered) |
| **Concurrency** | Single Reactor + Thread Pool (main thread dispatches to workers) |
| **Persistence** | SQLite3 with WAL mode for high-concurrency read/write |
| **Auth System** | Registration & Login with password hashing (`std::hash`; bcrypt recommended for production) |
| **Messaging** | Private (`/to`), Group (`/group`), and Broadcast modes |
| **Reliability** | Application-level buffering with `\n`-delimited message framing for TCP partial-read handling |
| **Thread Safety** | `shared_mutex` (read-write lock) for session management; `mutex` for database access |

For detailed architectural notes and design decisions, see [Design.md](./docs/Design.md).

## Build & Run

### Prerequisites

- CMake ≥ 3.10
- g++ ≥ 7.0 (C++17 support)
- SQLite3 Development Library (`libsqlite3-dev`)
- Linux environment (Kernel 2.6.28+)

### Build && Clean

```bash
./build.sh
```

```bash
./clean.sh
```

### Run Server

```bash
./ChatServer
```

The server listens on port **12345** by default (configurable in `Config.hpp`).

### Quick Start (Client)

```bash
telnet localhost 12345
```

Once connected:

| Command | Description |
|---------|-------------|
| `/reg <user> <pass>` | Register a new account |
| `/login <user> <pass>` | Log in with existing credentials |
| `/to <user> <msg>` | Send a private message |
| `/create <group>` | Create a chat group |
| `/join <group>` | Join an existing group |
| `/group <group> <msg>` | Send a message to a group |
| `/history` | View the latest 50 messages |
| `/quit` | Disconnect |

## Architecture at a Glance

```
  Clients (telnet / TCP)
        │
        ▼
  ┌─────────────┐
  │  listen_fd   │  Main Thread
  │  epoll_wait  │  (Single Reactor)
  └──────┬──────┘
         │ dispatch
         ▼
  ┌─────────────┐
  │  ThreadPool  │  Worker Threads
  │  (N workers) │  Command parsing, DB ops
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │   SQLite3    │  WAL mode
  │  (messages   │  mutex-protected
  │   + users)   │
  └─────────────┘
```

## Performance Highlights

- **Low Latency**: Business logic (command parsing, DB writes) is offloaded to the thread pool, keeping the I/O loop responsive.
- **Scalable Reads**: `UserManager` uses `std::shared_mutex` so concurrent login-status checks and history lookups don't block each other.
- **WAL Database**: SQLite configured in Write-Ahead Logging mode allows readers and writers to operate concurrently.
- **Graceful Shutdown**: `SIGINT` / `SIGTERM` handlers set an atomic flag; the event loop exits cleanly and all resources are released.


## Author

- **Name:** Linfeng Zhang
- **Email:** linfengzh01@gmail.com
- **School:** UC San Diego — Computer Science and Engineering (Computer Engineering)
- **Interests:** Systems programming, networking, and backend infrastructure