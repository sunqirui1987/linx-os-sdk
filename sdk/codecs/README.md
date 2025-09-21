# LINX Audio Codecs

LINX Audio Codecs 是一个模块化的音频编解码器库，提供统一的接口来支持多种音频编解码格式。目前主要支持 Opus 编解码器，采用工厂模式设计，便于扩展和维护。

## 目录结构

```
codecs/
├── CMakeLists.txt          # CMake 构建配置
├── README.md              # 本文档
├── audio_codec.h          # 音频编解码器通用接口定义
├── codec_factory.c        # 编解码器工厂实现
├── opus_codec.h           # Opus 编解码器接口
├── opus_codec.c           # Opus 编解码器实现
├── opus/                  # Opus 库源码（子模块）
├── build/                 # 构建输出目录
└── test/                  # 测试代码
    └── codec_test.c       # 编解码器测试程序
```

## 架构设计

### 核心组件

1. **通用接口层** (`audio_codec.h`)
   - 定义了统一的音频编解码器接口
   - 使用虚函数表（vtable）实现多态
   - 支持编码、解码、参数配置等操作

2. **工厂模式** (`codec_factory.c`)
   - 提供编解码器实例的创建和销毁
   - 支持动态选择不同的编解码器类型
   - 便于添加新的编解码器支持

3. **Opus 实现** (`opus_codec.c/h`)
   - 基于 libopus 的完整实现
   - 支持丰富的参数配置
   - 优化的音频质量和压缩率

4. **ES8311 实现** (`es8311_codec.c/h`)
   - 低功耗单声道音频编解码器
   - I2C 控制接口，I2S 数据传输
   - 支持麦克风输入和线路输出
   - 适用于语音通话和音频播放应用

5. **ES8388 实现** (`es8388_codec.c/h`)
   - 低功耗立体声音频编解码器
   - 双通道 ADC/DAC 支持
   - 灵活的音频路由和混音功能
   - 适用于高质量音频录制和播放

### 设计模式

- **策略模式**: 通过虚函数表实现不同编解码器的策略切换
- **工厂模式**: 统一的编解码器创建接口
- **RAII**: 自动资源管理，防止内存泄漏

## 功能特性

### 支持的编解码器

- **Opus**: 高质量、低延迟的音频编解码器
  - 支持 8kHz 到 48kHz 采样率
  - 可变比特率 (VBR) 和恒定比特率 (CBR)
  - 前向错误纠正 (FEC) 和丢包隐藏
  - 语音和音乐优化模式

- **ES8311**: 低功耗单声道音频编解码器 
  - 高性能低功耗多位 delta-sigma 音频 ADC 和 DAC
  - I2S/PCM 主从串行数据端口支持
  - ADC: 24位，8-96 kHz 采样频率，100 dB 信噪比
  - DAC: 24位，8-96 kHz 采样频率，110 dB 信噪比
  - 支持 I2S、左对齐、右对齐和 DSP/PCM 格式 
  - I2C 控制接口，默认地址 0x18
  - 支持麦克风输入和可调增益控制

- **ES8388**: 低功耗立体声音频编解码器
  - 立体声 ADC 和 DAC 支持
  - 高质量音频处理能力
  - I2S 数字音频接口
  - 灵活的音频路由和混音功能
  - 低功耗设计，适合便携设备
  - 支持多种采样率和位深度配置

### 核心功能

- 音频编码和解码
- 实时参数调整
- 错误处理和恢复
- 内存管理
- 性能优化

## 快速开始

### 构建要求

- CMake 3.10+
- C99 兼容的编译器 (GCC, Clang)
- libopus 库

#### ES8311/ES8388 额外依赖

- **I2C 库**: 用于编解码器配置和控制
- **I2S 驱动**: 用于音频数据传输
- **硬件平台支持**:
  - ESP32 系列 (推荐，有官方驱动支持)
  - 其他支持 I2C 和 I2S 的嵌入式平台
- **可选依赖**:
  - ESP-IDF (用于 ESP32 平台)
  - HAL 库 (用于其他 ARM 平台)

### 构建步骤

#### 基础构建 (Opus)

1. **构建 Opus 库**:
```bash
cd opus
./autogen.sh
./configure
make
```

2. **构建 LINX Codecs**:
```bash
mkdir build && cd build
cmake ..
make
```

3. **运行测试**:
```bash
cd test
./codec_test
```

#### ES8311/ES8388 构建 (ESP32 平台)

1. **安装 ESP-IDF**:
```bash
# 安装 ESP-IDF 开发环境
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh
```

2. **添加 ES8311 组件依赖**:
```bash
# 在项目根目录的 idf_component.yml 中添加
dependencies:
  espressif/es8311: "^1.0.0"
  espressif/esp_codec_dev: "^1.0.0"
```

