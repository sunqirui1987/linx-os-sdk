# LINX 音频测试套件

这是LINX音频接口的测试套件，用于验证音频接口的功能和性能。

## 功能特性

- **音频桩测试**: 测试音频桩实现的基本功能
- **基本音频测试**: 测试PortAudio Mac实现的基本功能
- **交互式测试**: 实时录制和播放测试（回声测试）
- **完整的接口覆盖**: 测试所有新的音频接口方法

## 测试内容

### 1. 音频桩测试 (`test_audio_stub`)
- 创建和销毁音频桩
- 初始化和配置测试
- 数据输入输出测试
- 状态管理测试

### 2. 基本音频测试 (`test_audio_basic`)
- PortAudio Mac实现的创建和销毁
- 音频接口初始化
- 配置设置验证
- 音量控制测试
- 输入输出状态测试
- Getter函数测试

### 3. 交互式测试 (`test_audio_record_play`)
- 实时音频录制和播放
- 回声测试（麦克风输入通过扬声器播放）
- 错误统计和性能监控
- 用户交互控制

## 依赖要求

### 必需依赖
- CMake 3.16+
- Make
- GCC 或 Clang（支持C99）
- pthread库

### 可选依赖
- PortAudio 2.0（用于真实音频硬件测试）
  ```bash
  # macOS安装
  brew install portaudio
  
  # Ubuntu/Debian安装
  sudo apt-get install libportaudio2 libportaudio-dev
  ```

## 构建和运行

### 方法1: 使用构建脚本（推荐）

```bash
# 进入测试目录
cd src/audio/audio/test

# 完整构建和测试
./build_and_test.sh

# 仅编译
./build_and_test.sh --build-only

# 运行交互式测试
./build_and_test.sh --interactive

# 清理构建目录
./build_and_test.sh --clean

# 查看帮助
./build_and_test.sh --help
```

### 方法2: 使用CMake

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make

# 运行基本测试
./bin/audio_test

# 运行交互式测试
./bin/audio_test --interactive

# 运行CMake测试
make test
```

## 测试输出示例

### 基本测试输出
```
=== LINX 音频测试套件 ===

1. 运行音频桩测试...
测试音频桩实现...
✓ 音频桩创建成功
✓ 音频桩初始化成功
✓ 音频桩配置设置完成
✓ 音频桩启动成功
✓ 音频桩输出数据测试成功
✓ 音频桩输入数据测试成功
✓ 音频桩销毁成功
音频桩测试成功完成

2. 运行基本音频测试...
测试基本音频接口功能...
✓ 音频接口创建成功
✓ 音频接口初始化成功
✓ 音频配置设置正确
✓ 音频接口启动成功
✓ 音量控制功能正常
✓ 输入输出启用状态正常
音频接口属性:
  支持全双工: 是
  输入采样率: 16000 Hz
  输出采样率: 16000 Hz
  输入通道数: 1
  输出通道数: 1
✓ 音频接口销毁成功
基本音频测试成功完成

=== 所有测试完成 ===
```

### 交互式测试输出
```
3. 运行交互式音频测试...
请确保你已连接麦克风和扬声器/耳机
按回车键继续或Ctrl+C跳过...

测试音频录制和播放功能...
✓ 音频接口创建成功
✓ 音频接口初始化成功
音频配置:
  采样率: 16000 Hz
  通道数: 1
  帧大小: 1024
  缓冲区大小: 8192
启动音频接口...
✓ 音频接口启动成功
输出音量设置为: 50%
开始录制和播放音频... 按Ctrl+C停止
你应该能听到麦克风输入通过扬声器/耳机播放出来
已处理 100 帧
已处理 200 帧
...
正在停止音频测试...
总共处理了 1523 帧
读取错误: 0 次，写入错误: 0 次
音频测试成功完成
```

## 故障排除

### 常见问题

1. **PortAudio未找到**
   ```
   未找到 PortAudio，将使用桩实现
   要安装 PortAudio，请运行: brew install portaudio
   ```
   解决方案：安装PortAudio库

2. **音频设备权限问题**
   ```
   初始化音频接口失败，错误码: -4
   ```
   解决方案：检查系统音频设备权限设置

3. **编译错误**
   ```
   fatal error: 'portaudio.h' file not found
   ```
   解决方案：确保PortAudio正确安装或使用桩实现

### 调试模式

要启用详细的调试输出，可以修改测试代码中的日志级别：

```c
log_config.level = LOG_LEVEL_DEBUG;  // 改为DEBUG级别
```

## 接口对齐说明

本测试套件完全对齐了AudioCodec的接口设计：

- ✅ `SetOutputVolume()` → `audio_interface_set_output_volume()`
- ✅ `EnableInput()` → `audio_interface_enable_input()`
- ✅ `EnableOutput()` → `audio_interface_enable_output()`
- ✅ `OutputData()` → `audio_interface_output_data()`
- ✅ `InputData()` → `audio_interface_input_data()`
- ✅ `Start()` → `audio_interface_start()`
- ✅ 所有getter方法都已实现

## 贡献

如果你发现问题或想要改进测试，请：

1. 确保所有现有测试通过
2. 为新功能添加相应的测试
3. 更新文档说明
4. 遵循现有的代码风格和注释规范

## 许可证

本项目遵循LINX项目的许可证条款。