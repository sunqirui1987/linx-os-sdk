# LINX Audio Module

这个模块提供了跨平台的音频录制和播放功能，目前实现了基于PortAudio的Mac版本。

## 功能特性

- 音频录制 (Record)
- 音频播放 (Play)
- 可配置的采样率、声道数、缓冲区大小等参数
- 基于C99标准，兼容性好
- 使用虚函数表实现多态，支持不同硬件平台
- 线程安全的环形缓冲区
- 集成日志系统

## 架构设计

```
AudioInterface (基类)
    ↓
├── PortAudioMac (Mac平台实现)
├── ESP32Audio (ESP32平台实现)
├── BroadcomAudio (博通芯片实现)
├── ALSALinux (Linux ALSA实现)
└── WASAPIWindows (Windows WASAPI实现)
```

- `AudioInterface`: 定义了统一的音频接口
- `PortAudioMac`: 基于PortAudio的Mac平台实现
- `ESP32Audio`: 基于ESP-IDF的ESP32平台实现
- `BroadcomAudio`: 博通芯片(如树莓派)的音频实现
- `ALSALinux`: Linux平台的ALSA音频实现
- `WASAPIWindows`: Windows平台的WASAPI音频实现

## 平台实现详解

### ESP32 音频播放实现

ESP32平台使用ESP-IDF框架的I2S接口实现音频播放功能。

#### 特性
- 支持I2S音频输出
- 支持内置DAC和外部DAC
- 支持多种采样率 (8kHz - 48kHz)
- 低功耗设计
- 支持DMA传输

#### 硬件连接
```
ESP32 引脚配置:
- I2S_BCLK: GPIO26 (位时钟)
- I2S_WS:   GPIO25 (字选择/帧同步)
- I2S_DOUT: GPIO22 (数据输出)
- 或使用内置DAC: GPIO25, GPIO26
```

#### 实现示例
```c
#include "audio/esp32_audio.h"

// 创建ESP32音频接口
AudioInterface* audio = esp32_audio_create();

// ESP32特定配置
esp32_audio_config_t config = {
    .i2s_port = I2S_NUM_0,
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .use_internal_dac = false,  // 使用外部DAC
    .dma_buf_count = 8,
    .dma_buf_len = 1024
};

esp32_audio_set_config(audio, &config);
audio_interface_init(audio);
```

#### 依赖安装
```bash
# 安装ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# 设置环境变量
. ./export.sh
```

### 博通芯片音频播放与录制实现

博通芯片(如树莓派的BCM2835/BCM2711)使用ALSA和专用的音频驱动实现。

#### 特性
- 支持模拟音频输出(3.5mm接口)
- 支持HDMI音频输出
- 支持USB音频设备
- 支持I2S音频接口
- 支持音频录制(通过USB或I2S)

#### 硬件配置
```bash
# 树莓派音频配置
# 启用音频接口
sudo raspi-config
# Advanced Options -> Audio -> Force 3.5mm

# 或编辑配置文件
echo "dtparam=audio=on" >> /boot/config.txt
echo "dtoverlay=hifiberry-dac" >> /boot/config.txt  # 可选：外部DAC
```

#### 实现示例
```c
#include "audio/broadcom_audio.h"

// 创建博通音频接口
AudioInterface* audio = broadcom_audio_create();

// 博通特定配置
broadcom_audio_config_t config = {
    .device_name = "hw:0,0",        // ALSA设备名
    .output_route = BCM_AUDIO_3_5MM, // 3.5mm输出
    .sample_rate = 16000,
    .channels = 1,
    .buffer_size = 4096,
    .period_size = 1024,
    .enable_recording = true
};

broadcom_audio_set_config(audio, &config);
audio_interface_init(audio);
```

#### 依赖安装
```bash
# 安装ALSA开发库
sudo apt-get update
sudo apt-get install libasound2-dev

# 安装博通特定库
sudo apt-get install libraspberrypi-dev
```

### Linux ALSA 实现

Linux平台使用ALSA(Advanced Linux Sound Architecture)实现音频功能。

#### 特性
- 支持所有ALSA兼容的音频设备
- 低延迟音频处理
- 支持多声道音频
- 支持音频混合
- 支持热插拔设备

