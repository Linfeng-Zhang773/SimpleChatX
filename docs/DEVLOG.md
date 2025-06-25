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

- Implemented `Server::create_and_bind()` to:
  - Create a TCP socket (`socket()`)
  - Bind it to `INADDR_ANY:12345` (`bind()`)
  - Start listening with `listen()`
  - Set the socket to non-blocking mode (`fcntl()`)

- Implemented `Server::setup_epoll()`:
  - Created an epoll instance (`epoll_create1`)
  - Registered `listen_fd` with `EPOLLIN` event via `epoll_ctl`

- Implemented `Server::run_event_loop()`:
  - Called `epoll_wait()` in a loop to wait for events
  - Printed notification when the listening socket is ready to accept connections

- Successfully ran the server and verified with `telnet localhost 12345`
  - Observed repeated epoll triggers before `accept()` is added (expected behavior)
  
### Issues Encountered

- Initially forgot to write `main()` function — caused server not to start
- Encountered repeated epoll events due to missing `accept()` — learned that unaccepted connections remain in the queue and re-trigger `EPOLLIN`

### Next Steps (Day 3)

- Handle new client connections with `accept()`
- Set `client_fd` to non-blocking
- Register `client_fd` with epoll
- Begin designing `ClientSession` abstraction

### day2_Notes

- Confirmed basic epoll mechanism works correctly
- Gained solid understanding of why unaccepted connections repeatedly trigger `EPOLLIN`
- All core logic currently lives in `Server.{hpp,cpp}` and `main.cpp`
