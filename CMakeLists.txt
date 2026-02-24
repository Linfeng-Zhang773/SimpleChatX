cmake_minimum_required(VERSION 3.10)

# Project name
project(ChatServer LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all source files from srcs directory
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/srcs/*.cpp
)

# Find SQLite3 package
find_package(SQLite3 REQUIRED)

# Create executable target
add_executable(ChatServer ${SOURCES})

# Add include directory (modern CMake style)
target_include_directories(ChatServer
    PRIVATE ${CMAKE_SOURCE_DIR}/includes
)

# Link SQLite3 library
target_link_libraries(ChatServer
    PRIVATE SQLite::SQLite3
)