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
- **⚙️ 统一构建系统**: 提供 linxos.py 配置选择界面，一键选择平台和工具链
- **🏗️ 智能编译**: 自动检测工具链，支持 SDK → Board → Examples 分层编译



### 🏗️ 架构概览

```
LinX OS SDK 分层架构
├── ⚙️ 构建配置层 (Build Configuration)
│   ├── linxos.py 配置选择界面
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

#### 2. 配置构建选项

LinX OS SDK 提供了统一的配置界面，让您可以轻松选择目标平台、工具链和编译选项：

```bash
# 启动配置选择界面
./linxos.py config choice
```

配置选择界面显示可用的预设配置：

```
LinX OS SDK 配置选择:
============================================================
当前配置:
  配置名称: 当前配置
  目标平台: native
  板框平台: mac
  构建类型: Release
  工具链文件: 
  SDK状态: 已编译
  Board状态: 已编译

可用配置选项:
------------------------------------------------------------
   1. LN882H          - LN882H WiFi芯片开发板
      平台: ln882h     板框: ln882h
   2. Ubuntu          - Ubuntu Linux 本地开发环境
      平台: native     板框: ubuntu
   3. ESP32           - ESP32 WiFi+蓝牙微控制器
      平台: esp32      板框: esp32
   4. macOS           - macOS 本地开发环境
      平台: native     板框: mac
   5. RISC-V32        - RISC-V 32位嵌入式Linux系统
      平台: riscv32    板框: generic
------------------------------------------------------------
输入 "q" 退出配置选择

请选择配置 (1-5, 回车保持当前配置):
```

#### 3. 一键构建

配置完成后，使用统一的构建命令：

```bash
# 构建整个项目 (SDK + Board + Examples)
./linxos.py build all

# 或者分步构建
./linxos.py build sdk          # 仅构建 SDK
./linxos.py build board        # 构建板级支持
./linxos.py build examples     # 构建示例程序
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
./linxos.py config preset native      # 本地开发配置
./linxos.py config preset riscv32     # RISC-V 32位配置
./linxos.py config preset esp32       # ESP32 配置
./linxos.py config preset allwinner   # 全志芯片配置

# 查看所有可用预设
./linxos.py config list
```