3. **配置 CMakeLists.txt**:
```cmake
# 添加 ES 编解码器支持
set(CODEC_SRCS 
    "opus_codec.c"
    "es8311_codec.c"  # 新增
    "es8388_codec.c"  # 新增
    "codec_factory.c"
)

# 链接 ES 编解码器库
target_link_libraries(${COMPONENT_LIB} 
    PRIVATE 
    opus 
    es8311
    esp_codec_dev
)
```

4. **构建项目**:
```bash
idf.py build
```

### 基本使用示例

```c
#include "audio_codec.h"

int main() {
    // 创建 Opus 编解码器
    audio_codec_t* codec = codec_factory_create(CODEC_TYPE_OPUS);
    if (!codec) {
        printf("Failed to create codec\n");
        return -1;
    }
    
    // 配置音频格式
    audio_format_t format;
    audio_format_init(&format, 16000, 1, 16, 20); // 16kHz, 单声道, 16位, 20ms帧
    
    // 初始化编码器
    codec_error_t result = codec->vtable->init_encoder(codec, &format);
    if (result != CODEC_SUCCESS) {
        printf("Failed to initialize encoder\n");
        codec_factory_destroy(codec);
        return -1;
    }
    
    // 编码音频数据
    int16_t input_buffer[320];  // 20ms @ 16kHz
    uint8_t output_buffer[1000];
    size_t encoded_size;
    
    // 填充输入数据...
    
    result = codec->vtable->encode(codec, input_buffer, 320, 
                                  output_buffer, sizeof(output_buffer), 
                                  &encoded_size);
    
    if (result == CODEC_SUCCESS) {
        printf("Encoded %zu bytes\n", encoded_size);
    }
    
    // 清理资源
    codec_factory_destroy(codec);
    return 0;
}
```

### ES8311 配置示例

```c
#include "audio_codec.h"
#include "es8311_codec.h"

int main() {
    // 创建 ES8311 编解码器
    audio_codec_t* codec = codec_factory_create(CODEC_TYPE_ES8311);
    if (!codec) {
        printf("Failed to create ES8311 codec\n");
        return -1;
    }
    
    // 配置 ES8311 特定参数
    es8311_config_t es8311_cfg = {
        .i2c_addr = 0x18,           // I2C 地址
        .sample_rate = 16000,       // 采样率
        .bits_per_sample = 16,      // 位深度
        .use_mclk = true,           // 使用 MCLK
        .use_microphone = false,    // 不使用麦克风
        .mic_gain = ES8311_MIC_GAIN_42DB,  // 麦克风增益
        .dac_output = ES8311_DAC_OUTPUT_LINE_OUT,  // DAC 输出模式
    };
    
    // 初始化 ES8311
    codec_error_t result = es8311_codec_init(codec, &es8311_cfg);
    if (result != CODEC_SUCCESS) {
        printf("Failed to initialize ES8311\n");
        codec_factory_destroy(codec);
        return -1;
    }
    
    // 配置 I2S 参数
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
    };
    
    // 启动音频处理...
    
    codec_factory_destroy(codec);
    return 0;
}
```

### ES8388 配置示例

```c
#include "audio_codec.h"
#include "es8388_codec.h"

int main() {
    // 创建 ES8388 编解码器
    audio_codec_t* codec = codec_factory_create(CODEC_TYPE_ES8388);
    if (!codec) {
        printf("Failed to create ES8388 codec\n");
        return -1;
    }
    
    // 配置 ES8388 特定参数
    es8388_config_t es8388_cfg = {
        .i2c_addr = 0x10,           // I2C 地址
        .sample_rate = 44100,       // 采样率
        .bits_per_sample = 16,      // 位深度
        .adc_input = ES8388_ADC_INPUT_LINE1,     // ADC 输入源
        .dac_output = ES8388_DAC_OUTPUT_LINE,    // DAC 输出
        .mic_gain = ES8388_MIC_GAIN_24DB,        // 麦克风增益
        .es8388_mode = ES8388_MODE_BOTH,         // ADC+DAC 模式
    };
    
    // 初始化 ES8388
    codec_error_t result = es8388_codec_init(codec, &es8388_cfg);
    if (result != CODEC_SUCCESS) {
        printf("Failed to initialize ES8388\n");
        codec_factory_destroy(codec);
        return -1;
    }
    
    // 配置立体声 I2S
    i2s_config_t i2s_cfg = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // 立体声
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
    };
    
    // 设置音量和增益
    es8388_codec_set_voice_volume(codec, 60);  // 设置音量 (0-100)
    es8388_codec_set_mic_gain(codec, ES8388_MIC_GAIN_18DB);
    
    // 启动音频处理...
    
    codec_factory_destroy(codec);
    return 0;
}
```

