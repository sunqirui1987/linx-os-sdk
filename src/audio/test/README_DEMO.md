# AudioService 演示程序使用指南

## 概述

`audio_service_demo_mac.c` 是一个完整的 AudioService 使用演示程序，展示了 AudioService 的最佳使用流程和各种功能特性。该演示程序专为 Mac 平台设计，但核心的 AudioService 使用模式适用于所有平台。

## 核心特性

### 🎯 演示的 AudioService 最佳实践

1. **标准初始化流程**
   - 创建服务实例
   - 设置平台特定组件
   - 配置功能特性
   - 设置事件回调
   - 启动服务

2. **完整的音频处理流程**
   - 音频录制 → 处理 → 编码 → 传输 → 解码 → 播放
   - 实时语音活动检测 (VAD)
   - 唤醒词检测
   - 音频质量测试

3. **优雅的资源管理**
   - 正确的错误处理
   - 完整的资源清理
   - 信号处理和优雅退出

## 演示模式

### 1. 基础模式 (basic)
```bash
./audio_service_demo_mac --mode basic --duration 10
```
- **功能**: 基础音频录制和播放
- **演示内容**: 
  - AudioService 标准初始化流程
  - 音频数据的编码和解码
  - 发送队列和解码队列的使用
- **适用场景**: 了解 AudioService 基本工作原理

### 2. VAD 模式 (vad)
```bash
./audio_service_demo_mac --mode vad --duration 30
```
- **功能**: 语音活动检测演示
- **演示内容**:
  - 实时语音活动检测
  - VAD 事件回调处理
  - 语音/静音状态统计
- **适用场景**: 需要语音检测功能的应用

### 3. 唤醒词模式 (wakeword)
```bash
export PICOVOICE_ACCESS_KEY="your_access_key_here"
./audio_service_demo_mac --mode wakeword --real-audio --duration 60
```
- **功能**: 基于 Picovoice Porcupine 的唤醒词检测演示
- **演示内容**:
  - 持续监听唤醒词 "porcupine"
  - 实时唤醒词检测回调
  - 检测统计信息和敏感度配置
  - Porcupine 引擎的完整集成
- **适用场景**: 智能音箱、语音助手等应用
- **注意事项**: 
  - 需要有效的 Picovoice 访问密钥
  - 建议使用真实音频设备以获得最佳效果
  - 默认唤醒词为 "porcupine"，敏感度为 0.5

### 4. 测试模式 (test)
```bash
./audio_service_demo_mac --mode test --duration 15
```
- **功能**: 音频测试和质量评估
- **演示内容**:
  - 音频数据收集
  - 测试队列管理
  - 音频质量分析
- **适用场景**: 音频系统调试和优化

### 5. 完整模式 (full)
```bash
./audio_service_demo_mac --mode full --real-audio --duration 45
```
- **功能**: 所有功能的综合演示
- **演示内容**:
  - 同时启用所有音频功能
  - 完整的音频处理管道
  - 综合性能统计
- **适用场景**: 完整功能验证和性能测试

## 命令行选项

| 选项 | 参数 | 默认值 | 说明 |
|------|------|--------|------|
| `--mode` | basic\|vad\|wakeword\|test\|full | basic | 演示模式 |
| `--duration` | 秒数 | 10 | 运行时长 |
| `--real-audio` | 无 | false | 使用真实音频设备 |
| `--no-wakeword` | 无 | false | 禁用唤醒词功能 |
| `--help` | 无 | - | 显示帮助信息 |

## 环境变量

| 变量名 | 说明 | 是否必需 |
|--------|------|----------|
| `PICOVOICE_ACCESS_KEY` | Picovoice 访问密钥，用于唤醒词功能 | 唤醒词模式时必需 |

## AudioService 最佳使用流程

### 1. 初始化阶段

```c
// 1. 创建配置并初始化为默认值
AudioServiceConfig config;
audio_service_config_init_default(&config);

// 2. 创建 AudioService 实例
AudioService* service = audio_service_create(&config);

// 3. 创建并设置平台特定组件
AudioInterface* audio_interface = create_platform_audio_interface();
AudioProcessor* audio_processor = create_platform_audio_processor();
audio_codec_t* opus_encoder = create_opus_encoder();
audio_codec_t* opus_decoder = create_opus_decoder();

audio_service_set_components(service, audio_interface, audio_processor, 
                           NULL, opus_encoder, opus_decoder);

// 4. 设置事件回调
AudioServiceCallbacks callbacks = {
    .on_send_queue_available = on_send_queue_callback,
    .on_wake_word_detected = on_wake_word_callback,
    .on_vad_change = on_vad_callback,
    .user_data = user_data
};
audio_service_set_callbacks(service, &callbacks);

// 5. 初始化服务
audio_service_initialize(service, opus_encoder);
```

### 2. 功能配置阶段

```c
// 配置需要的功能
AudioServiceFeatures features = {0};
features.voice_processing = true;           // 启用语音处理
features.voice_activity_detection = true;   // 启用VAD
features.wake_word_detection = true;        // 启用唤醒词检测
features.device_aec = true;                 // 启用回声消除
features.noise_suppression = true;          // 启用噪声抑制

// 应用配置
audio_service_configure_features(service, &features);
```

### 3. 运行阶段

