# Development Log

## Development Log — Day 1 (2025-06-23)

### Tasks completed

- Initialized project directory structure with all required folders and empty files.
- Created `Makefile` to support building the project.
- Implemented minimal `Server` class framework (`Server.hpp` and `Server.cpp`).
- Wrote `main.cpp` with basic startup code that initilizes `Server` and calls `run` method.
- Successfully compiled and ran the project, verifying build system and initial framework.

### Issues encountered

- None

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

## Development Log — Day 3 (2025-06-29)

### Day3_Tasks Completed

- Implemented client disconnection handling:
  - Detected disconnections via `EPOLLRDHUP` and `EPOLLHUP`.
  - Called `epoll_ctl(EPOLL_CTL_DEL, fd, nullptr)` to remove from epoll monitoring.
  - Closed the socket with `close(fd)` and erased from `clients` map.
  - Logged disconnection events clearly.

- Implemented basic message receiving:
  - Used `recv()` to read from `client_fd`.
  - Handled return values of `recv()`:
    - `n == 0`: client closed connection cleanly.
    - `n < 0`: ignored `EAGAIN`/`EWOULDBLOCK`, treated others as errors.
    - `n > 0`: read input into buffer.

- Implemented message broadcasting:
  - Formatted message as `[fd: X]: <message>`.
  - Sent to all other clients via loop over `clients`.

- Tested with multiple telnet clients:
  - Verified server logs and broadcast worked as expected.

### Day3_ClientSession Design

- No major change in `ClientSession` on Day 3.
- Still used as a placeholder for future buffer expansion and message handling.
- Confirmed structure supports fd association and can be extended on Day 4.

### Day3_Issues Encountered

- **recv() broadcast not triggered**:
  - Issue: broadcast code mistakenly placed in wrong `if` branch.
  - Fix: ensured message sending only occurs when `n > 0`.

- **bind failed!: Address already in use**:
  - Encountered after crash or abrupt shutdown.
  - Resolved by waiting or restarting terminal (will add `SO_REUSEADDR` later).

### Day3_Notes & Insights

- `recv()` must be carefully checked for multiple return cases in non-blocking mode.
- `epoll` only notifies readiness; actual data logic must be done with care.
- `send()` must use `.c_str()` with `.size()` to send entire message string.
- Iterating over `unordered_map<int, ClientSession>` with structured binding is neat and readable.

### Day3_Next Steps (Planned for Day 4)

- Extend `ClientSession` to include read buffer.
- Implement partial read handling and buffering until newline.
- Design framing protocol (e.g., line-delimited messages).
- Improve message structure: include nickname or ID (not just `fd`).
- Consider modularizing event handlers for better readability and separation of concerns.

## Development Log — Day 4 (2025-06-30)

### Day4_Tasks Completed

- Added support for **buffered message input** to handle incomplete or multi-part messages from clients.
- Introduced `read_buffer` in `ClientSession` to accumulate input data until a full message (ending in `\n`) is received.
- Modified `Server::run_event_loop()`:
  - On receiving data via `recv()`, append it to `read_buffer`.
  - Check if `read_buffer` contains a full line (i.e., contains `\n`).
  - Only when a complete line is received, broadcast it to other clients.
  - Preserve remaining partial input in the buffer for the next read.
  - Improve robustness of message broadcasting:
  - Add error handling around `send()`.
  - Detect and handle broken pipes or disconnected clients during send.

### Day4_ClientSession Design

- `ClientSession` now includes:
  - `int fd`: file descriptor.
  - `std::string read_buffer`: used to accumulate partial user input until newline.

### Day4_Issues Encountered

- Initially attempted to send message immediately after `recv()` without checking for newline — caused partial messages to be sent mid-typing.
- Incorrectly checked for `"\0"` as message delimiter — fixed to use `\n` instead.
- In `read_buffer.erase(0, i + 1);`, mistakenly used wrong variable name `i` instead of correct `pos` — corrected during testing.
- Realized that `localhost` is not accessible by friends on other machines — clarified that they must use the host's LAN IP.