#### 实现示例
```c
#include "audio/alsa_linux.h"

// 创建ALSA音频接口
AudioInterface* audio = alsa_linux_create();

// ALSA特定配置
alsa_config_t config = {
    .playback_device = "default",
    .capture_device = "default",
    .sample_rate = 16000,
    .channels = 1,
    .format = SND_PCM_FORMAT_S16_LE,
    .buffer_time = 500000,  // 500ms
    .period_time = 125000   // 125ms
};

alsa_linux_set_config(audio, &config);
audio_interface_init(audio);
```

### Windows WASAPI 实现

Windows平台使用WASAPI(Windows Audio Session API)实现现代音频功能。

#### 特性
- 低延迟音频处理
- 支持独占和共享模式
- 支持音频会话管理
- 支持音频端点检测
- 支持音频格式协商

#### 实现示例
```c
#include "audio/wasapi_windows.h"

// 创建WASAPI音频接口
AudioInterface* audio = wasapi_windows_create();

// WASAPI特定配置
wasapi_config_t config = {
    .share_mode = AUDCLNT_SHAREMODE_SHARED,
    .sample_rate = 16000,
    .channels = 1,
    .bits_per_sample = 16,
    .buffer_duration = 100000,  // 10ms in 100ns units
    .use_event_driven = true
};

wasapi_windows_set_config(audio, &config);
audio_interface_init(audio);
```

## 依赖安装

### macOS

```bash
# 安装 Homebrew (如果还没有安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 PortAudio
brew install portaudio
```

### ESP32

```bash
# 安装ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# 设置环境变量
. ./export.sh

# 安装必要的Python包
pip install esptool
```

### 树莓派/博通芯片

```bash
# 更新系统
sudo apt-get update && sudo apt-get upgrade

# 安装ALSA开发库
sudo apt-get install libasound2-dev

# 安装博通特定库
sudo apt-get install libraspberrypi-dev

# 安装音频工具
sudo apt-get install alsa-utils pulseaudio

# 启用音频接口
sudo raspi-config
# Advanced Options -> Audio -> Force 3.5mm
```

### Linux (Ubuntu/Debian)

```bash
# 安装ALSA开发库
sudo apt-get update
sudo apt-get install libasound2-dev

# 安装PulseAudio开发库 (可选)
sudo apt-get install libpulse-dev

# 安装音频工具
sudo apt-get install alsa-utils
```

### Linux (CentOS/RHEL)

```bash
# 安装ALSA开发库
sudo yum install alsa-lib-devel

# 或使用dnf (较新版本)
sudo dnf install alsa-lib-devel

# 安装音频工具
sudo yum install alsa-utils
```

### Windows

```bash
# 使用vcpkg安装依赖
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装音频库 (可选，WASAPI是系统自带)
.\vcpkg install portaudio:x64-windows
```

## 编译和测试

### 使用 Makefile

```bash
cd sdk/audio/test

# 安装依赖
make install-deps

# 编译
make

# 运行基础测试
make test

# 运行交互式测试 (录音和播放)
make test-interactive
```

### 使用 CMake

```bash
cd sdk/audio
mkdir build && cd build
cmake ..
make
```

## 使用示例

```c
#include "audio/audio_interface.h"
#include "audio/portaudio_mac.h"

int main() {
    // 创建音频接口
    AudioInterface* audio = portaudio_mac_create();
    
    // 初始化
    audio_interface_init(audio);
    
    // 配置音频参数
    // 采样率44.1kHz, 帧大小1024, 双声道, 4个周期, 缓冲区8192, 周期大小2048
    audio_interface_set_config(audio, 16000, 1024, 2, 4, 8192, 2048);
    
    // 开始录音
    audio_interface_record(audio);
    
    // 开始播放
    audio_interface_play(audio);
    
    // 音频数据缓冲区
    short buffer[1024 * 2]; // 1024帧 * 2声道
    
    // 录音和播放循环
    while (running) {
        // 从麦克风读取数据
        if (audio_interface_read(audio, buffer, 1024)) {
            // 写入到扬声器
            audio_interface_write(audio, buffer, 1024);
        }
    }
    
    // 清理资源
    audio_interface_destroy(audio);
    
    return 0;
}
```

## API 参考

### AudioInterface

#### 函数

