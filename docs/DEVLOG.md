# Development Log

## Development Log — Day 1 (2025-06-23)

### Tasks completed

- Initialized project directory structure with all required folders and empty files.
- Created `Makefile` to support building the project.
- Implemented minimal `Server` class framework (`Server.hpp` and `Server.cpp`).
- Wrote `main.cpp` with basic startup code that instantiates `Server` and calls `run` method.
- Successfully compiled and ran the project, verifying build system and initial framework.

### Issues encountered

- None (or describe any problems you ran into and how you resolved them)

### Next steps

- Implement socket creation, binding, and `epoll` event loop in `Server`.
- Prepare to handle incoming client connections.

### day1_Notes

- Ensured Makefile and project structure consistency.
- Set up `.gitignore` to exclude build artifacts.

## Development Log — Day 2 (2025-06-24)

### Tasks Completed

- Implemented `Server::create_and_bind()`:
  - Created a TCP socket using `socket()`.
  - Bound it to `INADDR_ANY:12345` with `bind()`.
  - Started listening with `listen()`.
  - Set the socket to non-blocking mode using `fcntl()` or helper `set_Nonblocking()`.

- Implemented `Server::setup_epoll()`:
  - Created epoll instance with `epoll_create1()`.
  - Registered `listen_fd` to the epoll instance using `epoll_ctl()` with `EPOLLIN` event.

- Implemented `Server::run_event_loop()`:
  - Entered event loop with `epoll_wait()`.
  - Identified readiness on `listen_fd` via `EPOLLIN`.
  - Accepted new incoming client connections using `accept()`.
  - Set new `client_fd` to non-blocking.
  - Registered `client_fd` to epoll with `EPOLLIN | EPOLLRDHUP`.
  - Logged new connections.

### ClientSession Design

- Designed `ClientSession` class to track per-client state.
- Created a `std::unordered_map<int, ClientSession>` (`clients`) to manage connected clients.
- Successfully associated `client_fd` with `ClientSession` instance upon connection.

### Issues Encountered

- Initially forgot `main()` entry point — server failed to launch.
- Encountered repeated epoll triggers before `accept()` was added:
  - Learned that `EPOLLIN` will persist until pending connections are handled via `accept()`.

- Debugged `ClientSession(fd)` constructor error:
  - Error due to missing default constructor when inserting into map.
  - Fixed by passing required `fd` during insertion.

### Notes & Insights

- Verified server can now accept multiple clients using `telnet localhost 12345`.
- Understood how to manage file descriptor lifecycle and avoid starvation.
- Learned how `epoll_wait()` delivers one or more events and how to handle them efficiently.
- Recognized the importance of handling `EPOLLRDHUP` to detect client disconnections (to be implemented in Day 3).

### Next Steps (Planned for Day 3)

- Handle client disconnections (`EPOLLRDHUP`) and clean up:
  - Remove `client_fd` from epoll
  - Close socket and delete from `clients` map
- Prepare for client read/write events (handle `EPOLLIN`)
- Expand `ClientSession` to buffer input data
