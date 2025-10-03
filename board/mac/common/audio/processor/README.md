# Mac音频处理器实现

本目录包含了专为Mac平台优化的音频处理器实现，基于Core Audio框架和Accelerate框架提供高性能的音频处理功能。

## 功能特性

### 核心功能
- **VAD (语音活动检测)**: 基于能量和过零率的自适应语音检测
- **AEC (回声消除)**: NLMS自适应滤波器实现的回声消除
- **NS (噪声抑制)**: 基于能量门限的噪声抑制
- **实时处理**: 低延迟音频处理管道

### Mac平台优化
- 使用Core Audio框架进行音频I/O
- 利用Accelerate框架进行高性能数学运算
- 针对macOS音频子系统优化的缓冲区管理
- 支持多种音频格式和采样率

## 文件结构

```
processor/
├── audio_processor_mac.h    # Mac音频处理器头文件
├── audio_processor_mac.c    # Mac音频处理器实现
└── README.md               # 本文档
```

## 技术实现

### VAD (语音活动检测)

VAD实现基于以下特征：

1. **能量检测**: 计算音频帧的RMS能量
2. **过零率**: 统计信号过零点的频率
3. **自适应阈值**: 根据历史数据动态调整检测阈值
4. **状态机**: 包含拖尾时间的状态转换机制

```c
// VAD特征计算
float energy = mac_calculate_energy(samples, sample_count);
float zcr = mac_calculate_zero_crossing_rate(samples, sample_count);

// 自适应阈值
float adaptive_threshold = base_threshold + avg_energy * 0.1f;

// VAD决策
bool is_speech = (energy > adaptive_threshold) && (zcr < zcr_threshold);
```

### AEC (回声消除)

AEC使用NLMS (Normalized Least Mean Squares) 自适应滤波器：

1. **自适应滤波**: 实时学习回声路径特性
2. **归一化步长**: 根据参考信号功率调整学习速率
3. **误差反馈**: 基于误差信号更新滤波器系数

```c
// NLMS滤波器更新
float step = step_size / (ref_power + epsilon);
for (size_t j = 0; j < filter_length; j++) {
    filter_coeffs[j] += step * error * reference_buffer[idx];
}
```

### NS (噪声抑制)

噪声抑制采用简化的时域方法：

1. **能量估计**: 计算当前帧的能量水平
2. **噪声底噪跟踪**: 自适应更新噪声底噪估计
3. **增益控制**: 根据信噪比应用不同的增益因子

```c
// 噪声抑制增益计算
if (frame_energy < noise_threshold) {
    gain = 0.1f;  // 大幅衰减
} else if (frame_energy < noise_threshold * 3.0f) {
    gain = 0.5f;  // 部分抑制
}
```

## 配置参数

### 音频格式配置
```c
audio_processor_config_t config = {
    .sample_rate = 16000,           // 采样率
    .channels = 1,                  // 声道数
    .frame_duration_ms = 60,        // 帧长度
    .enable_vad = true,             // 启用VAD
    .enable_aec = true,             // 启用AEC
    .enable_ns = true,              // 启用噪声抑制
    .vad_threshold = 0.01f          // VAD阈值
};
```

### VAD参数调优
- `energy_threshold`: 能量检测阈值 (默认: 0.01)
- `zcr_threshold`: 过零率阈值 (默认: 0.3)
- `hangover_time`: 拖尾时间 (默认: 500ms)
- `history_size`: 历史窗口大小 (默认: 50帧)

### AEC参数调优
- `filter_length`: 滤波器长度 (默认: 512)
- `step_size`: LMS步长 (默认: 0.01)
- `convergence_time`: 收敛时间 (约2-5秒)

### NS参数调优
- `noise_floor`: 噪声底噪 (默认: 0.001)
- `alpha`: 平滑因子 (默认: 0.95)
- `suppression_factor`: 抑制因子 (0.1-0.5)

## 性能特性

### 延迟指标
- **处理延迟**: < 60ms (单帧处理)
- **算法延迟**: < 10ms (VAD/NS)
- **AEC延迟**: 取决于滤波器长度

### 计算复杂度
- **VAD**: O(N) - 线性复杂度
- **AEC**: O(N×M) - N为帧长，M为滤波器长度
- **NS**: O(N) - 线性复杂度

### 内存使用
- **基础缓冲区**: ~16KB (16kHz, 60ms帧)
- **VAD历史**: ~800B (50帧历史)
- **AEC滤波器**: ~8KB (512系数 × 4字节 × 4缓冲区)
- **总内存**: ~25KB

## 使用示例

### 基本使用
```c
// 创建处理器
AudioProcessor* processor = audio_processor_mac_create();

// 配置参数
audio_processor_config_t config;
audio_processor_config_init_default(&config, 16000, 1, 60);
config.enable_vad = true;
config.enable_aec = true;
config.enable_ns = true;

// 初始化
audio_processor_initialize(processor, &config, NULL);

// 设置回调
audio_processor_set_output_callback(processor, output_callback, user_data);
audio_processor_set_vad_callback(processor, vad_callback, user_data);

// 启动处理
audio_processor_start(processor);

// 处理音频数据
audio_processor_feed(processor, audio_data, frame_size);

// 停止和清理
audio_processor_stop(processor);
audio_processor_destroy(processor);
```

### 高级配置
```c
// 自定义VAD阈值
config.vad_threshold = 0.005f;  // 更敏感的检测

// 启用设备AEC
audio_processor_enable_device_aec(processor, true);

// 重置处理器状态
audio_processor_reset(processor);

// 查询处理器状态
bool vad_active = audio_processor_get_vad_status(processor);
int delay_ms = audio_processor_get_delay_ms(processor);
```

## 依赖要求

### 系统框架
- **Core Audio**: 音频I/O和格式转换
- **AudioToolbox**: 音频处理工具
- **AudioUnit**: 音频单元支持
- **Accelerate**: 高性能数学运算
- **CoreFoundation**: 基础系统服务

### 编译要求
- macOS 10.12 或更高版本
- Xcode 9.0 或更高版本
- CMake 3.10 或更高版本

### 运行时要求
- 音频输入/输出设备
- 足够的CPU资源 (建议 > 10% 单核)
- 内存 > 50MB 可用

## 调试和优化

### 日志级别
```bash
export LINX_LOG_LEVEL=DEBUG  # 启用详细日志
```

### 性能监控
```c
// 获取处理统计
uint64_t frames_processed = data->frames_processed;
uint64_t vad_speech_frames = data->vad_speech_frames;
float processing_load = calculate_cpu_usage();
```

### 常见问题

1. **VAD误检测**
   - 调整 `vad_threshold` 参数
   - 检查音频输入增益
   - 确认环境噪声水平

2. **AEC效果不佳**
   - 增加滤波器长度
   - 调整步长参数
   - 确保参考信号质量

3. **处理延迟过高**
   - 减少帧长度
   - 优化缓冲区大小
   - 检查系统负载

## 扩展开发

### 添加新算法
1. 在头文件中定义新的状态结构
2. 实现初始化和处理函数
3. 集成到主处理流程中
4. 添加相应的配置参数

### 性能优化
1. 使用Accelerate框架的向量化函数
2. 实现SIMD优化的关键算法
3. 优化内存访问模式
4. 减少不必要的内存分配

### 测试验证
1. 单元测试各个算法模块
2. 集成测试完整处理流程
3. 性能基准测试
4. 音质主观评测

## 许可证

本实现遵循与主项目相同的许可证条款。