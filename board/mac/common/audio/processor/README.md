# SpeexDSP 音频处理器

## 概述

本目录包含基于SpeexDSP库的音频处理器实现，提供了回声消除(AEC)、噪声抑制(NS)和语音活动检测(VAD)功能。该实现遵循标准的 `audio_processor.h` 接口，可以与其他音频处理器实现互换使用。

## 文件结构

```
audio/processor/
├── audio_processor_speexdsp.h    # SpeexDSP音频处理器头文件
├── audio_processor_speexdsp.c    # SpeexDSP音频处理器实现
├── README.md                     # 本文档
└── test/                         # 测试代码
    ├── CMakeLists.txt
    ├── README.md
    └── audio_processor_mac_realtime_test.c
```

## 功能特性

### 1. 回声消除 (AEC)
- 基于SpeexDSP的自适应回声消除算法
- 支持可配置的滤波器长度（默认200ms）
- 自动适应不同的声学环境
- 支持运行时参数调整

### 2. 噪声抑制 (NS)
- 基于频谱减法的噪声抑制
- 可配置的噪声抑制强度（-15到0 dB）
- 自适应噪声估计
- 保持语音质量的同时有效降噪

### 3. 自动增益控制 (AGC)
- 自动调整音频电平
- 可配置的目标电平（1到32768）
- 防止音频削波和过载
- 保持一致的输出音量

### 4. 语音活动检测 (VAD)
- 基于能量和频谱特征的VAD算法
- 实时语音/静音状态检测
- 支持VAD状态变化回调
- 提供统计信息（语音帧数、静音帧数）

## API 接口

### 基础接口

```c
// 创建SpeexDSP音频处理器实例
AudioProcessor* audio_processor_speexdsp_create(void);

// 销毁音频处理器实例
void audio_processor_speexdsp_destroy(AudioProcessor* processor);
```

### 标准音频处理器接口

```c
// 初始化音频处理器
audio_processor_error_t audio_processor_initialize(AudioProcessor* processor,
                                                   const audio_processor_config_t* config,
                                                   AudioInterface* audio_interface);

// 启动/停止音频处理器
audio_processor_error_t audio_processor_start(AudioProcessor* processor);
audio_processor_error_t audio_processor_stop(AudioProcessor* processor);

// 输入音频数据
audio_processor_error_t audio_processor_feed(AudioProcessor* processor, 
                                            const int16_t* data, 
                                            size_t size);

// 设置回调函数
audio_processor_error_t audio_processor_set_output_callback(AudioProcessor* processor,
                                                           audio_processor_output_callback_t callback,
                                                           void* user_data);

audio_processor_error_t audio_processor_set_vad_callback(AudioProcessor* processor,
                                                        audio_processor_vad_callback_t callback,
                                                        void* user_data);
```

### 扩展接口

```c
// 获取统计信息
audio_processor_error_t audio_processor_speexdsp_get_stats(const AudioProcessor* processor,
                                                          unsigned long* frames_processed,
                                                          unsigned long* vad_speech_frames,
                                                          unsigned long* vad_silence_frames);

// 设置AEC参数
audio_processor_error_t audio_processor_speexdsp_set_aec_params(AudioProcessor* processor,
                                                               int filter_length);

// 设置降噪参数
audio_processor_error_t audio_processor_speexdsp_set_denoise_params(AudioProcessor* processor,
                                                                   int noise_suppress,
                                                                   int agc_level);
```

## 使用示例

### 基本使用

```c
#include "audio_processor_speexdsp.h"
#include "../../../../src/common/log/linx_log.h"

int main() {
    // 1. 首先初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_INFO;  // 设置日志级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        printf("日志系统初始化失败\n");
        return -1;
    }

    // 2. 创建音频处理器
    AudioProcessor* processor = audio_processor_speexdsp_create();
    if (!processor) {
        LINX_LOGE("MAIN", "创建音频处理器失败");
        return -1;
    }

    // 3. 配置参数
    audio_processor_config_t config;
    audio_processor_config_init_default(&config, 16000, 1, 20); // 16kHz, 单声道, 20ms帧
    config.enable_aec = true;
    config.enable_ns = true;
    config.enable_vad = true;

    // 4. 初始化
    AudioInterface* audio_interface = /* 获取音频接口 */;
    if (audio_processor_initialize(processor, &config, audio_interface) != AUDIO_PROCESSOR_SUCCESS) {
        LINX_LOGE("MAIN", "音频处理器初始化失败");
        audio_processor_speexdsp_destroy(processor);
        return -1;
    }

// 设置输出回调
audio_processor_set_output_callback(processor, output_callback, user_data);

// 设置VAD回调
audio_processor_set_vad_callback(processor, vad_callback, user_data);

// 启动处理器
audio_processor_start(processor);

// 输入音频数据
int16_t audio_data[320]; // 20ms @ 16kHz
audio_processor_feed(processor, audio_data, 320);

// 停止和清理
audio_processor_stop(processor);
audio_processor_speexdsp_destroy(processor);
```

