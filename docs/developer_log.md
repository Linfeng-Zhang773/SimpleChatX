# Development Log — Day 1 (2025-06-23)

## Tasks completed

- Initialized project directory structure with all required folders and empty files.
- Created `Makefile` to support building the project.
- Implemented minimal `Server` class framework (`Server.hpp` and `Server.cpp`).
- Wrote `main.cpp` with basic startup code that instantiates `Server` and calls `run` method.
- Successfully compiled and ran the project, verifying build system and initial framework.

## Issues encountered

- None (or describe any problems you ran into and how you resolved them)

## Next steps

- Implement socket creation, binding, and `epoll` event loop in `Server`.
- Prepare to handle incoming client connections.

## Notes

- Ensured Makefile and project structure consistency.
- Set up `.gitignore` to exclude build artifacts.
