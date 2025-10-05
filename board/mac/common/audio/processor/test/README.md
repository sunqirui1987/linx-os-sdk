# Mac音频处理器测试

本目录包含Mac平台音频处理器的测试程序。

## 文件说明

- `audio_processor_mac_test.c` - Mac音频处理器测试主程序
- `CMakeLists.txt` - CMake构建配置文件
- `build_and_test.sh` - 自动构建脚本

## 构建说明

### 使用构建脚本（推荐）

```bash
cd /path/to/linx-os-sdk/src/audio/processor/test
./build_and_test.sh
```

### 手动构建

```bash
cd /path/to/linx-os-sdk/src/audio/processor/test
mkdir build
cd build
cmake ..
make
```

## 运行测试

构建完成后，可以运行测试程序：

```bash
cd build
./audio_processor_mac_test
```

## 测试内容

测试程序将执行以下操作：

1. 创建Mac音频处理器实例
2. 使用默认配置初始化处理器（16kHz采样率，1声道，60ms帧时长）
3. 启用所有音频处理功能（AEC、NS、VAD）
4. 生成测试音频数据并处理
5. 显示处理结果和VAD状态
6. 清理资源

## 依赖项

- macOS 10.12 或更高版本
- CMake 3.10 或更高版本
- Xcode命令行工具
- CoreAudio框架
- AudioToolbox框架
- AudioUnit框架

## 输出示例

```
Mac音频处理器测试
==================
✓ 成功创建Mac音频处理器
配置参数:
  采样率: 16000 Hz
  声道数: 1
  帧时长: 60 ms
  启用AEC: 是
  启用NS: 是
  启用VAD: 是
✓ 成功初始化音频处理器
✓ 设置回调函数
✓ 成功启动音频处理器
输入数据大小: 960 样本

开始处理音频数据...
输出回调: 处理了 960 个样本
VAD状态变化: 静音
帧 0: VAD状态 = 静音
输出回调: 处理了 960 个样本
帧 1: VAD状态 = 静音
...

测试完成
```