### 高级配置

```c
// 调整AEC滤波器长度为300ms
audio_processor_speexdsp_set_aec_params(processor, 300 * 16); // 300ms * 16kHz

// 调整降噪参数
audio_processor_speexdsp_set_denoise_params(processor, -10, 12000); // -10dB降噪, AGC目标12000

// 获取统计信息
unsigned long frames_processed, speech_frames, silence_frames;
audio_processor_speexdsp_get_stats(processor, &frames_processed, &speech_frames, &silence_frames);
printf("处理帧数: %lu, 语音帧: %lu, 静音帧: %lu\n", 
       frames_processed, speech_frames, silence_frames);
```

## 配置参数

### 音频参数
- **采样率**: 支持8kHz, 16kHz, 32kHz, 48kHz
- **声道数**: 支持单声道和立体声
- **帧长度**: 建议10-30ms（默认20ms）
- **数据格式**: 16位PCM

### SpeexDSP参数
- **AEC滤波器长度**: 100-500ms（默认200ms）
- **噪声抑制强度**: -15到0 dB（默认-15dB）
- **AGC目标电平**: 1-32768（默认8000）

## 编译要求

### 依赖库
- SpeexDSP库 (libspeexdsp)
- pthread库
- 标准C库

### 编译选项
```bash
# 启用SpeexDSP支持
-DENABLE_SPEEX_DSP

# 链接库
-lspeexdsp -lpthread
```

### CMake配置示例
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

target_compile_definitions(audio_processor_speexdsp PRIVATE ENABLE_SPEEX_DSP)
target_link_libraries(audio_processor_speexdsp ${SPEEXDSP_LIBRARIES})
target_include_directories(audio_processor_speexdsp PRIVATE ${SPEEXDSP_INCLUDE_DIRS})
```

## 性能特性

### 计算复杂度
- **AEC**: O(N*M)，其中N为帧长，M为滤波器长度
- **NS**: O(N*log(N))，基于FFT的频域处理
- **VAD**: O(N)，时域特征提取

### 内存使用
- 基础内存: ~50KB
- AEC滤波器: ~4KB per 100ms filter length
- 音频缓冲区: 帧长 × 声道数 × 3 × sizeof(int16_t)

### 延迟
- 算法延迟: 1帧长度（默认20ms）
- 缓冲延迟: 可忽略
- 总延迟: ~20-30ms

## 注意事项

1. **线程安全**: 所有API都是线程安全的，使用互斥锁保护共享数据
2. **内存管理**: 自动管理内部缓冲区，用户只需管理输入/输出数据
3. **错误处理**: 所有函数都返回错误代码，建议检查返回值
4. **参数验证**: 输入参数会被验证，无效参数会返回错误
5. **资源清理**: 必须调用destroy函数释放资源

## 故障排除

### 常见问题

1. **编译错误**: 确保安装了SpeexDSP开发库
2. **运行时错误**: 检查音频参数是否正确配置
3. **性能问题**: 考虑减少滤波器长度或帧长度
4. **音质问题**: 调整降噪强度和AGC参数

### 调试建议

1. 启用详细日志输出
2. 检查统计信息确认处理状态
3. 使用测试程序验证功能
4. 监控CPU和内存使用情况

## 参考资料

- [SpeexDSP官方文档](https://speex.org/docs/)
- [音频处理器接口文档](../../../../src/audio/processor/audio_processor.h)
- [测试代码示例](test/)

## 版本历史

- v1.0.0: 初始版本，支持基本的AEC、NS、VAD功能
- 后续版本将添加更多高级功能和优化