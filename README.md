# SimpleChatX — High-Performance Event-Driven Chat Server

A robust, multi-threaded C++ chat backend featuring asynchronous I/O with epoll, thread-pool task distribution, and persistent storage with SQLite3.

## Overview

| Feature | Description |
|---------|-------------|
| **I/O Strategy** | Non-blocking I/O with Linux `epoll` (Edge-Triggered) |
| **Concurrency** | One-Loop-Per-Thread Reactor pattern with Thread Pool |
| **Persistence** | SQLite3 with WAL mode for high-concurrency logging |
| **Auth System** | Registration & Login with hashed password verification |
| **Messaging** | Support for Private (/to), Group (/group), and Broadcast |
| **Reliability** | Application-level buffering for TCP sticky packet handling |
| **Thread Safety** | Read-Write locks (`shared_mutex`) for session management |

For detailed architectural notes and logic flow, see [Design.md](./docs/Design.md).

## Build & Run

### Prerequisites

- CMake ≥ 3.10
- g++ ≥ 7.0 (C++17 support)
- SQLite3 Development Library (`libsqlite3-dev`)
- Linux environment (Kernel 2.6.28+)

### Build

```bash
./build.sh
```

### Clean

```bash
./clean.sh
```

### Run server

```bash
./chat_server
```

### Quick start(client)

```bash
telnet localhost 12345
```
Once connected, use the following commands:
- `/reg <username> <password>`: Register a user  
- `/login <username> <password>`: Login with an existing user  
- `/to <username> <message>`: Send a private message  
- `/create <groupname>`: Create a group  
- `/join <groupname>`: Join a group  
- `/group <groupname> <message>`: Send a message in group  
- `/history`: View the latest 50 messages  
- `/quit`: Exit the chat  

### Performance Highlights
1. Low Latency: Task-offloading to ThreadPool ensures the I/O loop is never blocked by DB disk I/O.
2. Scalable Auth: UserManager utilizes shared_mutex to allow concurrent login status checks.
3. WAL Database: SQLite configured in Write-Ahead Logging mode to prevent readers from blocking writers.

### About Author

- **Name:** Linfeng Zhang
- **Email:** linfengzh01@gmail.com
- **School:** UCSD — Computer Science and Engineering (Computer Engineering)
- **Interests:** System programming, networking, and backend infrastructure