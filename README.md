
# Shell 运行脚本

## 启动服务器

./scripts/start_server.sh

## 启动客户端（默认连接 localhost:12345）

./scripts/start_client.sh

- 注意：
- 服务端若运行在 WSL 中，则外部设备（如其他电脑）无法直接访问。
- 若需实现局域网连接，需考虑：
- 使用 Windows 原生系统运行服务端
- 设置 WSL 端口转发
- 确保防火墙未阻止访问
