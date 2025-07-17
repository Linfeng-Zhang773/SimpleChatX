#!/bin/bash

# 设置为有错误就退出
set -e

# 创建build目录
if [ ! -d build ]; then
    mkdir build
fi

# 进入build
cd build

# 运行cmake
cmake ..

# 编译
make -j$(nproc)

# 运行server
echo "start ChatServer..."
./ChatServer