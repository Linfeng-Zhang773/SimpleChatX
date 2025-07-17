#!/bin/bash

# 默认连接到 localhost 的 12345 端口
HOST="localhost"
PORT=12345


# 目前暂不支持非本地连接，因在 WSL 中运行服务端，
# 虽然绑定了 INADDR_ANY 能监听所有地址，
# 但由于 WSL 使用虚拟网络（NAT 模式），外部设备无法直接访问 WSL 的 IP 和端口，
# 除非进行端口转发或将服务端直接运行在 Windows 主机上。
# 同时，还需确保防火墙未阻止端口的访问
# 未来支持参数输入（暂无法使用）：
#if [ ! -z "$1" ]; then
  #HOST=$1
#fi

#if [ ! -z "$2" ]; then
  #PORT=$2
#fi

echo "Connecting to ${HOST}:${PORT} ..."
telnet $HOST $PORT