- `void audio_interface_init(AudioInterface* self)` - 初始化音频接口
- `void audio_interface_set_config(...)` - 设置音频配置
- `bool audio_interface_read(AudioInterface* self, short* buffer, size_t frame_size)` - 读取音频数据
- `bool audio_interface_write(AudioInterface* self, short* buffer, size_t frame_size)` - 写入音频数据
- `void audio_interface_record(AudioInterface* self)` - 开始录音
- `void audio_interface_play(AudioInterface* self)` - 开始播放
- `void audio_interface_destroy(AudioInterface* self)` - 销毁音频接口

#### 配置参数

- `sample_rate`: 采样率 (Hz)，推荐 16000 或 48000
- `frame_size`: 每次处理的帧数，推荐 512, 1024, 2048
- `channels`: 声道数，1=单声道，2=立体声
- `periods`: 周期数，通常 2-8
- `buffer_size`: 缓冲区大小（样本数）
- `period_size`: 周期大小（样本数）

### PortAudioMac

#### 创建实例

```c
AudioInterface* portaudio_mac_create(void);
```

## 注意事项

### 通用注意事项

1. **延迟**: 较小的frame_size会降低延迟但增加CPU使用率
2. **缓冲区**: 适当的缓冲区大小可以避免音频断续
3. **线程安全**: 所有API都是线程安全的
4. **错误处理**: 检查返回值和日志输出

### 平台特定注意事项

#### macOS
- **权限**: 应用可能需要麦克风权限，在系统偏好设置中启用
- **音频单元**: 避免与其他音频应用冲突
- **采样率**: 系统可能会自动转换采样率，建议使用44.1kHz或48kHz

#### ESP32
- **内存限制**: ESP32内存有限，注意缓冲区大小设置
- **引脚配置**: 确保I2S引脚配置正确，避免与其他外设冲突
- **电源管理**: 音频播放时禁用WiFi省电模式以避免音频断续
- **DMA缓冲区**: DMA缓冲区数量和大小影响音频质量和内存使用
- **时钟精度**: 使用外部晶振获得更好的音频质量

```c
// ESP32优化配置示例
esp32_audio_config_t config = {
    .dma_buf_count = 6,      // 减少内存使用
    .dma_buf_len = 512,      // 较小的缓冲区
    .use_apll = true,        // 使用APLL获得更好的时钟精度
    .fixed_mclk = 0          // 自动计算MCLK
};
```

#### 树莓派/博通芯片
- **音频路由**: 正确配置音频输出路由(3.5mm/HDMI)
- **USB音频**: USB音频设备可能需要额外的延迟补偿
- **I2S配置**: 使用外部DAC时需要正确配置设备树
- **权限**: 确保用户在audio组中: `sudo usermod -a -G audio $USER`
- **CPU负载**: 树莓派CPU性能有限，避免过高的采样率和复杂处理

```bash
# 检查音频设备
aplay -l
arecord -l

# 测试音频输出
speaker-test -t wav -c 2
```

#### Linux
- **设备权限**: 确保用户有音频设备访问权限
- **PulseAudio冲突**: 可能需要停止PulseAudio以获得低延迟
- **ALSA配置**: 检查/etc/asound.conf和~/.asoundrc配置
- **实时优先级**: 考虑使用实时调度以减少音频延迟

```bash
# 停止PulseAudio (如需要)
pulseaudio --kill

# 设置实时优先级
sudo setcap cap_sys_nice+ep your_audio_app
```

#### Windows
- **独占模式**: 独占模式可以获得更低延迟但会阻止其他应用使用音频
- **音频会话**: 正确管理音频会话以避免冲突
- **驱动程序**: 确保音频驱动程序是最新版本
- **防火墙**: 某些音频应用可能被防火墙阻止

```c
// Windows低延迟配置
wasapi_config_t config = {
    .share_mode = AUDCLNT_SHAREMODE_EXCLUSIVE,  // 独占模式
    .buffer_duration = 30000,  // 3ms缓冲区
    .use_event_driven = true   // 事件驱动模式
};
```

## 扩展支持

### 添加新平台支持

要添加新的平台支持，需要实现以下步骤：

#### 1. 创建平台特定的头文件和源文件

```c
// 例如：audio/my_platform.h
#ifndef MY_PLATFORM_AUDIO_H
#define MY_PLATFORM_AUDIO_H

#include "audio_interface.h"

// 平台特定配置结构
typedef struct {
    int sample_rate;
    int channels;
    int buffer_size;
    // 其他平台特定参数
} my_platform_config_t;

// 创建函数
AudioInterface* my_platform_create(void);

// 平台特定配置函数
void my_platform_set_config(AudioInterface* self, const my_platform_config_t* config);

#endif
```

