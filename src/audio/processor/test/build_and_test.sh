#!/bin/bash

# Mac音频处理器测试构建和运行脚本

echo "Mac音频处理器测试构建脚本"
echo "========================"

# 创建构建目录
echo "创建构建目录..."
mkdir -p build
cd build

# 运行CMake配置
echo "运行CMake配置..."
cmake ..

if [ $? -ne 0 ]; then
    echo "错误: CMake配置失败"
    exit 1
fi

# 编译测试程序
echo "编译测试程序..."
make

if [ $? -ne 0 ]; then
    echo "错误: 编译失败"
    exit 1
fi

echo "编译成功!"

# 返回上级目录
cd ..

echo ""
echo "构建完成。要运行测试，请执行:"
echo "  cd build"
echo "  ./audio_processor_mac_test"
echo ""