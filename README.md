# LinX OS SDK

<div align="center">

![LinX OS SDK](https://img.shields.io/badge/LinX%20OS%20SDK-v1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20ESP32%20%7C%20Allwinner-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-CMake-orange)

**智能语音交互SDK - 提供完整的实时语音对话解决方案**

</div>

## 📖 项目简介

LinX OS SDK 是一个跨平台的智能语音交互软件开发工具包，专为构建实时语音对话应用而设计。SDK 整合了音频处理、编解码、WebSocket 通信和 MCP (Model Context Protocol) 协议支持，为开发者提供了完整的语音交互解决方案。

### 🌟 核心特性

- **🎙️ 实时音频处理**: 支持音频录制、播放和实时流处理
- **🔊 高质量编解码**: 集成 Opus 音频编解码器，提供优秀的音质和压缩率
- **🌐 WebSocket 通信**: 基于 WebSocket 的实时双向通信协议
- **🔧 MCP 协议支持**: 支持 Model Context Protocol，实现工具调用和扩展功能
- **🖥️ 跨平台兼容**: 支持 macOS、Linux、ESP32、全志芯片等多个平台
- **📦 模块化设计**: 采用模块化架构，便于扩展和维护
- **🔒 线程安全**: 多线程安全设计，支持并发操作



### 🏗️ 架构概览

```
LinX OS SDK
├── 🎵 Audio Module        # 音频录制和播放
├── 🎛️ Codecs Module       # 音频编解码 (Opus)
├── 🌐 Protocols Module    # WebSocket 通信协议
├── 🔧 MCP Module          # Model Context Protocol
├── 📝 Log Module          # 日志系统
└── 📊 JSON Module         # JSON 数据处理
```

## 🚀 快速开始

### 📋 系统要求

#### macOS
- macOS 10.14 或更高版本
- Xcode 命令行工具
- CMake 3.16 或更高版本
- PortAudio 库

#### Linux
- Ubuntu 18.04 或更高版本 / CentOS 7 或更高版本
- GCC 7.0 或更高版本
- CMake 3.16 或更高版本
- ALSA 开发库

#### ESP32
- ESP-IDF 4.4 或更高版本
- Xtensa 工具链

#### 全志芯片 (Allwinner)
- 全志 SDK 开发环境
- ARM 交叉编译工具链
- 支持 A64、H3、H5、H6、H616 等系列芯片
- Tina Linux 或 Ubuntu 系统

### 🛠️ 安装依赖

#### macOS
```bash
# 安装 Homebrew (如果尚未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake portaudio pkg-config
```

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake libasound2-dev libportaudio2 portaudio19-dev pkg-config
```

#### CentOS/RHEL
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake alsa-lib-devel portaudio-devel pkgconfig
```

### 📦 编译安装

#### 1. 克隆项目
```bash
git clone https://github.com/sunqirui1987/linx-os-sdk.git
cd linx-os-sdk
```

#### 2. 构建 SDK
```bash
cd sdk
chmod +x run.sh
export RISCV32_TOOLCHAIN_PATH="/home/sqr-ubuntu/nds32le-linux-musl-v5d" && ./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake 
```

构建脚本会自动：
- 下载并编译第三方依赖 (Mongoose, Opus)
- 构建所有 SDK 模块
- 生成静态库和头文件
- 安装到 `sdk/build/install` 目录

#### 3. 构建演示程序
```bash
cd ../demo/mac
mkdir build && cd build
cmake ..
make
```

### 🎯 运行演示

```bash
# 运行演示程序
./linx_demo

# 指定服务器地址
./linx_demo -s ws://your-server.com/v1/ws/

# 查看帮助
./linx_demo --help
```

## 🔧 编译工具链集成

LinX OS SDK 提供了完整的跨平台编译工具链支持，通过 CMake 工具链文件实现不同目标平台的编译。SDK 支持多种架构和操作系统，包括嵌入式设备和桌面系统。

### 🛠️ 工具链架构

```
编译工具链系统
├── 🖥️ 主机平台 (Host)
│   ├── macOS (x86_64/arm64)
│   ├── Linux (x86_64/arm64)
│   └── Windows (x86_64)
├── 🎯 目标平台 (Target)
│   ├── ARM Linux (arm-linux-gnueabihf)
│   ├── RISC-V (riscv32/riscv64)
│   ├── ESP32 (xtensa-esp32)
│   ├── 全志芯片 (aarch64-linux-gnu)
│   └── x86_64 Linux (native)
└── 🔗 交叉编译工具链
    ├── GCC 工具链
    ├── Clang/LLVM 工具链
    └── 厂商专用工具链
```

### 📦 支持的工具链

| 平台 | 架构 | 工具链 | CMake 工具链文件 | 说明 |
|------|------|--------|------------------|------|
| **Linux x86_64** | x86_64 | GCC/Clang | `native-linux.cmake` | 本地编译 |
| **ARM Linux** | armv7/armv8 | arm-linux-gnueabihf | `arm-linux-gnueabihf.cmake` | ARM 嵌入式 Linux |
| **RISC-V** | riscv32/riscv64 | riscv-linux-musl | `riscv32-linux-musl.cmake` | RISC-V 架构 |
| **ESP32** | xtensa | esp-idf | `esp32.cmake` | ESP32 物联网平台 |
| **全志芯片** | aarch64 | aarch64-linux-gnu | `allwinner-aarch64.cmake` | 全志 ARM64 芯片 |
| **全志芯片** | armv7 | arm-linux-gnueabihf | `allwinner-armv7.cmake` | 全志 ARM32 芯片 |
| **全志V821** | riscv32 | nds32le-linux-musl-v5d | `riscv32-linux-musl.cmake` | 全志V821 RISC-V 无线SoC |

### 🔧 工具链配置

#### 1. RISC-V 工具链配置

```bash
# 设置 RISC-V 工具链路径
export RISCV32_TOOLCHAIN_PATH="/opt/riscv32-linux-musl"

# 编译 RISC-V 32位版本
cd sdk
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake

# 编译 RISC-V 64位版本
export RISCV64_TOOLCHAIN_PATH="/opt/riscv64-linux-musl"
./run.sh --toolchain cmake/toolchains/riscv64-linux-musl.cmake
```

#### 2. 全志芯片工具链配置

```bash
# 全志 A64/H5/H6 系列 (ARM64)
export ALLWINNER_TOOLCHAIN_PATH="/opt/aarch64-linux-gnu"
export ALLWINNER_SYSROOT="/opt/allwinner-sysroot"

# 编译全志 ARM64 版本
./run.sh --toolchain cmake/toolchains/allwinner-aarch64.cmake

# 全志 H3/H2+ 系列 (ARM32)
export ALLWINNER_ARM32_TOOLCHAIN_PATH="/opt/arm-linux-gnueabihf"
./run.sh --toolchain cmake/toolchains/allwinner-armv7.cmake
```

#### 3. ESP32 工具链配置

```bash
# 设置 ESP-IDF 环境
source $IDF_PATH/export.sh

# 编译 ESP32 版本
./run.sh --toolchain cmake/toolchains/esp32.cmake
```

#### 4. ARM Linux 工具链配置

```bash
# ARM 硬浮点工具链
export ARM_TOOLCHAIN_PATH="/opt/arm-linux-gnueabihf"
./run.sh --toolchain cmake/toolchains/arm-linux-gnueabihf.cmake

# ARM 软浮点工具链
export ARM_TOOLCHAIN_PATH="/opt/arm-linux-gnueabi"
./run.sh --toolchain cmake/toolchains/arm-linux-gnueabi.cmake
```

#### 5. 全志V821 RISC-V 工具链配置

```bash
# 下载并解压工具链
# 链接: https://pan.baidu.com/s/1f-xLwrOjHntsW4LyO1KKWw 提取码: 5ser
tar -xf nds32le-linux-musl-v5d.tar.xz

# 设置全志V821工具链路径
export ALLWINNER_V821_TOOLCHAIN_PATH="/path/to/nds32le-linux-musl-v5d"
export PATH="$ALLWINNER_V821_TOOLCHAIN_PATH/bin:$PATH"

# 验证工具链
riscv32-linux-musl-gcc --version

# 编译全志V821版本
cd sdk
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake

# 使用特定编译选项
export V821_CFLAGS="-g -ggdb -Wall -O3 -march=rv32imfdcxandes -mabi=ilp32d -mcmodel=medany"
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake
```

### 🏗️ 自定义工具链

#### 创建自定义工具链文件

```cmake
# 示例: cmake/toolchains/custom-platform.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 设置工具链路径
set(TOOLCHAIN_PREFIX arm-custom-linux-gnueabihf)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

# 设置系统根目录
set(CMAKE_FIND_ROOT_PATH /opt/custom-sysroot)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 设置编译选项
set(CMAKE_C_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard")
```

#### 使用自定义工具链

```bash
# 使用自定义工具链编译
./run.sh --toolchain cmake/toolchains/custom-platform.cmake
```

## 📚 使用指南

### 🔧 基本用法

#### 1. 初始化 SDK

```c
#include "linx_sdk.h"

// 配置 SDK
LinxSdkConfig config = {
    .server_url = "ws://your-server.com/v1/ws/",
    .sample_rate = 16000,
    .channels = 1,
    .timeout_ms = 5000,
    .auth_token = "your-auth-token",
    .device_id = "device-001",
    .client_id = "client-001",
    .protocol_version = 1,
    .listening_mode = LINX_LISTENING_CONTINUOUS
};

// 创建 SDK 实例
LinxSdk* sdk = linx_sdk_create(&config);
if (!sdk) {
    printf("Failed to create SDK instance\n");
    return -1;
}
```

#### 2. 设置事件回调

```c
void event_handler(const LinxEvent* event, void* user_data) {
    switch (event->type) {
        case LINX_EVENT_STATE_CHANGED:
            printf("状态变化: %d -> %d\n", 
                   event->data.state_changed.old_state,
                   event->data.state_changed.new_state);
            break;
            
        case LINX_EVENT_TEXT_MESSAGE:
            printf("收到文本: %s (来自: %s)\n",
                   event->data.text_message.text,
                   event->data.text_message.role);
            break;
            
        case LINX_EVENT_AUDIO_DATA:
            // 处理音频数据
            play_audio(event->data.audio_data.value);
            break;
            
        case LINX_EVENT_ERROR:
            printf("错误: %s (代码: %d)\n",
                   event->data.error.message,
                   event->data.error.code);
            break;
            
        default:
            break;
    }
}

// 设置回调
linx_sdk_set_event_callback(sdk, event_handler, NULL);
```

#### 3. 连接和通信

```c
// 连接到服务器
LinxSdkError result = linx_sdk_connect(sdk);
if (result != LINX_SDK_SUCCESS) {
    printf("连接失败: %d\n", result);
    return -1;
}

// 发送文本消息
linx_sdk_send_text(sdk, "你好，我想了解天气情况");

// 发送音频数据
uint8_t audio_data[1024];
// ... 填充音频数据 ...
linx_sdk_send_audio(sdk, audio_data, sizeof(audio_data));

// 轮询事件
while (running) {
    linx_sdk_poll_events(sdk, 100); // 100ms 超时
}
```

#### 4. 清理资源

```c
// 断开连接
linx_sdk_disconnect(sdk);

// 销毁 SDK 实例
linx_sdk_destroy(sdk);
```

### 🔧 MCP 工具集成

SDK 支持 MCP (Model Context Protocol) 工具调用，允许 AI 模型调用自定义功能：

```c
#include "mcp/mcp_server.h"

// 定义工具回调函数
mcp_return_value_t weather_tool_callback(const struct mcp_property_list* properties) {
    // 获取位置参数
    const char* location = mcp_property_get_string(properties, "location");
    
    // 调用天气 API
    char* weather_data = get_weather_info(location);
    
    // 返回结果
    return mcp_return_string(weather_data);
}

// 注册 MCP 工具
mcp_property_list_t* properties = mcp_property_list_create();
mcp_property_add_string(properties, "location", "城市名称", true);

linx_sdk_add_mcp_tool(sdk, "get_weather", "获取天气信息", 
                      properties, weather_tool_callback);
```

### 🎵 音频处理

#### 音频录制
```c
#include "audio/audio_interface.h"
#include "audio/portaudio_mac.h"

// 创建音频接口
AudioInterface* audio = portaudio_mac_create();

// 配置音频参数
AudioConfig audio_config = {
    .sample_rate = 16000,
    .channels = 1,
    .format = AUDIO_FORMAT_S16LE,
    .buffer_size = 1024
};

// 初始化音频
audio->init(audio, &audio_config);

// 开始录制
audio->start_record(audio);

// 读取音频数据
uint8_t buffer[1024];
size_t bytes_read = audio->read(audio, buffer, sizeof(buffer));

// 停止录制
audio->stop_record(audio);
```

#### 音频播放
```c
// 开始播放
audio->start_play(audio);

// 写入音频数据
audio->write(audio, audio_data, data_size);

// 停止播放
audio->stop_play(audio);
```

### 🎛️ 音频编解码

```c
#include "codecs/audio_codec.h"
#include "codecs/opus_codec.h"

// 创建 Opus 编码器
audio_codec_t* encoder = opus_codec_create_encoder(16000, 1, OPUS_APPLICATION_VOIP);

// 编码音频
uint8_t input_pcm[320];  // 20ms at 16kHz
uint8_t encoded_data[1024];
size_t encoded_size = encoder->encode(encoder, input_pcm, sizeof(input_pcm), 
                                     encoded_data, sizeof(encoded_data));

// 创建 Opus 解码器
audio_codec_t* decoder = opus_codec_create_decoder(16000, 1);

// 解码音频
uint8_t decoded_pcm[320];
size_t decoded_size = decoder->decode(decoder, encoded_data, encoded_size,
                                     decoded_pcm, sizeof(decoded_pcm));
```

## 📖 API 参考

### 核心 API

#### SDK 管理
- `LinxSdk* linx_sdk_create(const LinxSdkConfig* config)` - 创建 SDK 实例
- `void linx_sdk_destroy(LinxSdk* sdk)` - 销毁 SDK 实例
- `LinxSdkError linx_sdk_set_event_callback(LinxSdk* sdk, LinxEventCallback callback, void* user_data)` - 设置事件回调

#### 连接管理
- `LinxSdkError linx_sdk_connect(LinxSdk* sdk)` - 连接到服务器
- `LinxSdkError linx_sdk_disconnect(LinxSdk* sdk)` - 断开连接
- `bool linx_sdk_is_connected(LinxSdk* sdk)` - 检查连接状态

#### 消息发送
- `LinxSdkError linx_sdk_send_text(LinxSdk* sdk, const char* text)` - 发送文本消息
- `LinxSdkError linx_sdk_send_audio(LinxSdk* sdk, const uint8_t* data, size_t size)` - 发送音频数据
- `LinxSdkError linx_sdk_send_wake_word(LinxSdk* sdk, const char* wake_word)` - 发送唤醒词

#### 状态查询
- `LinxDeviceState linx_sdk_get_state(LinxSdk* sdk)` - 获取当前状态
- `const char* linx_sdk_get_session_id(LinxSdk* sdk)` - 获取会话 ID
- `uint32_t linx_sdk_get_message_count(LinxSdk* sdk)` - 获取消息计数
- `time_t linx_sdk_get_connect_time(LinxSdk* sdk)` - 获取连接时间

#### 控制操作
- `LinxSdkError linx_sdk_abort_speaking(LinxSdk* sdk, linx_abort_reason_t reason)` - 中断语音播放
- `LinxSdkError linx_sdk_poll_events(LinxSdk* sdk, int timeout_ms)` - 轮询事件

#### MCP 工具
- `LinxSdkError linx_sdk_add_mcp_tool(LinxSdk* sdk, const char* name, const char* description, mcp_property_list_t* properties, mcp_tool_callback_t callback)` - 添加 MCP 工具

### 事件类型

| 事件类型 | 描述 | 数据结构 |
|---------|------|----------|
| `LINX_EVENT_STATE_CHANGED` | 状态变化 | `state_changed` |
| `LINX_EVENT_TEXT_MESSAGE` | 文本消息 | `text_message` |
| `LINX_EVENT_AUDIO_DATA` | 音频数据 | `audio_data` |
| `LINX_EVENT_ERROR` | 错误事件 | `error` |
| `LINX_EVENT_WEBSOCKET_CONNECTED` | WebSocket 连接 | - |
| `LINX_EVENT_WEBSOCKET_DISCONNECTED` | WebSocket 断开 | - |
| `LINX_EVENT_SESSION_ESTABLISHED` | 会话建立 | `session_established` |
| `LINX_EVENT_SESSION_ENDED` | 会话结束 | - |
| `LINX_EVENT_LISTENING_STARTED` | 开始监听 | - |
| `LINX_EVENT_LISTENING_STOPPED` | 停止监听 | - |
| `LINX_EVENT_TTS_STARTED` | TTS 开始 | - |
| `LINX_EVENT_TTS_STOPPED` | TTS 停止 | - |
| `LINX_EVENT_MCP_MESSAGE` | MCP 消息 | `mcp_message` |

### 错误码

| 错误码 | 描述 |
|--------|------|
| `LINX_SDK_SUCCESS` | 成功 |
| `LINX_SDK_ERROR_INVALID_PARAM` | 无效参数 |
| `LINX_SDK_ERROR_NOT_INITIALIZED` | 未初始化 |
| `LINX_SDK_ERROR_NETWORK` | 网络错误 |
| `LINX_SDK_ERROR_WEBSOCKET` | WebSocket 错误 |
| `LINX_SDK_ERROR_MEMORY` | 内存错误 |
| `LINX_SDK_ERROR_UNKNOWN` | 未知错误 |

## 🏗️ 项目结构

```
linx-os-sdk/
├── README.md                    # 项目说明文档
├── .gitignore                   # Git 忽略文件
├── .vscode/                     # VS Code 配置
│   └── settings.json
├── demo/                        # 演示程序
│   └── mac/                     # macOS 演示
│       ├── CMakeLists.txt       # 演示程序构建配置
│       ├── README.md            # 演示程序说明
│       └── linx_demo.c          # 演示程序源码
└── sdk/                         # SDK 核心代码
    ├── CMakeLists.txt           # 主构建配置
    ├── run.sh                   # 构建脚本
    ├── linx_sdk.h               # SDK 主头文件
    ├── linx_sdk.c               # SDK 主实现
    ├── audio/                   # 音频模块
    │   ├── CMakeLists.txt
    │   ├── README.md
    │   ├── audio_interface.h    # 音频接口定义
    │   ├── audio_interface.c    # 音频接口实现
    │   ├── portaudio_mac.h      # macOS PortAudio 实现
    │   ├── portaudio_mac.c
    │   └── test/                # 音频模块测试
    ├── codecs/                  # 编解码模块
    │   ├── CMakeLists.txt
    │   ├── README.md
    │   ├── audio_codec.h        # 编解码器接口
    │   ├── audio_codec.c
    │   ├── opus_codec.h         # Opus 编解码器
    │   ├── opus_codec.c
    │   └── test/                # 编解码器测试
    ├── protocols/               # 协议模块
    │   ├── CMakeLists.txt
    │   ├── linx_protocol.h      # LinX 协议定义
    │   ├── linx_protocol.c
    │   ├── linx_websocket.h     # WebSocket 实现
    │   ├── linx_websocket.c
    │   └── test/                # 协议测试
    ├── mcp/                     # MCP 模块
    │   ├── CMakeLists.txt
    │   ├── mcp.h                # MCP 主头文件
    │   ├── mcp_server.h         # MCP 服务器
    │   ├── mcp_server.c
    │   ├── mcp_tool.h           # MCP 工具
    │   ├── mcp_tool.c
    │   ├── mcp_property.h       # MCP 属性
    │   ├── mcp_property.c
    │   └── test/                # MCP 测试
    ├── log/                     # 日志模块
    │   ├── CMakeLists.txt
    │   ├── linx_log.h           # 日志接口
    │   └── linx_log.c           # 日志实现
    ├── cjson/                   # JSON 模块
    │   ├── CMakeLists.txt
    │   ├── cJSON.h              # cJSON 头文件
    │   ├── cJSON.c              # cJSON 实现
    │   ├── cJSON_Utils.h
    │   └── cJSON_Utils.c
    ├── cmake/                   # CMake 配置
    │   ├── FindMongoose.cmake   # Mongoose 查找脚本
    │   ├── FindOpus.cmake       # Opus 查找脚本
    │   ├── PlatformDetection.cmake # 平台检测
    │   ├── platforms/           # 平台特定配置
    │   ├── toolchains/          # 工具链配置
    │   │   ├── allwinner-aarch64.cmake    # 全志 ARM64 工具链
    │   │   ├── allwinner-armv7.cmake      # 全志 ARM32 工具链
    │   │   ├── allwinner-tina.cmake       # 全志 Tina SDK 工具链
    │   │   ├── riscv32-linux-musl.cmake   # RISC-V 32位工具链
    │   │   ├── riscv64-linux-musl.cmake   # RISC-V 64位工具链
    │   │   ├── esp32.cmake                # ESP32 工具链
    │   │   └── arm-linux-gnueabihf.cmake  # ARM Linux 工具链
    │   └── templates/           # 模板文件
    └── third/                   # 第三方库
        ├── mongoose/            # Mongoose WebSocket 库
        └── opus/                # Opus 音频编解码库
```

## 🔧 高级配置

### 跨平台编译

#### ESP32 平台
```bash
# 设置 ESP-IDF 环境
source $IDF_PATH/export.sh

# 使用 ESP32 工具链编译
cd sdk
./run.sh --toolchain cmake/toolchains/esp32.cmake
```

#### 全志芯片平台

##### 全志 A64/H5/H6 系列 (ARM64)
```bash
# 1. 安装工具链
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# 2. 设置环境变量
export ALLWINNER_TOOLCHAIN_PATH="/usr/bin"
export ALLWINNER_SYSROOT="/usr/aarch64-linux-gnu"

# 3. 编译 SDK
cd sdk
./run.sh --toolchain cmake/toolchains/allwinner-aarch64.cmake

# 4. 验证编译结果
file build/install/lib/liblinx_sdk.a
# 输出应显示: ELF 64-bit LSB relocatable, ARM aarch64
```

##### 全志 H3/H2+ 系列 (ARM32)
```bash
# 1. 安装工具链
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# 2. 设置环境变量
export ALLWINNER_ARM32_TOOLCHAIN_PATH="/usr/bin"
export ALLWINNER_ARM32_SYSROOT="/usr/arm-linux-gnueabihf"

# 3. 编译 SDK
./run.sh --toolchain cmake/toolchains/allwinner-armv7.cmake

# 4. 验证编译结果
file build/install/lib/liblinx_sdk.a
# 输出应显示: ELF 32-bit LSB relocatable, ARM
```

##### 使用全志官方 SDK
```bash
# 1. 下载全志 SDK
git clone https://github.com/allwinner/tina-v83x.git
cd tina-v83x

# 2. 初始化环境
source build/envsetup.sh
lunch tina_v83x-eng

# 3. 设置工具链路径
export ALLWINNER_TOOLCHAIN_PATH="$PWD/prebuilt/gcc/linux-x86/aarch64/toolchain-sunxi-musl/toolchain/bin"

# 4. 编译 LinX SDK
cd /path/to/linx-os-sdk/sdk
./run.sh --toolchain cmake/toolchains/allwinner-tina.cmake
```

#### ARM Linux 平台
```bash
# 使用 ARM 工具链编译
./run.sh --toolchain cmake/toolchains/arm-linux-gnueabihf.cmake
```

#### RISC-V 平台

##### 通用 RISC-V 工具链
```bash
# 设置 RISC-V 工具链路径
export RISCV32_TOOLCHAIN_PATH="/opt/riscv32-linux-musl"

# 编译 RISC-V 版本
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake
```

##### 全志 V821 RISC-V 平台
```bash
# 1. 下载并解压工具链
# 链接: https://pan.baidu.com/s/1f-xLwrOjHntsW4LyO1KKWw 提取码: 5ser
tar -xf nds32le-linux-musl-v5d.tar.xz

# 2. 设置工具链环境
export ALLWINNER_V821_TOOLCHAIN_PATH="/path/to/nds32le-linux-musl-v5d"
export PATH="$ALLWINNER_V821_TOOLCHAIN_PATH/bin:$PATH"

# 3. 设置编译选项
export V821_CFLAGS="-g -ggdb -Wall -O3 -march=rv32imfdcxandes -mabi=ilp32d -mcmodel=medany"
export V821_CXXFLAGS="$V821_CFLAGS"

# 4. 验证工具链
riscv32-linux-musl-gcc --version
riscv32-linux-musl-g++ --version

# 5. 编译 V821 版本
cd sdk
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake

# 6. 验证编译结果
file build/install/lib/liblinx_sdk.a
# 输出应显示: ELF 32-bit LSB relocatable, UCB RISC-V

# 7. 编译特定模块（可选）
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake --target audio_codec
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake --target mcp_tools

# 8. 清理并重新编译（如果需要）
make clean
./run.sh --toolchain cmake/toolchains/riscv32-linux-musl.cmake
```

### 自定义配置

#### 音频配置
```c
LinxSdkConfig config = {
    .sample_rate = 48000,        // 高质量音频
    .channels = 2,               // 立体声
    .timeout_ms = 10000,         // 10秒超时
    .listening_mode = LINX_LISTENING_PUSH_TO_TALK  // 按键说话模式
};
```

#### 日志配置
```c
#include "log/linx_log.h"

log_config_t log_config = {
    .level = LOG_LEVEL_DEBUG,    // 调试级别
    .enable_timestamp = true,    // 启用时间戳
    .enable_color = true,        // 启用颜色输出
    .output_file = "linx.log"    // 输出到文件
};
log_init(&log_config);
```

## 🧪 测试

### 运行单元测试
```bash
# 构建并运行所有测试
cd sdk/build
make test

# 运行特定模块测试
./audio/test/audio_test
./codecs/test/codec_test
./protocols/test/protocol_test
./mcp/test/mcp_test
```

### 性能测试
```bash
# 音频延迟测试
./audio/test/latency_test

# 编解码性能测试
./codecs/test/performance_test

# 网络性能测试
./protocols/test/network_test
```

## 🐛 故障排除

### 常见问题

#### 1. 编译错误
```bash
# 确保安装了所有依赖
brew install cmake portaudio pkg-config  # macOS
sudo apt install build-essential cmake libasound2-dev  # Ubuntu

# 清理并重新构建
rm -rf build
./run.sh
```

#### 2. 音频设备问题
```bash
# 检查音频设备
./demo/mac/build/linx_demo --list-devices

# 设置默认音频设备
export LINX_AUDIO_DEVICE="Built-in Microphone"
```

#### 3. 网络连接问题
```bash
# 检查网络连接
ping your-server.com

# 使用调试模式
export LINX_LOG_LEVEL=DEBUG
./linx_demo -s ws://your-server.com/v1/ws/
```

#### 4. 权限问题
```bash
# macOS 麦克风权限
# 系统偏好设置 -> 安全性与隐私 -> 隐私 -> 麦克风

# Linux 音频权限
sudo usermod -a -G audio $USER
```

#### 5. 全志芯片编译问题
```bash
# 工具链路径错误
export ALLWINNER_TOOLCHAIN_PATH="/usr/bin"
export PATH="/usr/bin:$PATH"

# 缺少系统库
sudo apt install libc6-dev-arm64-cross

# 验证工具链
aarch64-linux-gnu-gcc --version

# 清理并重新编译
rm -rf build
./run.sh --toolchain cmake/toolchains/allwinner-aarch64.cmake
```

#### 6. RISC-V 编译问题
```bash
# 工具链未找到
export RISCV32_TOOLCHAIN_PATH="/opt/riscv32-linux-musl"
export PATH="$RISCV32_TOOLCHAIN_PATH/bin:$PATH"

# 验证 RISC-V 工具链
riscv32-linux-musl-gcc --version

# 检查目标架构
readelf -h build/install/lib/liblinx_sdk.a | grep Machine
```

### 调试技巧

#### 启用详细日志
```c
log_config_t config = LOG_DEFAULT_CONFIG;
config.level = LOG_LEVEL_TRACE;  // 最详细的日志
log_init(&config);
```

#### 网络调试
```bash
# 使用 Wireshark 抓包分析 WebSocket 通信
# 或使用 tcpdump
sudo tcpdump -i any -w linx_traffic.pcap port 80
```

## 🤝 贡献指南

我们欢迎社区贡献！请遵循以下步骤：

1. **Fork** 项目
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 **Pull Request**

### 代码规范

- 使用 C99 标准
- 遵循 Linux 内核代码风格
- 添加适当的注释和文档
- 编写单元测试

### 提交信息格式
```
type(scope): description

[optional body]

[optional footer]
```

类型：
- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码格式
- `refactor`: 重构
- `test`: 测试
- `chore`: 构建/工具

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- [PortAudio](http://www.portaudio.com/) - 跨平台音频 I/O 库
- [Opus](https://opus-codec.org/) - 高质量音频编解码器
- [Mongoose](https://mongoose.ws/) - 嵌入式 Web 服务器和网络库
- [cJSON](https://github.com/DaveGamble/cJSON) - 轻量级 JSON 解析器

## 📞 支持

- 📧 邮箱: support@linx-os.com
- 💬 讨论: [GitHub Discussions](https://github.com/your-org/linx-os-sdk/discussions)
- 🐛 问题报告: [GitHub Issues](https://github.com/your-org/linx-os-sdk/issues)
- 📖 文档: [在线文档](https://docs.linx-os.com)

---

<div align="center">

**[⬆ 回到顶部](#linx-os-sdk)**

Made with ❤️ by the LinX OS Team

</div>