#### 2. 实现AudioInterfaceVTable中的所有函数

```c
// audio/my_platform.c
#include "my_platform.h"
#include <stdlib.h>

typedef struct {
    AudioInterface base;
    my_platform_config_t config;
    // 平台特定的私有数据
    void* platform_handle;
    bool is_recording;
    bool is_playing;
} MyPlatformAudio;

// 实现虚函数表中的所有函数
static void my_platform_init(AudioInterface* self) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 初始化平台特定的音频系统
}

static void my_platform_set_config(AudioInterface* self, int sample_rate, 
                                   int frame_size, int channels, int periods,
                                   int buffer_size, int period_size) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 设置音频配置
}

static bool my_platform_read(AudioInterface* self, short* buffer, size_t frame_size) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 从音频设备读取数据
    return true;
}

static bool my_platform_write(AudioInterface* self, const short* buffer, size_t frame_size) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 向音频设备写入数据
    return true;
}

static void my_platform_record(AudioInterface* self) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 开始录音
    impl->is_recording = true;
}

static void my_platform_play(AudioInterface* self) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 开始播放
    impl->is_playing = true;
}

static void my_platform_destroy(AudioInterface* self) {
    MyPlatformAudio* impl = (MyPlatformAudio*)self;
    // 清理资源
    free(impl);
}

// 虚函数表
static const AudioInterfaceVTable my_platform_vtable = {
    .init = my_platform_init,
    .set_config = my_platform_set_config,
    .read = my_platform_read,
    .write = my_platform_write,
    .record = my_platform_record,
    .play = my_platform_play,
    .destroy = my_platform_destroy
};

// 创建函数
AudioInterface* my_platform_create(void) {
    MyPlatformAudio* impl = malloc(sizeof(MyPlatformAudio));
    if (!impl) return NULL;
    
    impl->base.vtable = &my_platform_vtable;
    impl->platform_handle = NULL;
    impl->is_recording = false;
    impl->is_playing = false;
    
    return (AudioInterface*)impl;
}
```

#### 3. 更新CMakeLists.txt

```cmake
# 在CMakeLists.txt中添加新的源文件
if(MY_PLATFORM)
    list(APPEND AUDIO_SOURCES
        audio/my_platform.c
    )
    list(APPEND AUDIO_HEADERS
        audio/my_platform.h
    )
    # 添加平台特定的链接库
    target_link_libraries(audio_interface my_platform_lib)
endif()
```

#### 4. 添加平台检测

```cmake
# 在主CMakeLists.txt中添加平台检测
if(CMAKE_SYSTEM_NAME STREQUAL "MyPlatform")
    set(MY_PLATFORM ON)
    add_definitions(-DMY_PLATFORM)
endif()
```

### 平台实现最佳实践

1. **错误处理**: 所有函数都应该有适当的错误处理和日志记录
2. **资源管理**: 确保正确释放所有分配的资源
3. **线程安全**: 如果平台API不是线程安全的，添加必要的同步机制
4. **配置验证**: 验证音频配置参数的有效性
5. **性能优化**: 针对平台特性进行优化

### 测试新平台实现

```c
// 创建平台特定的测试
#include "audio/my_platform.h"

int test_my_platform() {
    AudioInterface* audio = my_platform_create();
    if (!audio) {
        printf("Failed to create audio interface\n");
        return -1;
    }
    
    // 测试初始化
    audio_interface_init(audio);
    
    // 测试配置
    audio_interface_set_config(audio, 16000, 1024, 2, 4, 8192, 2048);
    
    // 测试录音和播放
    audio_interface_record(audio);
    audio_interface_play(audio);
    
    // 清理
    audio_interface_destroy(audio);
    
    printf("Platform test completed successfully\n");
    return 0;
}
```

## 故障排除

### 通用问题

#### 编译错误
- 检查所有依赖库是否正确安装
- 验证头文件路径配置
- 确认CMake配置正确
- 检查编译器版本兼容性

#### 运行时错误
- 检查音频设备是否可用和可访问
- 查看日志输出获取详细错误信息
- 验证音频配置参数的有效性
- 检查内存分配是否成功

