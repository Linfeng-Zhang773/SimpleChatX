cmake_minimum_required(VERSION 3.10)
project(ChatServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/includes)

# 查找源文件
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/srcs/*.cpp")

# 查找 SQLite3 库
find_package(SQLite3 REQUIRED)

# 指定可执行文件及其源文件
add_executable(ChatServer ${SOURCES})

# 链接 SQLite3 库
target_link_libraries(ChatServer SQLite::SQLite3)
