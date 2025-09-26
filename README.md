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
- **⚙️ 统一构建系统**: 提供 menuconfig 配置界面，一键选择平台和工具链
- **🏗️ 智能编译**: 自动检测工具链，支持 SDK → Board → Examples 分层编译



### 🏗️ 架构概览

```
LinX OS SDK 分层架构
├── ⚙️ 构建配置层 (Build Configuration)
│   ├── menuconfig 配置界面
│   ├── 平台和工具链选择
│   └── 预设配置管理
├── 🏗️ SDK 核心层 (Core SDK)
│   ├── 🎵 Audio Module        # 音频录制和播放
│   ├── 🎛️ Codecs Module       # 音频编解码 (Opus)
│   ├── 🌐 Protocols Module    # WebSocket 通信协议
│   ├── 🔧 MCP Module          # Model Context Protocol
│   ├── 📝 Log Module          # 日志系统
│   └── 📊 JSON Module         # JSON 数据处理
├── 🔌 Board 适配层 (Board Adaptation)
│   ├── macOS 开发板适配
│   ├── 全志 AWOL 板适配
│   ├── ESP32 开发板适配
│   └── 自定义板级支持
└── 📱 应用示例层 (Examples & Apps)
    ├── 基础功能演示
    ├── 平台特定示例
    └── 完整应用程序
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

#### 2. 配置构建选项 (menuconfig)

LinX OS SDK 提供了统一的配置界面，让您可以轻松选择目标平台、工具链和编译选项：

```bash
# 启动配置界面
make menuconfig
```

配置界面包含以下选项：

```
┌─────────────────── LinX OS SDK Configuration ───────────────────┐
│                                                                  │
│ Target Platform Selection                                        │
│   ● Native (Host Platform)                                      │
│   ○ ARM Linux (Embedded)                                        │
│   ○ RISC-V (32-bit)                                            │
│   ○ RISC-V (64-bit)                                            │
│   ○ ESP32 (IoT Platform)                                        │
│   ○ Allwinner A64/H5/H6 (ARM64)                                │
│   ○ Allwinner H3/H2+ (ARM32)                                   │
│   ○ Allwinner V821 (RISC-V)                                    │
│                                                                  │
│ Board Platform Selection                                         │
│   ● Generic                                                     │
│   ○ macOS Development Board                                     │
│   ○ Allwinner AWOL Board                                       │
│   ○ ESP32 DevKit                                               │
│   ○ Custom Board                                               │
│                                                                  │
│ Toolchain Configuration                                          │
│   Toolchain Path: [/opt/toolchain]                             │
│   Sysroot Path:   [/opt/sysroot]                               │
│   Custom CFLAGS:  [-O2 -g]                                     │
│                                                                  │
│ Build Options                                                    │
│   [*] Enable Debug Build                                        │
│   [*] Build Examples                                            │
│   [*] Build Tests                                               │
│   [ ] Enable Static Linking                                     │
│                                                                  │
│ Audio Configuration                                              │
│   [*] Enable Opus Codec                                         │
│   [*] Enable PortAudio                                          │
│   Sample Rate: [16000]                                          │
│                                                                  │
│ Network Configuration                                            │
│   [*] Enable WebSocket Support                                  │
│   [*] Enable SSL/TLS                                            │
│   Default Server: [ws://localhost:8080/v1/ws/]                  │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

#### 3. 一键构建

配置完成后，使用统一的构建命令：

```bash
# 构建整个项目 (SDK + Board + Examples)
make all

# 或者分步构建
make sdk          # 仅构建 SDK
make board        # 构建板级支持
make examples     # 构建示例程序
```

构建系统会自动：
- 根据配置选择合适的工具链
- 下载并编译第三方依赖 (Mongoose, Opus)
- 构建 SDK 核心模块
- 编译板级适配代码
- 生成静态库和头文件
- 构建示例程序和测试用例
- 安装到 `build/install` 目录

#### 4. 快速配置预设

为了方便使用，我们提供了常用平台的预设配置：

```bash
# 使用预设配置
make config-native      # 本地开发配置
make config-riscv32     # RISC-V 32位配置
make config-esp32       # ESP32 配置
make config-allwinner   # 全志芯片配置

# 查看所有可用预设
make list-configs
```

#### 5. 高级构建选项

```bash
# 清理构建文件
make clean

# 完全清理 (包括配置)
make distclean

# 仅重新配置
make reconfig

# 显示构建信息
make info

# 并行构建 (使用多核)
make -j$(nproc)

# 详细构建日志
make VERBOSE=1
```

### 🎯 运行演示

构建完成后，可以运行相应平台的演示程序：

```bash
# 运行演示程序 (根据配置的平台自动选择)
make run

# 或者直接运行可执行文件
./build/examples/linx_demo

# 指定服务器地址
./build/examples/linx_demo -s ws://your-server.com/v1/ws/

# 查看帮助
./build/examples/linx_demo --help
```

#### 平台特定运行方式

##### macOS/Linux 平台
```bash
# 直接运行
./build/examples/linx_demo

# 使用调试模式
./build/examples/linx_demo --debug --log-level=debug
```

##### ESP32 平台
```bash
# 烧录到设备
make flash

# 监控串口输出
make monitor

# 烧录并监控
make flash monitor
```

##### 全志平台
```bash
# 复制到目标设备
scp build/examples/linx_demo root@target-device:/usr/bin/

# 在目标设备上运行
ssh root@target-device
/usr/bin/linx_demo --config /etc/linx/config.json
```

## 🔧 编译工具链集成

LinX OS SDK 提供了完整的跨平台编译工具链支持，通过统一的 menuconfig 配置界面和自动化构建系统，大大简化了跨平台编译的复杂性。SDK 支持多种架构和操作系统，包括嵌入式设备和桌面系统。

### 🛠️ 统一构建系统架构

```
LinX OS SDK 构建系统
├── 📋 配置层 (Configuration Layer)
│   ├── menuconfig 界面配置
│   ├── 预设配置文件
│   └── 环境变量检测
├── 🔧 工具链管理 (Toolchain Management)
│   ├── 自动工具链检测
│   ├── 工具链路径配置
│   └── 交叉编译环境设置
├── 🏗️ 构建引擎 (Build Engine)
│   ├── SDK 核心编译
│   ├── Board 平台适配
│   └── 示例程序构建
└── 📦 输出管理 (Output Management)
    ├── 库文件生成
    ├── 头文件安装
    └── 示例程序打包
```

### 🎯 支持的目标平台

| 平台类别 | 目标平台 | 架构 | 工具链 | menuconfig 选项 |
|---------|---------|------|--------|----------------|
| **桌面平台** | Native Host | x86_64/arm64 | GCC/Clang | `Native (Host Platform)` |
| **ARM 嵌入式** | ARM Linux | armv7/armv8 | arm-linux-gnueabihf | `ARM Linux (Embedded)` |
| **RISC-V** | RISC-V 32位 | riscv32 | riscv32-linux-musl | `RISC-V (32-bit)` |
| **RISC-V** | RISC-V 64位 | riscv64 | riscv64-linux-musl | `RISC-V (64-bit)` |
| **物联网** | ESP32 | xtensa | esp-idf | `ESP32 (IoT Platform)` |
| **全志芯片** | A64/H5/H6 | aarch64 | aarch64-linux-gnu | `Allwinner A64/H5/H6 (ARM64)` |
| **全志芯片** | H3/H2+ | armv7 | arm-linux-gnueabihf | `Allwinner H3/H2+ (ARM32)` |
| **全志芯片** | V821 | riscv32 | nds32le-linux-musl-v5d | `Allwinner V821 (RISC-V)` |

### 🔧 工具链自动配置

通过 menuconfig 配置界面，工具链配置变得非常简单。系统会自动检测和配置工具链，无需手动设置复杂的环境变量。

#### 1. 自动工具链检测

```bash
# 启动配置界面
make menuconfig

# 系统会自动检测以下工具链：
# ✓ 检测到 GCC 工具链: /usr/bin/gcc
# ✓ 检测到 ARM 工具链: /opt/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
# ✓ 检测到 RISC-V 工具链: /opt/riscv32/bin/riscv32-linux-musl-gcc
# ✓ 检测到 ESP-IDF: /opt/esp-idf
```

#### 2. 工具链路径配置

如果系统未自动检测到工具链，可以在 menuconfig 中手动指定：

```
Toolchain Configuration
├── Toolchain Path: [/opt/your-toolchain]
├── Sysroot Path:   [/opt/your-sysroot]
├── Custom CFLAGS:  [-O2 -g -march=native]
└── Custom LDFLAGS: [-static]
```

#### 3. 常用工具链安装

##### RISC-V 工具链
```bash
# Ubuntu/Debian
sudo apt install gcc-riscv64-linux-gnu

# 或下载预编译工具链
wget https://github.com/riscv/riscv-gnu-toolchain/releases/download/...
tar -xf riscv32-linux-musl.tar.xz -C /opt/
```

##### ARM 工具链
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu

# CentOS/RHEL
sudo yum install gcc-arm-linux-gnu gcc-aarch64-linux-gnu
```

##### 全志V821专用工具链
```bash
# 下载全志V821工具链
# 链接: https://pan.baidu.com/s/1f-xLwrOjHntsW4LyO1KKWw 提取码: 5ser
wget -O nds32le-linux-musl-v5d.tar.xz "https://..."
tar -xf nds32le-linux-musl-v5d.tar.xz -C /opt/

# 在 menuconfig 中设置路径为: /opt/nds32le-linux-musl-v5d
```

##### ESP32 工具链
```bash
# 安装 ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
source export.sh

# menuconfig 会自动检测 ESP-IDF 环境
```

### 🏗️ Board 平台配置

LinX OS SDK 支持多种板级平台，每个平台都有特定的硬件适配代码和配置。通过 menuconfig 可以轻松选择和配置目标板级平台。

#### 📋 支持的 Board 平台

| Board 平台 | 描述 | 支持芯片 | 特性 |
|-----------|------|---------|------|
| **Generic** | 通用平台 | 所有支持的芯片 | 基础功能，适用于大多数场景 |
| **macOS Development Board** | macOS 开发板 | x86_64/arm64 | 完整开发环境，支持所有功能 |
| **Allwinner AWOL Board** | 全志 AWOL 开发板 | V821/A64/H5/H6 | 全志专用硬件适配 |
| **ESP32 DevKit** | ESP32 开发套件 | ESP32/ESP32-S3 | 物联网功能，低功耗设计 |
| **Custom Board** | 自定义板级 | 用户定义 | 支持用户自定义硬件配置 |

#### 🔧 Board 平台选择

在 menuconfig 中选择 Board 平台：

```
Board Platform Selection
├── ● Generic                    # 通用平台，适用于大多数场景
├── ○ macOS Development Board    # macOS 开发环境
├── ○ Allwinner AWOL Board      # 全志 AWOL 开发板
├── ○ ESP32 DevKit              # ESP32 开发套件
└── ○ Custom Board              # 自定义板级平台
```

#### 📦 Board 特定配置

每个 Board 平台都有特定的配置选项：

##### macOS Development Board
```
macOS Board Configuration
├── Audio Backend: [PortAudio]
├── Network Interface: [WiFi/Ethernet]
├── Debug Interface: [USB/Serial]
└── Power Management: [Disabled]
```

##### Allwinner AWOL Board
```
Allwinner AWOL Board Configuration
├── Audio Codec: [AC108/ES8388]
├── Display Output: [HDMI/MIPI-DSI]
├── Network Interface: [Ethernet/WiFi]
├── Storage: [eMMC/SD Card]
└── GPIO Configuration: [Custom]
```

##### ESP32 DevKit
```
ESP32 DevKit Configuration
├── Audio Codec: [I2S/PDM]
├── WiFi Configuration: [STA/AP Mode]
├── Bluetooth: [Classic/BLE]
├── Power Management: [Deep Sleep]
└── OTA Update: [Enabled]
```

#### 🏗️ Board 编译流程

Board 平台的编译是在 SDK 编译完成后进行的：

```bash
# 1. 配置 Board 平台
make menuconfig
# 选择目标 Board 平台

# 2. 编译 SDK (如果尚未编译)
make sdk

# 3. 编译 Board 适配代码
make board

# 4. 编译示例程序 (可选)
make examples

# 或者一键编译所有组件
make all
```

#### 📁 Board 目录结构

```
board/
├── mac/                        # macOS 开发板
│   ├── CMakeLists.txt          # 构建配置
│   ├── common/                 # 通用代码
│   │   └── audio/              # 音频适配
│   └── build/                  # 构建输出
├── awol/                       # 全志 AWOL 开发板
│   ├── common/                 # 通用代码
│   │   └── audio/              # 音频适配
│   └── v821/                   # V821 特定代码
└── esp32/                      # ESP32 开发板 (待添加)
    ├── main/                   # 主程序
    ├── components/             # 组件
    └── sdkconfig               # ESP-IDF 配置
```

#### 🔧 自定义 Board 平台

如果需要支持新的硬件平台，可以创建自定义 Board：

```bash
# 1. 创建 Board 目录
mkdir -p board/my-custom-board/{common,specific}

# 2. 复制模板文件
cp board/mac/CMakeLists.txt board/my-custom-board/
cp -r board/mac/common/* board/my-custom-board/common/

# 3. 修改适配代码
# 编辑 board/my-custom-board/common/audio/ 中的音频适配代码
# 根据硬件特性修改 CMakeLists.txt

# 4. 在 menuconfig 中添加选项
# 编辑构建系统配置文件，添加新的 Board 选项
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