```c
// 启动服务
audio_service_start(service);

// 主循环 - 处理音频数据和事件
while (running) {
    // 从发送队列获取编码数据
    AudioStreamPacket* packet = audio_service_pop_packet_from_send_queue(service);
    if (packet) {
        // 处理编码后的音频数据（如发送到网络）
        handle_encoded_audio(packet);
        audio_stream_packet_destroy(packet);
    }
    
    // 将接收到的数据放入解码队列
    if (received_audio_data) {
        AudioStreamPacket* decode_packet = create_packet_from_data(received_data);
        audio_service_push_packet_to_decode_queue(service, decode_packet, false);
    }
    
    // 检查服务状态
    if (audio_service_is_voice_detected(service)) {
        // 处理语音检测事件
    }
}
```

### 4. 清理阶段

```c
// 停止服务
audio_service_stop(service);

// 销毁服务和组件
audio_service_destroy(service);
cleanup_audio_components();
```

## 回调函数最佳实践

### 发送队列回调
```c
void on_send_queue_available(void* user_data) {
    // 及时处理编码后的音频数据
    AudioStreamPacket* packet = audio_service_pop_packet_from_send_queue(service);
    if (packet) {
        // 发送到网络或存储
        send_to_network(packet->payload, packet->payload_size);
        audio_stream_packet_destroy(packet);
    }
}
```

### VAD 状态回调
```c
void on_vad_change(bool speaking, void* user_data) {
    if (speaking) {
        // 开始语音处理逻辑
        start_speech_processing();
    } else {
        // 结束语音处理逻辑
        end_speech_processing();
    }
}
```

### 唤醒词检测回调
```c
void on_wake_word_detected(const char* wake_word, void* user_data) {
    // 处理唤醒词事件
    printf("检测到唤醒词: %s\n", wake_word);
    trigger_voice_assistant();
}
```

## 编译和运行

### 编译
```bash
cd /Users/sunqirui/gitlab/aiagent/linx-os-sdk
mkdir -p build && cd build
cmake ..
make audio_service_demo_mac
```

### 运行示例
```bash
# 基础演示
./audio_service_demo_mac

# VAD 演示，运行30秒
./audio_service_demo_mac --mode vad --duration 30

# 唤醒词演示，使用真实音频设备
export PICOVOICE_ACCESS_KEY="your_access_key_here"
./audio_service_demo_mac --mode wakeword --real-audio --duration 60

# 完整功能演示，使用真实音频设备
./audio_service_demo_mac --mode full --real-audio --duration 60

# 完整功能演示，但禁用唤醒词
./audio_service_demo_mac --mode full --real-audio --no-wakeword

# 显示帮助
./audio_service_demo_mac --help
```

## 输出示例

```
[INFO] AudioServiceDemo: 🎵 AudioService 演示程序启动
[INFO] AudioServiceDemo: 演示配置:
[INFO] AudioServiceDemo:   模式: VAD
[INFO] AudioServiceDemo:   持续时间: 30 秒
[INFO] AudioServiceDemo:   音频设备: 模拟设备
[INFO] AudioServiceDemo: 🔧 开始初始化AudioService...
[INFO] AudioServiceDemo: ✅ AudioService实例创建成功
[INFO] AudioServiceDemo: ✅ 音频组件创建成功
[INFO] AudioServiceDemo: ✅ 音频组件设置完成
[INFO] AudioServiceDemo: ✅ 回调函数设置完成
[INFO] AudioServiceDemo: ✅ AudioService初始化完成
[INFO] AudioServiceDemo: 🗣️  启动VAD语音活动检测演示
[INFO] AudioServiceDemo: VAD模式配置完成，请说话测试语音检测...
[INFO] AudioServiceDemo: ✅ AudioService启动成功
[INFO] AudioServiceDemo: 演示运行中，持续时间: 30秒...
[INFO] AudioServiceDemo: 🗣️  检测到语音活动 (语音事件: 1)
[INFO] AudioServiceDemo: 🔇 语音活动结束 (静音事件: 1)
[INFO] AudioServiceDemo: 运行状态 - 已发送: 15, 已接收: 0, VAD语音: 3, 唤醒词: 0
[INFO] AudioServiceDemo: 演示运行完成
[INFO] AudioServiceDemo: 
📊 演示统计信息:
[INFO] AudioServiceDemo: 运行时长: 30.0 秒
[INFO] AudioServiceDemo: 发送数据包: 45
[INFO] AudioServiceDemo: 接收数据包: 0
[INFO] AudioServiceDemo: VAD语音事件: 8
[INFO] AudioServiceDemo: VAD静音事件: 8
[INFO] AudioServiceDemo: 唤醒词检测: 0
[INFO] AudioServiceDemo: 平均数据包速率: 1.5 包/秒
[INFO] AudioServiceDemo: 🧹 开始清理资源...
[INFO] AudioServiceDemo: ✅ AudioService已清理
[INFO] AudioServiceDemo: ✅ 所有资源清理完成
[INFO] AudioServiceDemo: 🎵 AudioService 演示程序结束 (退出码: 0)
```

## 故障排除

### 常见问题

1. **编译错误**: 确保所有依赖库已正确安装
2. **音频设备错误**: 检查音频设备权限和可用性
3. **组件初始化失败**: 验证平台特定组件的实现

### 调试技巧

1. 使用 `--real-audio` 选项测试真实音频设备
2. 调整 `--duration` 参数进行长时间测试
3. 查看日志输出了解详细的执行流程
4. 使用不同模式测试特定功能

## 扩展开发

基于此演示程序，你可以：

1. **添加新的演示模式**: 在 `DemoMode` 枚举中添加新模式
2. **集成其他平台**: 实现平台特定的音频组件
3. **添加新功能**: 扩展 AudioService 的功能特性
4. **性能优化**: 基于统计信息进行性能调优

## 相关文档

- [AudioService API 参考](../audio_service.h)
- [音频组件开发指南](../README.md)
- [平台移植指南](../../board/README.md)