### Day4_Notes & Insights

- Learned that TCP is stream-oriented — data may arrive in chunks that don't align with logical message boundaries.
- Buffered input is necessary for correct message framing; it's dangerous to assume one `recv()` == one message.
- Realized importance of testing edge cases such as slow typing, multi-line messages, and long pauses.
- Understood that telnet on another computer cannot connect to `localhost`; must use the actual IP address of the host.

### Day4_Next Steps (Planned for Day 5)

- Add timestamp or username support (optional, for logging/user distinction).
- Explore how to support command handling or admin messages in future design.

## Development Log — Day 5 (2025-07-15)

### Day5_Tasks Completed

- Built **basic user authentication system**:  
  Users must now register (`/reg <username>`) or log in (`/login <username>`) before sending messages.  
  Server rejects messages from unauthenticated users.
- Implemented `ClientSession` **status tracking** using `AuthStatus` enum (`NONE`, `AUTHORIZED`).
- Refactored `handle_client_input` to process commands (`/reg`, `/login`, `/quit`) and normal messages more clearly.
- Added `/quit` command support — cleanly disconnects clients.
- Blocked **already authenticated users** from issuing `/reg` or `/login` again.
- Improved message formatting:
  - Broadcast messages now show as `[username]: <message>`.
  - Server also echoes messages back to the sender for better design.
- Fixed **partial message issues** with buffered input via `read_buffer`.
- Enhanced validation for usernames:
  - Disallowed empty names, overly long names, or names containing whitespace.
- Added `/quit` to the welcome prompt.  
  Fixed missing line caused by incorrect C++ string literal usage.
- Confirmed **telnet** works for multi-client testing; verified correct behavior with concurrent users.
- Added `CMakeLists.txt` file to support **CMake-based builds**; tested and confirmed successful compilation.

---

### Day5_ClientSession Design

- `enum class AuthStatus`: a enum class for status
- `int fd`: client's socket file descriptor.
- `std::string read_buffer`: accumulated message buffer.
- `std::string nickname`: username set upon registration/login.
- `AuthStatus status`: indicates if the client is authenticated (`NONE`, `AUTHORIZED`).

---

### Day5_Issues Encountered

- **Welcome message truncation**: caused by incorrect multiline string (missing `+=` or unassigned block) — fixed using `std::string` concatenation.
- **Login logic executed when already authenticated** — added guard to block repeated `/reg` or `/login`.
- **Partial input misparsed** (e.g., command fragment sent prematurely) — fixed using `read_buffer` and newline detection.
- Forgot to `pop_back()` `\r` from telnet input — caused incorrect command matching — fixed.
- Misunderstood `CMake` vs. `make` coexistence — clarified that CMake generates Makefiles and should not be mixed with raw `make`.

---

### Day5_Notes & Insights

- Telnet input ends with `\r\n` — trimming `\r` is crucial for correct command parsing.
- A simple user auth system isolates unauthenticated input effectively.
- Directly processing `recv()` data is error-prone — **buffering and delimiter parsing are essential**.
- CMake greatly improves **build scalability and readability**.
- Echoing messages to the sender enhances user feedback experience.
- Explicit enums like `AuthStatus` make user state transitions **safe and debuggable**.

---

### Day5_Next Steps (Planned for Later tasks)

- Add **password support** for `/reg <username> <password>` and `/login <username> <password>`.
- Use **SQLite** API to persist:
  - User credentials
  - Chat logs
- Implement **private messaging**: `/msg <username> <message>`
- Support **chat history replay** via `/history` (last N messages)
- Build **thread pool** to handle concurrent message parsing & I/O
- Enable **group chat rooms**: `/join <group>`, `/gmsg <group> <message>`
- Support **session recovery** via `/reconnect <username> <password>`
- Implement **graceful shutdown**: catch `SIGINT`, close sockets, flush buffers
- Add **rate limiting / abuse protection**

---
