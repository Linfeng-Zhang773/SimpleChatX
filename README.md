# 项目介绍（Chinese Version）

## 💬 高性能聊天服务器（C++ / Socket / epoll / 线程池 / 多用户系统）

基于 epoll 实现非阻塞 I/O 模型，支持多个客户端同时连接的实时聊天服务。  
构建线程池处理并发消息收发，实现客户端私聊、群聊及断线重连功能。  
设计简易用户系统，包括登录、认证、昵称管理与会话状态恢复。  
实现聊天记录本地持久化，支持历史消息回溯与日志输出。

---

## ⚠️ 用户系统持久化说明

目前用户注册信息（用户名 / 密码）**仅保存在内存中**，未能进行持久化存储。  
因此在以下情况下会出现用户数据丢失的问题：

- 服务器异常崩溃或主动关闭
- 进程重启或系统重启

此设计适用于测试环境或临时聊天会话。若需在正式环境中部署，请考虑：

- 使用 SQLite 或其他数据库保存注册用户信息
- 为 `UserManager` 模块添加加载 / 保存用户信息的接口
- 引入加密存储机制保障密码安全性（如 bcrypt / SHA256 + salt）

💡 后续计划：将在下一版本中实现用户系统的持久化支持。

## 🚀 功能特性

- 用户注册 / 登录（用户名 + 密码，支持 `/reg`、`/login`）
- 实时私聊与群聊（支持 `/to` 和 `/group` 指令）
- 群组创建与加入（支持 `/create`、`/join`）
- 最近消息历史查看（支持 `/history`）
- 聊天记录持久化（SQLite 支持广播 / 私聊 / 群聊三类消息）
- 支持多客户端并发连接（epoll + 线程池）

---

## 📦 项目结构

```bash
.
├── includes/           # 头文件
├── src/                # 源代码
├── chat.db             # 聊天记录数据库（运行后生成）
├── CMakeLists.txt      # CMake 构建文件
├── Makefile            # Makefile
├── DEVLOG.md           # 开发日志
└── README.md
```

---

## 🛠️ 编译运行

### 构建项目

```bash
make
```

### 启动服务器

```bash
./scripts/start_server.sh
```

### 启动客户端（默认连接 localhost:12345）

```bash
./scripts/start_client.sh
```

> ⚠️ 注意事项：
>
> - 若服务器运行于 WSL 中，外部设备无法直接连接。
> - 若需实现局域网访问，可考虑：
>   - 在 Windows 原生系统运行服务端；
>   - 为 WSL 设置端口转发；
>   - 确保 Windows 防火墙未阻止连接。

---

## 💬 聊天指令说明

- `/reg <username> <password>`：注册用户  
- `/login <username> <password>`：登录用户  
- `/to <username> <message>`：私聊指定用户  
- `/create <groupname>`：创建群组  
- `/join <groupname>`：加入群组  
- `/group <groupname> <message>`：在群组内发言  
- `/history`：查看最近 50 条消息  
- `/quit`：退出聊天  

---

## 🧱 依赖环境

- g++ ≥ C++17
- Linux 操作系统（支持 epoll）
- SQLite3 开发库（可通过以下命令安装）：

```bash
sudo apt install libsqlite3-dev
```

---

## 📌 TODO（可选扩展）

- 登录成功后推送离线消息 / 历史摘要
- 添加 Web 客户端或 GUI 客户端
- TLS 加密通信支持
- 用户状态（在线 / 离线）管理
- 消息分页 / 消息撤回 / 屏蔽功能

---

## Introduction(EN)

## 💬 High-Performance Chat Server (C++ / Socket / epoll / Thread Pool / Multi-user System)

This project implements a real-time chat server supporting multiple clients using epoll-based non-blocking I/O.  
It handles concurrent message transmission via a thread pool and supports private/group chats with session recovery.  
A simple user system is designed, including login, authentication, nickname management, and session state tracking.  
Chat messages are persisted locally via SQLite, supporting message history retrieval and logging.

---

## ⚠️ User System Persistence Notice

Currently, user registration info (username / password) is stored **only in memory** and is **not persistent**.  
This causes loss of all user data under the following conditions:

- Server crash or manual shutdown
- Process or system reboot

This design is suitable for testing or temporary usage. For production use, consider:

- Persisting users into SQLite or other databases
- Adding load/save methods to `UserManager`
- Securing passwords with encryption (e.g., bcrypt / SHA256 + salt)

💡 Planned Feature: Persistent user management will be implemented in the next version.

---

## 🚀 Features

- User registration and login (username + password with `/reg` and `/login`)
- Real-time private and group messaging (`/to` and `/group`)
- Group creation and joining (`/create`, `/join`)
- View recent chat history (`/history`)
- Message persistence with SQLite (supports broadcast / private / group types)
- Concurrent client connections using epoll + thread pool

---

## 📦 Project Structure

```bash
.
├── includes/           # Header files
├── src/                # Source code
├── scripts/            # Startup scripts
├── chat.db             # SQLite chat log (generated after running)
├── CMakeLists.txt      # CMake build file
├── Makefile            # Makefile
├── DEVLOG.md           # Development log
└── README.md
```

---

## 🛠️ Build & Run

### Build the project

```bash
make
```

### Start the server

```bash
./scripts/start_server.sh
```

### Start the client (default: localhost:12345)

```bash
./scripts/start_client.sh
```

> ⚠️ Note:
>
> - If the server is running under WSL, external devices may not be able to connect.
> - For LAN access, consider:
>   - Running server natively on Windows;
>   - Configuring WSL port forwarding;
>   - Ensuring firewall does not block the connection.

---

## 💬 Chat Commands

- `/reg <username> <password>`: Register a user  
- `/login <username> <password>`: Login with an existing user  
- `/to <username> <message>`: Send a private message  
- `/create <groupname>`: Create a group  
- `/join <groupname>`: Join a group  
- `/group <groupname> <message>`: Send a message in group  
- `/history`: View the latest 50 messages  
- `/quit`: Exit the chat  

---

## 🧱 Dependencies

- g++ ≥ C++17
- Linux system with epoll support
- SQLite3 development library:

```bash
sudo apt install libsqlite3-dev
```

---

## 📌 TODO (Optional Features)

- Push offline messages / history on login
- Add Web or GUI client
- Enable TLS encrypted communication
- Show online / offline user status
- Add message pagination / recall / mute features

---