#### 音频质量问题
- 调整采样率和缓冲区大小
- 检查音频设备设置
- 避免音频反馈（使用耳机）
- 检查音频数据格式是否正确

### 平台特定故障排除

#### macOS
```bash
# 检查PortAudio安装
brew list portaudio

# 检查音频设备
system_profiler SPAudioDataType

# 重置音频系统
sudo killall coreaudiod

# 检查权限
tccutil reset Microphone com.yourapp.bundle
```

**常见问题:**
- `kAudioUnitErr_NoConnection`: 检查音频设备连接
- `kAudioUnitErr_CannotDoInCurrentContext`: 音频单元状态错误
- 权限被拒绝: 在系统偏好设置中启用麦克风权限

#### ESP32
```bash
# 检查ESP-IDF版本
idf.py --version

# 清理并重新编译
idf.py fullclean
idf.py build

# 检查分区表
idf.py partition_table

# 监控串口输出
idf.py monitor
```

**常见问题:**
- `ESP_ERR_NO_MEM`: 内存不足，减少DMA缓冲区大小
- `ESP_ERR_INVALID_ARG`: 检查I2S配置参数
- 音频断续: 禁用WiFi省电模式，增加任务优先级
- 无声音输出: 检查I2S引脚连接和DAC配置

```c
// ESP32调试配置
esp32_audio_config_t debug_config = {
    .dma_buf_count = 4,      // 减少内存使用
    .dma_buf_len = 256,      // 更小的缓冲区
    .use_apll = false,       // 禁用APLL调试
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
};
```

#### 树莓派/博通芯片
```bash
# 检查音频设备状态
cat /proc/asound/cards
cat /proc/asound/devices

# 测试音频输出
aplay /usr/share/sounds/alsa/Front_Left.wav

# 检查ALSA配置
alsactl store
alsactl restore

# 检查设备树配置
dtoverlay -l
```

**常见问题:**
- 无音频设备: 检查`dtparam=audio=on`配置
- 权限错误: 添加用户到audio组
- USB音频延迟: 调整USB音频缓冲区设置
- I2S配置错误: 检查设备树覆盖配置

#### Linux
```bash
# 检查ALSA状态
cat /proc/asound/version
aplay -l
arecord -l

# 检查PulseAudio状态
pulseaudio --check -v

# 重启音频服务
sudo systemctl restart alsa-state
sudo systemctl restart pulseaudio

# 检查音频权限
groups $USER | grep audio
```

**常见问题:**
- `Device or resource busy`: 其他进程占用音频设备
- `Permission denied`: 用户权限不足
- `No such device`: ALSA配置错误或设备不存在
- 高延迟: 停止PulseAudio或调整缓冲区设置

#### Windows
```cmd
# 检查音频设备
dxdiag /t audio_info.txt

# 重启音频服务
net stop audiosrv
net start audiosrv

# 检查WASAPI设备
powershell "Get-WmiObject -Class Win32_SoundDevice"
```

**常见问题:**
- `AUDCLNT_E_DEVICE_IN_USE`: 设备被其他应用占用
- `AUDCLNT_E_UNSUPPORTED_FORMAT`: 音频格式不支持
- `E_INVALIDARG`: 配置参数无效
- 独占模式失败: 检查其他音频应用是否在运行

### 性能调优

#### 延迟优化
```c
// 低延迟配置示例
audio_interface_set_config(audio, 
    48000,  // 高采样率
    128,    // 小帧大小
    2,      // 立体声
    2,      // 少周期数
    512,    // 小缓冲区
    256     // 小周期大小
);
```

#### 内存优化
```c
// 内存优化配置
audio_interface_set_config(audio,
    22050,  // 较低采样率
    1024,   // 较大帧大小
    1,      // 单声道
    4,      // 适中周期数
    4096,   // 适中缓冲区
    1024    // 适中周期大小
);
```

### 调试工具

#### 音频分析
```bash
# 录制音频进行分析
arecord -f cd -t wav -d 10 test.wav

# 播放测试音频
speaker-test -t sine -f 1000 -l 1

# 音频频谱分析
ffmpeg -i test.wav -af "showspectrum" spectrum.png
```

#### 性能监控
```bash
# 监控CPU使用率
top -p $(pgrep your_audio_app)

# 监控内存使用
valgrind --tool=memcheck ./your_audio_app

# 监控音频延迟
jack_delay (如果使用JACK)
```