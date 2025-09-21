# Linx SDK 完整语音对话演示程序

这是一个完整的语音对话演示程序，展示了 Linx SDK 的全部功能，包括实时音频录制、语音识别、TTS 语音合成、WebSocket 通信和 MCP 工具调用。

## 🎯 功能特性

### 核心功能
- **🎤 实时音频录制**：高质量音频采集和实时处理
- **🔊 TTS 语音播放**：自然语音合成和播放
- **🎵 Opus 音频编解码**：高效音频压缩和传输
- **🌐 WebSocket 通信**：稳定的实时双向通信
- **🧵 多线程处理**：音频和网络并发处理
- **🔧 MCP 工具调用**：丰富的工具集成能力

### 交互体验
- **💬 语音对话**：自然的语音交互体验
- **📝 文本输入**：支持文本和语音混合输入
- **🎛️ 实时控制**：录音开始/停止控制
- **📊 状态监控**：实时显示连接和音频状态
- **🛠️ 工具展示**：可用 MCP 工具列表

## 🖥️ 系统要求

### 操作系统
- **macOS**: 10.15+ (推荐 12.0+)
- **Linux**: Ubuntu 20.04+, CentOS 8+
- **Windows**: Windows 10+ (通过 WSL2)

### 硬件要求
- **CPU**: 双核 2.0GHz 以上
- **内存**: 至少 512MB 可用内存
- **音频**: 麦克风和扬声器/耳机
- **网络**: 稳定的网络连接 (建议 1Mbps+)

### 依赖库

#### 核心依赖
```bash
# macOS (使用 Homebrew)
brew install opus json-c curl portaudio libsndfile fftw openssl

# Ubuntu/Debian
sudo apt-get install libopus-dev libjson-c-dev libcurl4-openssl-dev \
                     libportaudio2 libportaudio-dev libsndfile1-dev \
                     libfftw3-dev libssl-dev pkg-config
```

#### 编译工具
- **GCC**: 7.0+ 或 **Clang**: 10.0+
- **Make**: GNU Make 4.0+
- **pkg-config**: 用于依赖检查

## 🚀 快速开始

### 1. 安装依赖

```bash
# macOS
make install-deps-macos

# Ubuntu/Debian
make install-deps-ubuntu

# 手动检查依赖
make check-deps
```

### 2. 编译程序

```bash
# 进入 demo 目录
cd demo

# 编译程序 (会自动检查依赖)
make

# 查看编译信息
make info
```

### 3. 运行程序

```bash
# 启动语音对话演示
make run

# 显示程序帮助
make run-help

# 连接到指定服务器
make run-server

# 连接到本地测试服务器
make run-local
```

## 🎮 使用指南

### 启动程序

```bash
./bin/linx_demo [选项]

选项:
  -h, --help              显示帮助信息
  -s, --server URL        WebSocket服务器地址
  -i, --interactive       交互模式 (默认)
```

### 交互命令

程序启动后进入交互模式：

```
=== Linx 语音对话演示 ===
命令:
  /start    - 开始录音
  /stop     - 停止录音
  /status   - 显示状态
  /tools    - 显示MCP工具
  /help     - 显示帮助
  /quit     - 退出程序
  其他文本  - 发送文本消息

linx> 
```

### 基本操作流程

1. **启动程序**：`make run`
2. **开始录音**：输入 `/start`
3. **说话交流**：对着麦克风说话
4. **停止录音**：输入 `/stop`
5. **查看回复**：AI 会语音回复并显示文本
6. **文本输入**：直接输入文本消息
7. **工具调用**：说 "帮我查天气" 等触发工具

## 🔧 MCP 工具支持

### 内置工具

1. **🌤️ 天气查询**
   - 工具名：`get_weather`
   - 功能：获取指定城市天气信息
   - 用法：说 "北京天气怎么样"

2. **🧮 计算器**
   - 工具名：`calculator`
   - 功能：执行数学计算
   - 用法：说 "计算 123 加 456"

3. **📁 文件操作**
   - 工具名：`file_operation`
   - 功能：文件读写操作
   - 用法：说 "读取文件 config.txt"

### 查看工具列表

```bash
linx> /tools
可用工具:
{
  "tools": [
    {
      "name": "get_weather",
      "description": "获取指定城市的天气信息",
      "inputSchema": {
        "type": "object",
        "properties": {
          "location": {
            "type": "string",
            "description": "城市名称"
          }
        }
      }
    }
    // ... 更多工具
  ]
}
```

## 📊 状态监控

### 实时状态

```bash
linx> /status
连接状态: 已连接
录音状态: 录音中
播放状态: 未播放
```

### 事件日志

程序会实时显示各种事件：

```
✓ 已连接到服务器
🎤 开始录音...
♪ 收到音频数据: 1024 字节
💬 AI回复: 你好！我听到了你的问题。
🔊 开始TTS播放
🔇 TTS播放完成
🔧 MCP工具调用: get_weather
🌤️ 获取天气信息: {"location": "北京"}
```

## 🛠️ 开发和调试

### 编译选项

```bash
# 调试编译
make debug

# 内存检查
make valgrind

# 性能分析
make profile

# 代码检查
make lint

# 代码格式化
make format
```

## 🔄 示例会话

### 完整语音对话流程

```
$ make run
正在连接到服务器: ws://localhost:8080/ws
✓ 已连接到服务器
✓ 演示程序初始化成功
✓ MCP工具设置完成

=== Linx 语音对话演示 ===
命令:
  /start    - 开始录音
  /stop     - 停止录音
  /status   - 显示状态
  /tools    - 显示MCP工具
  /help     - 显示帮助
  /quit     - 退出程序
  其他文本  - 发送文本消息

linx> /start
🎤 开始录音...

[用户对着麦克风说话: "你好，今天北京的天气怎么样？"]

linx> /stop
🎤 停止录音
♪ 收到音频数据: 2048 字节
💬 AI回复: 你好！我来帮你查询北京的天气。
🔧 MCP工具调用: get_weather
🌤️ 获取天气信息: {"location": "北京"}
💬 AI回复: 北京今天天气晴朗，温度22°C，适合外出活动。
🔊 开始TTS播放
🔇 TTS播放完成

linx> 谢谢你的帮助
✓ 文本已发送
💬 AI回复: 不客气！还有什么我可以帮助你的吗？

linx> /quit
✓ 资源清理完成
程序退出
```

## 📄 许可证

本演示程序遵循 MIT 许可证。详见 LICENSE 文件。

## 🤝 支持

如有问题或建议，请：

1. 查看故障排除部分
2. 提交 Issue 到项目仓库
3. 联系开发团队

---

**享受与 AI 的语音对话体验！** 🎉