## API 参考

### 核心数据结构

#### `audio_format_t`
```c
typedef struct {
    int sample_rate;        // 采样率 (Hz)
    int channels;           // 声道数
    int bits_per_sample;    // 每样本位数
    int frame_size_ms;      // 帧大小 (毫秒)
} audio_format_t;
```

#### `codec_error_t`
```c
typedef enum {
    CODEC_SUCCESS = 0,
    CODEC_INVALID_PARAMETER,
    CODEC_INITIALIZATION_FAILED,
    CODEC_ENCODING_FAILED,
    CODEC_DECODING_FAILED,
    CODEC_BUFFER_TOO_SMALL,
    CODEC_UNSUPPORTED_FORMAT,
    CODEC_MEMORY_ALLOCATION_FAILED
} codec_error_t;
```

### 工厂函数

#### `codec_factory_create()`
```c
audio_codec_t* codec_factory_create(codec_type_t type);
```
创建指定类型的编解码器实例。

**参数**:
- `type`: 编解码器类型 (CODEC_TYPE_OPUS)

**返回值**: 编解码器实例指针，失败时返回 NULL

#### `codec_factory_destroy()`
```c
void codec_factory_destroy(audio_codec_t* codec);
```
销毁编解码器实例并释放资源。

### 编解码器接口

#### 初始化
```c
codec_error_t (*init_encoder)(audio_codec_t* codec, const audio_format_t* format);
codec_error_t (*init_decoder)(audio_codec_t* codec, const audio_format_t* format);
```

#### 编解码
```c
codec_error_t (*encode)(audio_codec_t* codec, const int16_t* input, size_t input_size,
                       uint8_t* output, size_t output_size, size_t* encoded_size);

codec_error_t (*decode)(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                       int16_t* output, size_t output_size, size_t* decoded_size);
```

### Opus 特定功能

#### 参数配置
```c
// 设置比特率 (bps)
codec_error_t opus_codec_set_bitrate(audio_codec_t* codec, int bitrate);

// 设置复杂度 (0-10)
codec_error_t opus_codec_set_complexity(audio_codec_t* codec, int complexity);

// 设置信号类型 (OPUS_SIGNAL_VOICE, OPUS_SIGNAL_MUSIC, OPUS_AUTO)
codec_error_t opus_codec_set_signal_type(audio_codec_t* codec, int signal_type);

// 启用/禁用可变比特率
codec_error_t opus_codec_set_vbr(audio_codec_t* codec, int vbr);

// 启用/禁用前向错误纠正
codec_error_t opus_codec_set_inband_fec(audio_codec_t* codec, int use_inband_fec);
```

## 性能优化

### 编码优化建议

1. **选择合适的帧大小**: 20ms 是推荐的帧大小，平衡延迟和效率
2. **调整复杂度**: 实时应用使用较低复杂度 (5-8)，离线处理可使用最高复杂度 (10)
3. **启用 VBR**: 可变比特率通常提供更好的音质
4. **使用 FEC**: 在网络不稳定环境下启用前向错误纠正

### 内存使用

- 编码器状态: ~2.5KB
- 解码器状态: ~1.5KB
- 每帧缓冲区: 根据采样率和帧大小计算

## 测试

### 运行测试套件

```bash
cd build/test
./codec_test
```

### 测试覆盖

- 编解码器工厂测试
- Opus 编解码基本功能
- 参数配置测试
- 错误处理测试
- 性能基准测试

## 故障排除

### 常见问题

1. **编译错误**: 确保 Opus 库已正确构建
2. **链接错误**: 检查 CMake 配置中的库路径
3. **运行时错误**: 验证音频格式参数的有效性

### 调试技巧

- 启用详细日志输出
- 使用 Valgrind 检查内存泄漏
- 验证输入数据的格式和大小

## 扩展开发

### 添加新的编解码器

1. 创建新的头文件和实现文件
2. 实现 `audio_codec_vtable_t` 接口
3. 在 `codec_factory.c` 中添加创建逻辑
4. 更新 `codec_type_t` 枚举
5. 添加相应的测试用例

### 示例: 添加 AAC 编解码器

```c
// aac_codec.h
#ifndef AAC_CODEC_H
#define AAC_CODEC_H

#include "audio_codec.h"

audio_codec_t* aac_codec_create(void);

#endif

// 在 codec_factory.c 中添加
case CODEC_TYPE_AAC:
    return aac_codec_create();
```

## 许可证

本项目遵循与 LINX SDK 相同的许可证。Opus 库遵循 BSD 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request。请确保：

1. 代码符合项目的编码规范
2. 添加适当的测试用例
3. 更新相关文档
4. 通过所有现有测试

## 联系方式

如有问题或建议，请通过项目的 Issue 系统联系我们。