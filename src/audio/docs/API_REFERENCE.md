# LinxOS音频系统API参考

本文档提供LinxOS音频系统的完整API参考，包括所有公共接口、数据结构和使用示例。

## 目录

1. [核心类型和常量](#核心类型和常量)
2. [音频管理器API](#音频管理器api)
3. [流管理器API](#流管理器api)
4. [事件总线API](#事件总线api)
5. [插件管理器API](#插件管理器api)
6. [音频管道API](#音频管道api)
7. [驱动接口API](#驱动接口api)
8. [工具函数API](#工具函数api)
9. [测试框架API](#测试框架api)

---

## 核心类型和常量

### 错误代码

```c
typedef enum {
    AUDIO_RESULT_SUCCESS = 0,           // 操作成功
    AUDIO_RESULT_ERROR,                 // 一般错误
    AUDIO_RESULT_INVALID_PARAMETER,     // 无效参数
    AUDIO_RESULT_OUT_OF_MEMORY,         // 内存不足
    AUDIO_RESULT_NOT_INITIALIZED,       // 未初始化
    AUDIO_RESULT_ALREADY_INITIALIZED,   // 已初始化
    AUDIO_RESULT_NOT_SUPPORTED,         // 不支持
    AUDIO_RESULT_DEVICE_NOT_FOUND,      // 设备未找到
    AUDIO_RESULT_DEVICE_BUSY,           // 设备忙
    AUDIO_RESULT_FORMAT_MISMATCH,       // 格式不匹配
    AUDIO_RESULT_BUFFER_TOO_SMALL,      // 缓冲区太小
    AUDIO_RESULT_BUFFER_OVERFLOW,       // 缓冲区溢出
    AUDIO_RESULT_TIMEOUT,               // 超时
    AUDIO_RESULT_INTERRUPTED,           // 中断
    AUDIO_RESULT_IO_ERROR,              // IO错误
    AUDIO_RESULT_PERMISSION_DENIED,     // 权限拒绝
    AUDIO_RESULT_UNSUPPORTED_FORMAT,    // 不支持的格式
} audio_result_t;
```

### 音频格式

```c
typedef enum {
    AUDIO_FORMAT_UNKNOWN = 0,
    AUDIO_FORMAT_U8,                    // 8位无符号
    AUDIO_FORMAT_S16_LE,                // 16位有符号小端
    AUDIO_FORMAT_S16_BE,                // 16位有符号大端
    AUDIO_FORMAT_S24_LE,                // 24位有符号小端
    AUDIO_FORMAT_S24_BE,                // 24位有符号大端
    AUDIO_FORMAT_S32_LE,                // 32位有符号小端
    AUDIO_FORMAT_S32_BE,                // 32位有符号大端
    AUDIO_FORMAT_F32_LE,                // 32位浮点小端
    AUDIO_FORMAT_F32_BE,                // 32位浮点大端
    AUDIO_FORMAT_F64_LE,                // 64位浮点小端
    AUDIO_FORMAT_F64_BE,                // 64位浮点大端
} audio_format_t;
```

### 通道布局

```c
typedef enum {
    AUDIO_CHANNEL_LAYOUT_UNKNOWN = 0,
    AUDIO_CHANNEL_LAYOUT_MONO,          // 单声道
    AUDIO_CHANNEL_LAYOUT_STEREO,        // 立体声
    AUDIO_CHANNEL_LAYOUT_2_1,           // 2.1声道
    AUDIO_CHANNEL_LAYOUT_3_0,           // 3.0声道
    AUDIO_CHANNEL_LAYOUT_3_1,           // 3.1声道
    AUDIO_CHANNEL_LAYOUT_4_0,           // 4.0声道
    AUDIO_CHANNEL_LAYOUT_4_1,           // 4.1声道
    AUDIO_CHANNEL_LAYOUT_5_0,           // 5.0声道
    AUDIO_CHANNEL_LAYOUT_5_1,           // 5.1声道
    AUDIO_CHANNEL_LAYOUT_6_0,           // 6.0声道
    AUDIO_CHANNEL_LAYOUT_6_1,           // 6.1声道
    AUDIO_CHANNEL_LAYOUT_7_0,           // 7.0声道
    AUDIO_CHANNEL_LAYOUT_7_1,           // 7.1声道
} audio_channel_layout_t;
```

### 音频格式信息

```c
typedef struct {
    audio_format_t format;              // 音频格式
    uint32_t sample_rate;               // 采样率
    audio_channel_layout_t channel_layout; // 通道布局
} audio_format_info_t;
```

### 音频缓冲区

```c
typedef struct {
    void* data;                         // 音频数据指针
    uint32_t size;                      // 缓冲区大小（字节）
    uint32_t frame_count;               // 音频帧数
    audio_format_info_t format;         // 音频格式信息
    uint64_t timestamp;                 // 时间戳（微秒）
    uint32_t flags;                     // 缓冲区标志
} audio_buffer_t;
```

### 流类型

```c
typedef enum {
    AUDIO_STREAM_TYPE_UNKNOWN = 0,
    AUDIO_STREAM_TYPE_PLAYBACK,         // 播放流
    AUDIO_STREAM_TYPE_CAPTURE,          // 录制流
    AUDIO_STREAM_TYPE_DUPLEX,           // 双工流
} audio_stream_type_t;
```

### 流状态

```c
typedef enum {
    AUDIO_STREAM_STATE_STOPPED = 0,     // 已停止
    AUDIO_STREAM_STATE_STARTING,        // 启动中
    AUDIO_STREAM_STATE_RUNNING,         // 运行中
    AUDIO_STREAM_STATE_PAUSED,          // 已暂停
    AUDIO_STREAM_STATE_STOPPING,        // 停止中
    AUDIO_STREAM_STATE_ERROR,           // 错误状态
} audio_stream_state_t;
```

### 事件类型

```c
typedef enum {
    AUDIO_EVENT_UNKNOWN = 0,
    AUDIO_EVENT_STREAM_STARTED,         // 流已启动
    AUDIO_EVENT_STREAM_STOPPED,         // 流已停止
    AUDIO_EVENT_STREAM_PAUSED,          // 流已暂停
    AUDIO_EVENT_STREAM_RESUMED,         // 流已恢复
    AUDIO_EVENT_STREAM_ERROR,           // 流错误
    AUDIO_EVENT_DEVICE_CONNECTED,       // 设备连接
    AUDIO_EVENT_DEVICE_DISCONNECTED,    // 设备断开
    AUDIO_EVENT_DEVICE_CHANGED,         // 设备变更
    AUDIO_EVENT_BUFFER_UNDERRUN,        // 缓冲区下溢
    AUDIO_EVENT_BUFFER_OVERRUN,         // 缓冲区上溢
    AUDIO_EVENT_FORMAT_CHANGED,         // 格式变更
    AUDIO_EVENT_VOLUME_CHANGED,         // 音量变更
} audio_event_type_t;
```

### 插件类型

```c
typedef enum {
    AUDIO_PLUGIN_TYPE_UNKNOWN = 0,
    AUDIO_PLUGIN_TYPE_EFFECT,           // 音频效果
    AUDIO_PLUGIN_TYPE_CODEC,            // 编解码器
    AUDIO_PLUGIN_TYPE_ANALYZER,         // 分析器
    AUDIO_PLUGIN_TYPE_GENERATOR,        // 信号生成器
    AUDIO_PLUGIN_TYPE_FILTER,           // 滤波器
    AUDIO_PLUGIN_TYPE_MIXER,            // 混音器
} audio_plugin_type_t;
```

### 线程优先级

```c
typedef enum {
    AUDIO_THREAD_PRIORITY_LOW = 0,      // 低优先级
    AUDIO_THREAD_PRIORITY_NORMAL,       // 普通优先级
    AUDIO_THREAD_PRIORITY_HIGH,         // 高优先级
    AUDIO_THREAD_PRIORITY_REALTIME,     // 实时优先级
} audio_thread_priority_t;
```

### 设备信息

```c
typedef struct {
    uint32_t device_id;                 // 设备ID
    char name[256];                     // 设备名称
    char description[512];              // 设备描述
    audio_stream_type_t supported_types; // 支持的流类型
    uint32_t max_input_channels;        // 最大输入通道数
    uint32_t max_output_channels;       // 最大输出通道数
    uint32_t min_sample_rate;           // 最小采样率
    uint32_t max_sample_rate;           // 最大采样率
    audio_format_t* supported_formats;  // 支持的格式列表
    uint32_t format_count;              // 格式数量
    bool is_default;                    // 是否为默认设备
    bool is_available;                  // 是否可用
} audio_device_info_t;
```

### 系统配置

```c
typedef struct {
    uint32_t max_streams;               // 最大流数量
    uint32_t buffer_size_frames;        // 缓冲区大小（帧）
    uint32_t buffer_count;              // 缓冲区数量
    uint32_t sample_rate;               // 默认采样率
    audio_thread_priority_t thread_priority; // 线程优先级
    bool enable_real_time;              // 启用实时处理
    bool enable_power_management;       // 启用电源管理
    bool enable_debug_logging;          // 启用调试日志
    const char* plugin_directory;       // 插件目录
    const char* config_file;            // 配置文件路径
    void* platform_data;               // 平台特定数据
} audio_system_config_t;
```

### 流配置

```c
typedef struct {
    uint32_t device_id;                 // 设备ID
    audio_stream_type_t type;           // 流类型
    audio_format_info_t format;         // 音频格式
    uint32_t buffer_size_frames;        // 缓冲区大小（帧）
    uint32_t buffer_count;              // 缓冲区数量
    audio_data_callback_t data_callback; // 数据回调
    audio_event_callback_t event_callback; // 事件回调
    void* user_data;                    // 用户数据
    const char* stream_name;            // 流名称
    bool exclusive_mode;                // 独占模式
    uint32_t latency_ms;                // 期望延迟（毫秒）
    float volume;                       // 初始音量
    bool muted;                         // 是否静音
} audio_stream_config_t;
```

### 回调函数类型

```c
// 音频数据回调
typedef audio_result_t (*audio_data_callback_t)(
    const audio_buffer_t* input,        // 输入缓冲区
    audio_buffer_t* output,             // 输出缓冲区
    void* user_data                     // 用户数据
);

// 事件回调
typedef void (*audio_event_callback_t)(
    audio_event_type_t event_type,      // 事件类型
    const void* event_data,             // 事件数据
    void* user_data                     // 用户数据
);

// 错误回调
typedef void (*audio_error_callback_t)(
    audio_result_t error_code,          // 错误代码
    const char* error_message,          // 错误消息
    void* user_data                     // 用户数据
);
```

### 音频事件结构

```c
typedef struct {
    audio_event_type_t type;            // 事件类型
    uint64_t timestamp;                 // 时间戳
    uint32_t stream_id;                 // 流ID
    union {
        struct {
            audio_result_t error_code;   // 错误代码
            char message[256];           // 错误消息
        } error;
        struct {
            uint32_t device_id;          // 设备ID
            char device_name[256];       // 设备名称
        } device;
        struct {
            audio_format_info_t old_format; // 旧格式
            audio_format_info_t new_format; // 新格式
        } format_change;
        struct {
            float old_volume;            // 旧音量
            float new_volume;            // 新音量
        } volume_change;
    } data;
} audio_event_t;
```

### 统计信息

```c
typedef struct {
    uint64_t frames_processed;          // 已处理帧数
    uint64_t bytes_processed;           // 已处理字节数
    uint32_t buffer_underruns;          // 缓冲区下溢次数
    uint32_t buffer_overruns;           // 缓冲区上溢次数
    uint32_t callback_errors;           // 回调错误次数
    float cpu_usage;                    // CPU使用率
    uint32_t current_latency_ms;        // 当前延迟（毫秒）
    uint32_t min_latency_ms;            // 最小延迟（毫秒）
    uint32_t max_latency_ms;            // 最大延迟（毫秒）
    uint64_t total_runtime_ms;          // 总运行时间（毫秒）
} audio_statistics_t;
```

---

## 音频管理器API

音频管理器是系统的核心入口点，负责整个音频系统的生命周期管理。

### 创建和销毁

```c
/**
 * 创建音频管理器实例
 * @return 音频管理器指针，失败返回NULL
 */
audio_manager_t* audio_manager_create(void);

/**
 * 销毁音频管理器实例
 * @param manager 音频管理器指针
 */
void audio_manager_destroy(audio_manager_t* manager);
```

**使用示例：**
```c
// 创建管理器
audio_manager_t* manager = audio_manager_create();
if (!manager) {
    fprintf(stderr, "Failed to create audio manager\n");
    return -1;
}

// 使用管理器...

// 销毁管理器
audio_manager_destroy(manager);
```

### 生命周期管理

```c
/**
 * 初始化音频管理器
 * @param manager 音频管理器指针
 * @param config 系统配置，NULL使用默认配置
 * @return 操作结果
 */
audio_result_t audio_manager_initialize(audio_manager_t* manager,
                                        const audio_system_config_t* config);

/**
 * @brief 获取事件总线实例
 * @param manager 音频管理器指针
 * @return 事件总线指针
 */
event_bus_t* audio_manager_get_event_bus(audio_manager_t* manager);

/**
 * 反初始化音频管理器
 * @param manager 音频管理器指针
 * @return 操作结果
 */
audio_result_t audio_manager_deinitialize(audio_manager_t* manager);

/**
 * 启动音频管理器
 * @param manager 音频管理器指针
 * @return 操作结果
 */
audio_result_t audio_manager_start(audio_manager_t* manager);

/**
 * 停止音频管理器
 * @param manager 音频管理器指针
 * @return 操作结果
 */
audio_result_t audio_manager_stop(audio_manager_t* manager);

/**
 * 获取管理器状态
 * @param manager 音频管理器指针
 * @return 管理器状态
 */
audio_manager_state_t audio_manager_get_state(const audio_manager_t* manager);
```

**使用示例：**
```c
// 配置系统
audio_system_config_t config = {0};
audio_manager_get_default_config(&config);
config.max_streams = 32;
config.buffer_size_frames = 1024;
config.enable_real_time = true;

// 初始化
audio_result_t result = audio_manager_initialize(manager, &config);
if (result != AUDIO_RESULT_SUCCESS) {
    fprintf(stderr, "Failed to initialize: %s\n", 
            audio_result_to_string(result));
    return -1;
}

// 启动
result = audio_manager_start(manager);
if (result != AUDIO_RESULT_SUCCESS) {
    fprintf(stderr, "Failed to start: %s\n", 
            audio_result_to_string(result));
    return -1;
}
```

### 设备管理

```c
/**
 * 枚举输入设备
 * @param manager 音频管理器指针
 * @param devices 设备信息数组指针
 * @param count 设备数量指针
 * @return 操作结果
 */
audio_result_t audio_manager_enumerate_input_devices(
    const audio_manager_t* manager,
    audio_device_info_t** devices,
    uint32_t* count);

/**
 * 枚举输出设备
 * @param manager 音频管理器指针
 * @param devices 设备信息数组指针
 * @param count 设备数量指针
 * @return 操作结果
 */
audio_result_t audio_manager_enumerate_output_devices(
    const audio_manager_t* manager,
    audio_device_info_t** devices,
    uint32_t* count);

/**
 * 获取默认输入设备
 * @param manager 音频管理器指针
 * @param device_id 设备ID指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_default_input_device(
    const audio_manager_t* manager,
    uint32_t* device_id);

/**
 * 获取默认输出设备
 * @param manager 音频管理器指针
 * @param device_id 设备ID指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_default_output_device(
    const audio_manager_t* manager,
    uint32_t* device_id);

/**
 * 获取设备信息
 * @param manager 音频管理器指针
 * @param device_id 设备ID
 * @param device_info 设备信息指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_device_info(
    const audio_manager_t* manager,
    uint32_t device_id,
    audio_device_info_t* device_info);
```

**使用示例：**
```c
// 枚举输出设备
audio_device_info_t* devices = NULL;
uint32_t device_count = 0;

audio_result_t result = audio_manager_enumerate_output_devices(
    manager, &devices, &device_count);

if (result == AUDIO_RESULT_SUCCESS) {
    printf("Found %u output devices:\n", device_count);
    for (uint32_t i = 0; i < device_count; i++) {
        printf("  %u: %s (%s)\n", 
               devices[i].device_id,
               devices[i].name,
               devices[i].description);
    }
    
    // 释放设备列表
    audio_manager_free_device_list(devices, device_count);
}
```

### 流管理

```c
/**
 * 创建音频流
 * @param manager 音频管理器指针
 * @param config 流配置
 * @param stream 流指针的指针
 * @return 操作结果
 */
audio_result_t audio_manager_create_stream(
    audio_manager_t* manager,
    const audio_stream_config_t* config,
    audio_stream_t** stream);

/**
 * 销毁音频流
 * @param manager 音频管理器指针
 * @param stream 流指针
 * @return 操作结果
 */
audio_result_t audio_manager_destroy_stream(
    audio_manager_t* manager,
    audio_stream_t* stream);

/**
 * 获取活动流列表
 * @param manager 音频管理器指针
 * @param streams 流指针数组
 * @param count 流数量指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_active_streams(
    const audio_manager_t* manager,
    audio_stream_t*** streams,
    uint32_t* count);
```

### 配置管理

```c
/**
 * 获取默认系统配置
 * @param config 配置结构指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_default_config(audio_system_config_t* config);

/**
 * 设置系统配置
 * @param manager 音频管理器指针
 * @param config 配置结构指针
 * @return 操作结果
 */
audio_result_t audio_manager_set_config(
    audio_manager_t* manager,
    const audio_system_config_t* config);

/**
 * 获取当前系统配置
 * @param manager 音频管理器指针
 * @param config 配置结构指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_config(
    const audio_manager_t* manager,
    audio_system_config_t* config);
```

### 统计信息

```c
/**
 * 获取系统统计信息
 * @param manager 音频管理器指针
 * @param stats 统计信息指针
 * @return 操作结果
 */
audio_result_t audio_manager_get_statistics(
    const audio_manager_t* manager,
    audio_statistics_t* stats);

/**
 * 重置统计信息
 * @param manager 音频管理器指针
 * @return 操作结果
 */
audio_result_t audio_manager_reset_statistics(audio_manager_t* manager);
```

---

## 音频流API

音频流是音频数据处理的基本单元，支持播放、录制和双工操作。

### 流控制

```c
/**
 * 启动音频流
 * @param stream 音频流指针
 * @return 操作结果
 */
audio_result_t audio_stream_start(audio_stream_t* stream);

/**
 * 停止音频流
 * @param stream 音频流指针
 * @return 操作结果
 */
audio_result_t audio_stream_stop(audio_stream_t* stream);

/**
 * 暂停音频流
 * @param stream 音频流指针
 * @return 操作结果
 */
audio_result_t audio_stream_pause(audio_stream_t* stream);

/**
 * 恢复音频流
 * @param stream 音频流指针
 * @return 操作结果
 */
audio_result_t audio_stream_resume(audio_stream_t* stream);

/**
 * 刷新音频流缓冲区
 * @param stream 音频流指针
 * @return 操作结果
 */
audio_result_t audio_stream_flush(audio_stream_t* stream);
```

### 状态查询

```c
/**
 * 获取流状态
 * @param stream 音频流指针
 * @return 流状态
 */
audio_stream_state_t audio_stream_get_state(const audio_stream_t* stream);

/**
 * 获取流位置
 * @param stream 音频流指针
 * @param position 位置指针（帧数）
 * @return 操作结果
 */
audio_result_t audio_stream_get_position(
    const audio_stream_t* stream,
    uint64_t* position);

/**
 * 获取流延迟
 * @param stream 音频流指针
 * @param latency_ms 延迟指针（毫秒）
 * @return 操作结果
 */
audio_result_t audio_stream_get_latency(
    const audio_stream_t* stream,
    uint32_t* latency_ms);

/**
 * 获取流信息
 * @param stream 音频流指针
 * @param info 流信息指针
 * @return 操作结果
 */
audio_result_t audio_stream_get_info(
    const audio_stream_t* stream,
    audio_stream_info_t* info);
```

### 音量控制

```c
/**
 * 设置流音量
 * @param stream 音频流指针
 * @param volume 音量值（0.0-1.0）
 * @return 操作结果
 */
audio_result_t audio_stream_set_volume(audio_stream_t* stream, float volume);

/**
 * 获取流音量
 * @param stream 音频流指针
 * @param volume 音量指针
 * @return 操作结果
 */
audio_result_t audio_stream_get_volume(
    const audio_stream_t* stream,
    float* volume);

/**
 * 设置静音状态
 * @param stream 音频流指针
 * @param muted 是否静音
 * @return 操作结果
 */
audio_result_t audio_stream_set_mute(audio_stream_t* stream, bool muted);

/**
 * 获取静音状态
 * @param stream 音频流指针
 * @param muted 静音状态指针
 * @return 操作结果
 */
audio_result_t audio_stream_get_mute(
    const audio_stream_t* stream,
    bool* muted);
```

### 数据操作

```c
/**
 * 读取音频数据（录制流）
 * @param stream 音频流指针
 * @param buffer 数据缓冲区
 * @param timeout_ms 超时时间（毫秒）
 * @return 操作结果
 */
audio_result_t audio_stream_read(
    audio_stream_t* stream,
    audio_buffer_t* buffer,
    uint32_t timeout_ms);

/**
 * 写入音频数据（播放流）
 * @param stream 音频流指针
 * @param buffer 数据缓冲区
 * @param timeout_ms 超时时间（毫秒）
 * @return 操作结果
 */
audio_result_t audio_stream_write(
    audio_stream_t* stream,
    const audio_buffer_t* buffer,
    uint32_t timeout_ms);

/**
 * 获取可用缓冲区数量
 * @param stream 音频流指针
 * @param available 可用缓冲区数量指针
 * @return 操作结果
 */
audio_result_t audio_stream_get_available_frames(
    const audio_stream_t* stream,
    uint32_t* available);
```

**使用示例：**
```c
// 创建播放流
audio_stream_config_t config = {0};
config.device_id = 0;  // 默认设备
config.type = AUDIO_STREAM_TYPE_PLAYBACK;
config.format.format = AUDIO_FORMAT_F32_LE;
config.format.sample_rate = 44100;
config.format.channel_layout = AUDIO_CHANNEL_LAYOUT_STEREO;
config.buffer_size_frames = 1024;
config.data_callback = my_audio_callback;
config.user_data = my_data;

audio_stream_t* stream;
audio_result_t result = audio_manager_create_stream(manager, &config, &stream);

if (result == AUDIO_RESULT_SUCCESS) {
    // 启动流
    audio_stream_start(stream);
    
    // 设置音量
    audio_stream_set_volume(stream, 0.8f);
    
    // 播放一段时间
    sleep(5);
    
    // 停止并销毁流
    audio_stream_stop(stream);
    audio_manager_destroy_stream(manager, stream);
}
```
    uint32_t frame_count;               // 帧数
    uint32_t capacity;                  // 容量（帧）
    audio_format_info_t format;         // 音频格式
    struct timespec timestamp;          // 时间戳
} audio_buffer_t;
```

---

## 音频管理器API

音频管理器是整个音频系统的核心，负责系统级别的管理和协调。

### 创建和销毁

```c
/**
 * @brief 创建音频管理器
 * @param config 管理器配置
 * @return 音频管理器实例，失败返回NULL
 */
audio_manager_t* audio_manager_create(const audio_manager_config_t* config);

/**
 * @brief 销毁音频管理器
 * @param manager 音频管理器
 */
void audio_manager_destroy(audio_manager_t* manager);
```

### 生命周期管理

```c
/**
 * @brief 初始化音频管理器
 * @param manager 音频管理器
 * @return 操作结果
 */
audio_result_t audio_manager_initialize(audio_manager_t* manager);

/**
 * @brief 反初始化音频管理器
 * @param manager 音频管理器
 * @return 操作结果
 */
audio_result_t audio_manager_deinitialize(audio_manager_t* manager);

/**
 * @brief 启动音频管理器
 * @param manager 音频管理器
 * @return 操作结果
 */
audio_result_t audio_manager_start(audio_manager_t* manager);

/**
 * @brief 停止音频管理器
 * @param manager 音频管理器
 * @return 操作结果
 */
audio_result_t audio_manager_stop(audio_manager_t* manager);
```

### 设备管理

```c
/**
 * @brief 枚举音频设备
 * @param manager 音频管理器
 * @param direction 设备方向（输入/输出）
 * @param devices 设备信息数组（输出）
 * @param device_count 设备数量（输入/输出）
 * @return 操作结果
 */
audio_result_t audio_manager_enumerate_devices(audio_manager_t* manager,
                                             audio_device_direction_t direction,
                                             audio_device_info_t** devices,
                                             uint32_t* device_count);

/**
 * @brief 获取默认设备
 * @param manager 音频管理器
 * @param direction 设备方向
 * @param device_id 设备ID（输出）
 * @return 操作结果
 */
audio_result_t audio_manager_get_default_device(audio_manager_t* manager,
                                               audio_device_direction_t direction,
                                               uint32_t* device_id);

/**
 * @brief 获取设备信息
 * @param manager 音频管理器
 * @param device_id 设备ID
 * @param device_info 设备信息（输出）
 * @return 操作结果
 */
audio_result_t audio_manager_get_device_info(audio_manager_t* manager,
                                            uint32_t device_id,
                                            audio_device_info_t* device_info);
```

**使用示例：**
```c
// 枚举输出设备
audio_device_info_t* devices = NULL;
uint32_t device_count = 0;

audio_result_t result = audio_manager_enumerate_devices(
    manager, AUDIO_DEVICE_DIRECTION_OUTPUT, &devices, &device_count);

if (result == AUDIO_RESULT_SUCCESS) {
    printf("Found %u output devices:\n", device_count);
    for (uint32_t i = 0; i < device_count; i++) {
        printf("  Device %u: %s\n", devices[i].device_id, devices[i].name);
        printf("    Channels: %u\n", devices[i].max_output_channels);
        printf("    Sample Rate: %u-%u Hz\n", 
               devices[i].min_sample_rate, devices[i].max_sample_rate);
    }
    
    // 释放设备列表
    audio_manager_free_device_list(devices, device_count);
}
```

---

 * @return 操作结果
 */
audio_result_t audio_event_publish(audio_manager_t* manager,
                                  const audio_event_t* event);
```

**使用示例：**
```c
// 事件回调函数
void my_event_callback(audio_event_type_t event_type,
                      const void* event_data,
                      void* user_data) {
    switch (event_type) {
        case AUDIO_EVENT_DEVICE_CONNECTED:
            printf("Audio device connected\n");
            break;
        case AUDIO_EVENT_DEVICE_DISCONNECTED:
            printf("Audio device disconnected\n");
            break;
        case AUDIO_EVENT_STREAM_ERROR:
            printf("Audio stream error occurred\n");
            break;
        default:
            break;
    }
}

// 订阅设备事件
uint32_t subscription_id;
audio_result_t result = audio_event_subscribe(
    manager,
    AUDIO_EVENT_DEVICE_CONNECTED,
    my_event_callback,
    NULL,
    &subscription_id);

if (result == AUDIO_RESULT_SUCCESS) {
    // 事件已订阅，系统会在设备连接时调用回调
    
    // 稍后取消订阅
    audio_event_unsubscribe(manager, subscription_id);
}
```

---

## 插件管理器API

插件管理器支持动态加载和管理音频插件。

### 插件加载

```c
/**
 * @brief 加载插件
 * @param manager 音频管理器
 * @param plugin_path 插件路径
 * @param plugin_id 插件ID（输出）
 * @return 操作结果
 */
audio_result_t audio_plugin_load(audio_manager_t* manager,
                                const char* plugin_path,
                                uint32_t* plugin_id);

/**
 * @brief 卸载插件
 * @param manager 音频管理器
 * @param plugin_id 插件ID
 * @return 操作结果
 */
audio_result_t audio_plugin_unload(audio_manager_t* manager,
                                  uint32_t plugin_id);

/**
 * @brief 枚举已加载的插件
 * @param manager 音频管理器
 * @param plugins 插件信息数组（输出）
 * @param plugin_count 插件数量（输入/输出）
 * @return 操作结果
 */
audio_result_t audio_plugin_enumerate(audio_manager_t* manager,
                                     audio_plugin_info_t** plugins,
                                     uint32_t* plugin_count);
```

### 插件实例化

```c
/**
 * @brief 创建插件实例
 * @param manager 音频管理器
 * @param plugin_id 插件ID
 * @param config 插件配置
 * @param instance 插件实例（输出）
 * @return 操作结果
 */
audio_result_t audio_plugin_create_instance(audio_manager_t* manager,
                                           uint32_t plugin_id,
                                           const void* config,
                                           audio_plugin_instance_t** instance);

/**
 * @brief 销毁插件实例
 * @param instance 插件实例
 * @return 操作结果
 */
audio_result_t audio_plugin_destroy_instance(audio_plugin_instance_t* instance);

/**
 * @brief 处理音频数据
 * @param instance 插件实例
 * @param input 输入缓冲区
 * @param output 输出缓冲区
 * @return 操作结果
 */
audio_result_t audio_plugin_process(audio_plugin_instance_t* instance,
                                   const audio_buffer_t* input,
                                   audio_buffer_t* output);
```

**使用示例：**
```c
// 加载混响插件
uint32_t plugin_id;
audio_result_t result = audio_plugin_load(
    manager, "/usr/lib/audio/plugins/reverb.so", &plugin_id);

if (result == AUDIO_RESULT_SUCCESS) {
    // 创建插件实例
    reverb_config_t reverb_config = {
        .room_size = 0.5f,
        .damping = 0.3f,
        .wet_level = 0.2f,
        .dry_level = 0.8f
    };
    
    audio_plugin_instance_t* reverb_instance;
    result = audio_plugin_create_instance(
        manager, plugin_id, &reverb_config, &reverb_instance);
    
    if (result == AUDIO_RESULT_SUCCESS) {
        // 在音频回调中使用插件
        // audio_plugin_process(reverb_instance, input, output);
        
        // 销毁实例
        audio_plugin_destroy_instance(reverb_instance);
    }
    
    // 卸载插件
    audio_plugin_unload(manager, plugin_id);
}
```

---

## 音频管道API

音频管道提供高级的音频处理链功能。

### 管道创建

```c
/**
 * @brief 创建音频管道
 * @param manager 音频管理器
 * @param config 管道配置
 * @param pipeline 音频管道（输出）
 * @return 操作结果
 */
audio_result_t audio_pipeline_create(audio_manager_t* manager,
                                    const audio_pipeline_config_t* config,
                                    audio_pipeline_t** pipeline);

/**
 * @brief 销毁音频管道
 * @param pipeline 音频管道
 * @return 操作结果
 */
audio_result_t audio_pipeline_destroy(audio_pipeline_t* pipeline);
```

### 节点管理

```c
/**
 * @brief 添加处理节点
 * @param pipeline 音频管道
 * @param node_config 节点配置
 * @param node_id 节点ID（输出）
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_node(audio_pipeline_t* pipeline,
                                      const audio_node_config_t* node_config,
                                      uint32_t* node_id);

/**
 * @brief 移除处理节点
 * @param pipeline 音频管道
 * @param node_id 节点ID
 * @return 操作结果
 */
audio_result_t audio_pipeline_remove_node(audio_pipeline_t* pipeline,
                                         uint32_t node_id);

/**
 * @brief 连接节点
 * @param pipeline 音频管道
 * @param source_node_id 源节点ID
 * @param dest_node_id 目标节点ID
 * @return 操作结果
 */
audio_result_t audio_pipeline_connect_nodes(audio_pipeline_t* pipeline,
                                           uint32_t source_node_id,
                                           uint32_t dest_node_id);

/**
 * @brief 断开节点连接
 * @param pipeline 音频管道
 * @param source_node_id 源节点ID
 * @param dest_node_id 目标节点ID
 * @return 操作结果
 */
audio_result_t audio_pipeline_disconnect_nodes(audio_pipeline_t* pipeline,
                                              uint32_t source_node_id,
                                              uint32_t dest_node_id);
```

### 管道控制

```c
/**
 * @brief 启动音频管道
 * @param pipeline 音频管道
 * @return 操作结果
 */
audio_result_t audio_pipeline_start(audio_pipeline_t* pipeline);

/**
 * @brief 停止音频管道
 * @param pipeline 音频管道
 * @return 操作结果
 */
audio_result_t audio_pipeline_stop(audio_pipeline_t* pipeline);

/**
 * @brief 暂停音频管道
 * @param pipeline 音频管道
 * @return 操作结果
 */
audio_result_t audio_pipeline_pause(audio_pipeline_t* pipeline);

/**
 * @brief 恢复音频管道
 * @param pipeline 音频管道
 * @return 操作结果
 */
audio_result_t audio_pipeline_resume(audio_pipeline_t* pipeline);
```

**使用示例：**
```c
// 创建音频处理管道
audio_pipeline_config_t pipeline_config = {0};
pipeline_config.max_nodes = 16;
pipeline_config.buffer_size_frames = 1024;

audio_pipeline_t* pipeline;
audio_result_t result = audio_pipeline_create(manager, &pipeline_config, &pipeline);

if (result == AUDIO_RESULT_SUCCESS) {
    // 添加输入节点
    audio_node_config_t input_config = {0};
    input_config.type = AUDIO_NODE_TYPE_INPUT;
    input_config.device_id = 0;
    
    uint32_t input_node_id;
    audio_pipeline_add_node(pipeline, &input_config, &input_node_id);
    
    // 添加均衡器节点
    audio_node_config_t eq_config = {0};
    eq_config.type = AUDIO_NODE_TYPE_EQUALIZER;
    eq_config.plugin_id = equalizer_plugin_id;
    
    uint32_t eq_node_id;
    audio_pipeline_add_node(pipeline, &eq_config, &eq_node_id);
    
    // 添加输出节点
    audio_node_config_t output_config = {0};
    output_config.type = AUDIO_NODE_TYPE_OUTPUT;
    output_config.device_id = 0;
    
    uint32_t output_node_id;
    audio_pipeline_add_node(pipeline, &output_config, &output_node_id);
    
    // 连接节点：输入 -> 均衡器 -> 输出
    audio_pipeline_connect_nodes(pipeline, input_node_id, eq_node_id);
    audio_pipeline_connect_nodes(pipeline, eq_node_id, output_node_id);
    
    // 启动管道
    audio_pipeline_start(pipeline);
    
    // 运行一段时间
    sleep(10);
    
    // 停止并销毁管道
    audio_pipeline_stop(pipeline);
    audio_pipeline_destroy(pipeline);
}
```

---

## 驱动接口API

驱动接口提供与底层音频硬件的交互能力。

### 驱动注册

```c
/**
 * @brief 注册音频驱动
 * @param manager 音频管理器
 * @param driver_info 驱动信息
 * @return 操作结果
 */
audio_result_t audio_driver_register(audio_manager_t* manager,
                                    const audio_driver_info_t* driver_info);

/**
 * @brief 注销音频驱动
 * @param manager 音频管理器
 * @param driver_name 驱动名称
 * @return 操作结果
 */
audio_result_t audio_driver_unregister(audio_manager_t* manager,
                                      const char* driver_name);

/**
 * @brief 枚举已注册的驱动
 * @param manager 音频管理器
 * @param drivers 驱动信息数组（输出）
 * @param driver_count 驱动数量（输入/输出）
 * @return 操作结果
 */
audio_result_t audio_driver_enumerate(audio_manager_t* manager,
                                     audio_driver_info_t** drivers,
                                     uint32_t* driver_count);
```

### 驱动控制

```c
/**
 * @brief 初始化驱动
 * @param manager 音频管理器
 * @param driver_name 驱动名称
 * @param config 驱动配置
 * @return 操作结果
 */
audio_result_t audio_driver_initialize(audio_manager_t* manager,
                                      const char* driver_name,
                                      const void* config);

/**
 * @brief 反初始化驱动
 * @param manager 音频管理器
 * @param driver_name 驱动名称
 * @return 操作结果
 */
audio_result_t audio_driver_deinitialize(audio_manager_t* manager,
                                        const char* driver_name);

/**
 * @brief 获取驱动状态
 * @param manager 音频管理器
 * @param driver_name 驱动名称
 * @param status 驱动状态（输出）
 * @return 操作结果
 */
audio_result_t audio_driver_get_status(audio_manager_t* manager,
                                      const char* driver_name,
                                      audio_driver_status_t* status);
```

---

## 工具函数API

提供各种音频处理和转换的工具函数。

### 格式转换

```c
/**
 * @brief 转换音频格式
 * @param input 输入缓冲区
 * @param output 输出缓冲区
 * @param target_format 目标格式
 * @return 操作结果
 */
audio_result_t audio_convert_format(const audio_buffer_t* input,
                                   audio_buffer_t* output,
                                   const audio_format_info_t* target_format);

/**
 * @brief 重采样音频
 * @param input 输入缓冲区
 * @param output 输出缓冲区
 * @param target_sample_rate 目标采样率
 * @param quality 重采样质量
 * @return 操作结果
 */
audio_result_t audio_resample(const audio_buffer_t* input,
                             audio_buffer_t* output,
                             uint32_t target_sample_rate,
                             audio_resample_quality_t quality);

/**
 * @brief 混合音频通道
 * @param input 输入缓冲区
 * @param output 输出缓冲区
 * @param target_layout 目标通道布局
 * @return 操作结果
 */
audio_result_t audio_mix_channels(const audio_buffer_t* input,
                                 audio_buffer_t* output,
                                 audio_channel_layout_t target_layout);
```

### 音频分析

```c
/**
 * @brief 计算音频功率
 * @param buffer 音频缓冲区
 * @param power 功率值（输出）
 * @return 操作结果
 */
audio_result_t audio_calculate_power(const audio_buffer_t* buffer,
                                    float* power);

/**
 * @brief 计算音频峰值
 * @param buffer 音频缓冲区
 * @param peak 峰值（输出）
 * @return 操作结果
 */
audio_result_t audio_calculate_peak(const audio_buffer_t* buffer,
                                   float* peak);

/**
 * @brief 计算音频RMS
 * @param buffer 音频缓冲区
 * @param rms RMS值（输出）
 * @return 操作结果
 */
audio_result_t audio_calculate_rms(const audio_buffer_t* buffer,
                                  float* rms);

/**
 * @brief 执行FFT分析
 * @param buffer 音频缓冲区
 * @param fft_result FFT结果（输出）
 * @param fft_size FFT大小
 * @return 操作结果
 */
audio_result_t audio_fft_analyze(const audio_buffer_t* buffer,
                                audio_fft_result_t* fft_result,
                                uint32_t fft_size);
```

### 错误处理

```c
/**
 * @brief 获取错误描述
 * @param error_code 错误代码
 * @return 错误描述字符串
 */
const char* audio_result_to_string(audio_result_t error_code);

/**
 * @brief 获取最后一个错误
 * @return 最后一个错误代码
 */
audio_result_t audio_get_last_error(void);

/**
 * @brief 设置错误回调
 * @param callback 错误回调函数
 * @param user_data 用户数据
 * @return 操作结果
 */
audio_result_t audio_set_error_callback(audio_error_callback_t callback,
                                       void* user_data);
```

**使用示例：**
```c
// 格式转换示例
audio_buffer_t input_buffer = {/* ... */};
audio_buffer_t output_buffer = {0};

audio_format_info_t target_format = {
    .format = AUDIO_FORMAT_S16_LE,
    .sample_rate = 48000,
    .channel_layout = AUDIO_CHANNEL_LAYOUT_STEREO
};

audio_result_t result = audio_convert_format(&input_buffer, &output_buffer, &target_format);
if (result != AUDIO_RESULT_SUCCESS) {
    fprintf(stderr, "Format conversion failed: %s\n", 
            audio_result_to_string(result));
}

// 音频分析示例
float power, peak, rms;
audio_calculate_power(&input_buffer, &power);
audio_calculate_peak(&input_buffer, &peak);
audio_calculate_rms(&input_buffer, &rms);

printf("Audio Analysis:\n");
printf("  Power: %.2f dB\n", 20.0f * log10f(power));
printf("  Peak: %.2f dB\n", 20.0f * log10f(peak));
printf("  RMS: %.2f dB\n", 20.0f * log10f(rms));
```

---

## 测试框架API

提供音频系统测试和验证的框架。

### 测试套件

```c
/**
 * @brief 创建测试套件
 * @param name 测试套件名称
 * @param suite 测试套件（输出）
 * @return 操作结果
 */
audio_result_t audio_test_suite_create(const char* name,
                                      audio_test_suite_t** suite);

/**
 * @brief 销毁测试套件
 * @param suite 测试套件
 * @return 操作结果
 */
audio_result_t audio_test_suite_destroy(audio_test_suite_t* suite);

/**
 * @brief 添加测试用例
 * @param suite 测试套件
 * @param test_case 测试用例
 * @return 操作结果
 */
audio_result_t audio_test_suite_add_case(audio_test_suite_t* suite,
                                        const audio_test_case_t* test_case);

/**
 * @brief 运行测试套件
 * @param suite 测试套件
 * @param result 测试结果（输出）
 * @return 操作结果
 */
audio_result_t audio_test_suite_run(audio_test_suite_t* suite,
                                   audio_test_result_t* result);
```

### 性能测试

```c
/**
 * @brief 测试音频延迟
 * @param manager 音频管理器
 * @param config 测试配置
 * @param latency_ms 延迟结果（输出）
 * @return 操作结果
 */
audio_result_t audio_test_latency(audio_manager_t* manager,
                                 const audio_latency_test_config_t* config,
                                 uint32_t* latency_ms);

/**
 * @brief 测试音频吞吐量
 * @param manager 音频管理器
 * @param config 测试配置
 * @param throughput 吞吐量结果（输出）
 * @return 操作结果
 */
audio_result_t audio_test_throughput(audio_manager_t* manager,
                                    const audio_throughput_test_config_t* config,
                                    audio_throughput_result_t* throughput);

/**
 * @brief 测试音频质量
 * @param input 输入信号
 * @param output 输出信号
 * @param quality_metrics 质量指标（输出）
 * @return 操作结果
 */
audio_result_t audio_test_quality(const audio_buffer_t* input,
                                 const audio_buffer_t* output,
                                 audio_quality_metrics_t* quality_metrics);
```

**使用示例：**
```c
// 创建测试套件
audio_test_suite_t* suite;
audio_test_suite_create("Audio System Tests", &suite);

// 添加延迟测试
audio_test_case_t latency_test = {
    .name = "Latency Test",
    .description = "Test audio system latency",
    .test_function = test_audio_latency,
    .setup_function = setup_latency_test,
    .teardown_function = teardown_latency_test,
    .timeout_ms = 5000
};
audio_test_suite_add_case(suite, &latency_test);

// 运行测试
audio_test_result_t result;
audio_test_suite_run(suite, &result);

printf("Test Results:\n");
printf("  Total: %u\n", result.total_tests);
printf("  Passed: %u\n", result.passed_tests);
printf("  Failed: %u\n", result.failed_tests);
printf("  Duration: %u ms\n", result.total_duration_ms);

// 销毁测试套件
audio_test_suite_destroy(suite);
```
                                              audio_device_direction_t direction,
                                              audio_device_info_t* devices,
                                              uint32_t* device_count);

/**
 * @brief 获取默认设备
 * @param manager 音频管理器
 * @param direction 设备方向
 * @param device_id 设备ID（输出）
 * @return 操作结果
 */
audio_result_t audio_manager_get_default_device(audio_manager_t* manager,
                                               audio_device_direction_t direction,
                                               uint32_t* device_id);
```

### 流管理

```c
/**
 * @brief 创建音频流
 * @param manager 音频管理器
 * @param config 流配置
 * @param stream 音频流（输出）
 * @return 操作结果
 */
audio_result_t audio_manager_create_stream(audio_manager_t* manager,
                                          const audio_stream_config_t* config,
                                          audio_stream_t** stream);

/**
 * @brief 销毁音频流
 * @param manager 音频管理器
 * @param stream 音频流
 * @return 操作结果
 */
audio_result_t audio_manager_destroy_stream(audio_manager_t* manager,
                                           audio_stream_t* stream);
```

### 使用示例

```c
#include "core/audio_manager.h"

int main() {
    // 创建管理器配置
    audio_manager_config_t config = {0};
    config.max_streams = 16;
    config.buffer_size_frames = 1024;
    config.sample_rate = 44100;
    config.enable_real_time = true;
    
    // 创建音频管理器
    audio_manager_t* manager = audio_manager_create(&config);
    if (!manager) {
        printf("Failed to create audio manager\n");
        return -1;
    }
    
    // 初始化并启动
    if (audio_manager_initialize(manager) != AUDIO_RESULT_SUCCESS) {
        printf("Failed to initialize audio manager\n");
        audio_manager_destroy(manager);
        return -1;
    }
    
    if (audio_manager_start(manager) != AUDIO_RESULT_SUCCESS) {
        printf("Failed to start audio manager\n");
        audio_manager_deinitialize(manager);
        audio_manager_destroy(manager);
        return -1;
    }
    
    // 枚举输出设备
    audio_device_info_t devices[16];
    uint32_t device_count = 16;
    audio_manager_enumerate_devices(manager, AUDIO_DEVICE_DIRECTION_OUTPUT,
                                   devices, &device_count);
    
    printf("Found %u output devices:\n", device_count);
    for (uint32_t i = 0; i < device_count; i++) {
        printf("  %u: %s\n", devices[i].device_id, devices[i].name);
    }
    
    // 清理
    audio_manager_stop(manager);
    audio_manager_deinitialize(manager);
    audio_manager_destroy(manager);
    
    return 0;
}
```

---

## 音频引擎API

音频引擎负责核心的音频处理逻辑。

### 创建和销毁

```c
/**
 * @brief 创建音频引擎
 * @param config 引擎配置
 * @return 音频引擎实例，失败返回NULL
 */
audio_engine_t* audio_engine_create(const audio_engine_config_t* config);

/**
 * @brief 销毁音频引擎
 * @param engine 音频引擎
 */
void audio_engine_destroy(audio_engine_t* engine);
```

### 流处理

```c
/**
 * @brief 创建音频流
 * @param engine 音频引擎
 * @param config 流配置
 * @param stream 音频流（输出）
 * @return 操作结果
 */
audio_result_t audio_engine_create_stream(audio_engine_t* engine,
                                         const audio_stream_config_t* config,
                                         audio_stream_t** stream);

/**
 * @brief 处理音频数据
 * @param engine 音频引擎
 * @param input_buffer 输入缓冲区
 * @param output_buffer 输出缓冲区
 * @return 操作结果
 */
audio_result_t audio_engine_process(audio_engine_t* engine,
                                   const audio_buffer_t* input_buffer,
                                   audio_buffer_t* output_buffer);
```

---

## 流管理器API

流管理器负责音频流的生命周期管理和调度。

### 流操作

```c
/**
 * @brief 启动音频流
 * @param manager 流管理器
 * @param stream_id 流ID
 * @return 操作结果
 */
audio_result_t stream_manager_start_stream(stream_manager_t* manager,
                                          uint32_t stream_id);

/**
 * @brief 停止音频流
 * @param manager 流管理器
 * @param stream_id 流ID
 * @return 操作结果
 */
audio_result_t stream_manager_stop_stream(stream_manager_t* manager,
                                         uint32_t stream_id);

/**
 * @brief 暂停音频流
 * @param manager 流管理器
 * @param stream_id 流ID
 * @return 操作结果
 */
audio_result_t stream_manager_pause_stream(stream_manager_t* manager,
                                          uint32_t stream_id);

/**
 * @brief 恢复音频流
 * @param manager 流管理器
 * @param stream_id 流ID
 * @return 操作结果
 */
audio_result_t stream_manager_resume_stream(stream_manager_t* manager,
                                           uint32_t stream_id);
```

---

## 事件总线API

事件总线提供系统内的事件通信机制。

### 事件发布和订阅

```c
/**
 * @brief 发布事件（同步）
 * @param bus 事件总线
 * @param event 事件
 * @return 操作结果
 */
audio_result_t event_bus_publish_sync(event_bus_t* bus,
                                     const audio_event_t* event);

/**
 * @brief 发布事件（异步）
 * @param bus 事件总线
 * @param event 事件
 * @return 操作结果
 */
audio_result_t event_bus_publish_async(event_bus_t* bus,
                                      const audio_event_t* event);

/**
 * @brief 订阅事件
 * @param bus 事件总线
 * @param subscriber 订阅者
 * @return 操作结果
 */
audio_result_t event_bus_subscribe(event_bus_t* bus,
                                  const event_subscriber_t* subscriber);

/**
 * @brief 订阅事件
 * @param bus 事件总线
 * @param subscriber 订阅者信息
 * @return 操作结果
 */
audio_result_t event_bus_subscribe(event_bus_t* bus,
                                   const event_subscriber_t* subscriber);

/**
 * @brief 取消订阅
 * @param bus 事件总线
 * @param subscriber_id 订阅者ID
 * @return 操作结果
 */
audio_result_t event_bus_unsubscribe(event_bus_t* bus,
                                    uint32_t subscriber_id);
```

### 使用示例

```c
#include "core/event_bus.h"

// 事件回调函数
void on_stream_event(const audio_event_t* event, void* user_data) {
    printf("Received event: %s from source %u\n",
           audio_event_type_to_string(event->type),
           event->source_id);
}

int main() {
    // 创建事件总线
    event_bus_config_t config = {0};
    config.max_subscribers = 32;
    config.queue_size = 1024;
    config.enable_async = true;
    
    event_bus_t* bus = event_bus_create(&config);
    
    // 创建订阅者
    event_subscriber_t subscriber = {0};
    subscriber.subscriber_id = 1;
    subscriber.callback = on_stream_event;
    subscriber.user_data = NULL;
    
    // 添加事件过滤器
    event_filter_t filter = {0};
    filter.event_types = AUDIO_EVENT_STREAM_STARTED | AUDIO_EVENT_STREAM_STOPPED;
    subscriber.filter = filter;
    
    // 订阅事件
    event_bus_subscribe(bus, &subscriber);
    
    // 启动事件总线
    event_bus_start(bus);
    
    // 发布事件
    audio_event_t* event = audio_event_create(AUDIO_EVENT_STREAM_STARTED,
                                             123, NULL, 0);
    event_bus_publish_async(bus, event);
    
    // 清理
    audio_event_destroy(event);
    event_bus_stop(bus);
    event_bus_destroy(bus);
    
    return 0;
}
```

---

## 插件管理器API

插件管理器负责音频插件的加载、管理和实例化。

### 插件发现和注册

```c
/**
 * @brief 发现插件
 * @param manager 插件管理器
 * @param search_paths 搜索路径数组
 * @param path_count 路径数量
 * @return 操作结果
 */
audio_result_t plugin_manager_discover_plugins(plugin_manager_t* manager,
                                              const char** search_paths,
                                              uint32_t path_count);

/**
 * @brief 注册插件
 * @param manager 插件管理器
 * @param descriptor 插件描述符
 * @return 操作结果
 */
audio_result_t plugin_manager_register_plugin(plugin_manager_t* manager,
                                             const plugin_descriptor_t* descriptor);
```

### 插件实例管理

```c
/**
 * @brief 创建插件实例
 * @param manager 插件管理器
 * @param plugin_id 插件ID
 * @param config 插件配置
 * @param instance 插件实例（输出）
 * @return 操作结果
 */
audio_result_t plugin_manager_create_instance(plugin_manager_t* manager,
                                             uint32_t plugin_id,
                                             const plugin_config_t* config,
                                             plugin_instance_t** instance);

/**
 * @brief 销毁插件实例
 * @param manager 插件管理器
 * @param instance 插件实例
 * @return 操作结果
 */
audio_result_t plugin_manager_destroy_instance(plugin_manager_t* manager,
                                              plugin_instance_t* instance);
```

---

## 音频管道API

音频管道提供灵活的音频处理链。

### 管道创建和管理

```c
/**
 * @brief 创建音频管道
 * @param config 管道配置
 * @return 音频管道实例，失败返回NULL
 */
audio_pipeline_t* audio_pipeline_create(const pipeline_config_t* config);

/**
 * @brief 销毁音频管道
 * @param pipeline 音频管道
 */
void audio_pipeline_destroy(audio_pipeline_t* pipeline);
```

### 节点管理

```c
/**
 * @brief 添加节点
 * @param pipeline 音频管道
 * @param node 管道节点
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_node(audio_pipeline_t* pipeline,
                                       pipeline_node_t* node);

/**
 * @brief 连接节点
 * @param pipeline 音频管道
 * @param source_node_id 源节点ID
 * @param source_port 源端口
 * @param dest_node_id 目标节点ID
 * @param dest_port 目标端口
 * @return 操作结果
 */
audio_result_t audio_pipeline_connect_nodes(audio_pipeline_t* pipeline,
                                           uint32_t source_node_id,
                                           uint32_t source_port,
                                           uint32_t dest_node_id,
                                           uint32_t dest_port);
```

### 使用示例

```c
#include "pipeline/audio_pipeline.h"

int main() {
    // 创建管道
    pipeline_config_t config = {0};
    config.max_nodes = 16;
    config.max_connections = 32;
    
    audio_pipeline_t* pipeline = audio_pipeline_create(&config);
    
    // 创建输入节点
    pipeline_node_t* input_node = audio_pipeline_create_input_node(
        pipeline, "audio_input", 1);
    
    // 创建增益插件节点
    plugin_config_t gain_config = {0};
    gain_config.parameters[0].value.f = 0.8f; // 增益值
    
    pipeline_node_t* gain_node = audio_pipeline_create_plugin_node(
        pipeline, "gain_plugin", "gain", &gain_config);
    
    // 创建输出节点
    pipeline_node_t* output_node = audio_pipeline_create_output_node(
        pipeline, "audio_output", 2);
    
    // 连接节点
    audio_pipeline_connect_nodes(pipeline,
                                input_node->node_id, 0,
                                gain_node->node_id, 0);
    
    audio_pipeline_connect_nodes(pipeline,
                                gain_node->node_id, 0,
                                output_node->node_id, 0);
    
    // 准备和启动管道
    audio_pipeline_prepare(pipeline);
    audio_pipeline_start(pipeline);
    
    // 处理音频数据...
    
    // 清理
    audio_pipeline_stop(pipeline);
    audio_pipeline_destroy(pipeline);
    
    return 0;
}
```

---

## 工具函数API

工具函数提供常用的音频处理和转换功能。

### 格式验证和转换

```c
/**
 * @brief 验证音频格式
 * @param format 音频格式信息
 * @return true表示有效，false表示无效
 */
bool audio_format_is_valid(const audio_format_info_t* format);

/**
 * @brief 比较两个音频格式
 * @param format1 格式1
 * @param format2 格式2
 * @return true表示相同，false表示不同
 */
bool audio_format_is_equal(const audio_format_info_t* format1,
                          const audio_format_info_t* format2);

/**
 * @brief 音频格式转字符串
 * @param format 音频格式
 * @return 格式字符串
 */
const char* audio_format_to_string(audio_format_t format);
```

### 时间转换

```c
/**
 * @brief 帧数转微秒
 * @param frames 帧数
 * @param sample_rate 采样率
 * @return 时间（微秒）
 */
uint64_t audio_frames_to_microseconds(uint32_t frames, uint32_t sample_rate);

/**
 * @brief 微秒转帧数
 * @param microseconds 时间（微秒）
 * @param sample_rate 采样率
 * @return 帧数
 */
uint32_t audio_microseconds_to_frames(uint64_t microseconds, uint32_t sample_rate);
```

### 音频数据处理

```c
/**
 * @brief 计算音频RMS值
 * @param buffer 音频缓冲区
 * @return RMS值（0.0-1.0）
 */
float audio_calculate_rms(const audio_buffer_t* buffer);

/**
 * @brief 应用音量增益
 * @param buffer 音频缓冲区
 * @param gain 增益值（1.0为原始音量）
 * @return 操作结果
 */
audio_result_t audio_apply_gain(audio_buffer_t* buffer, float gain);

/**
 * @brief 混合两个音频缓冲区
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param mix_ratio 混合比例（0.0-1.0）
 * @return 操作结果
 */
audio_result_t audio_mix_buffers(audio_buffer_t* dst,
                                const audio_buffer_t* src,
                                float mix_ratio);
```

### 缓冲区管理

```c
/**
 * @brief 创建音频缓冲区
 * @param format 音频格式信息
 * @param frame_count 帧数
 * @return 音频缓冲区，失败返回NULL
 */
audio_buffer_t* audio_buffer_create(const audio_format_info_t* format,
                                   uint32_t frame_count);

/**
 * @brief 销毁音频缓冲区
 * @param buffer 音频缓冲区
 */
void audio_buffer_destroy(audio_buffer_t* buffer);
```

---

## 测试框架API

测试框架提供完整的单元测试和集成测试支持。

### 测试运行器

```c
/**
 * @brief 创建测试运行器
 * @param config 运行器配置
 * @return 测试运行器，失败返回NULL
 */
test_runner_t* test_runner_create(const test_runner_config_t* config);

/**
 * @brief 运行所有测试
 * @param runner 测试运行器
 * @return 操作结果
 */
audio_result_t test_runner_run_all(test_runner_t* runner);
```

### 断言宏

```c
// 基础断言
#define TEST_ASSERT(condition)
#define TEST_ASSERT_EQUAL(expected, actual)
#define TEST_ASSERT_NOT_NULL(ptr)
#define TEST_ASSERT_STRING_EQUAL(expected, actual)

// 音频专用断言
#define TEST_ASSERT_AUDIO_SUCCESS(result)
#define TEST_ASSERT_FLOAT_EQUAL(expected, actual, tolerance)
```

### 测试用例定义

```c
// 定义测试用例
#define TEST_CASE(name, desc, type, priority, func)

// 定义测试套件
#define TEST_SUITE(name, desc, cases, count)
```

---

## 错误处理

所有API函数都返回`audio_result_t`类型的错误代码。应用程序应该检查返回值并适当处理错误。

### 错误检查示例

```c
audio_result_t result = audio_manager_initialize(manager);
if (result != AUDIO_RESULT_SUCCESS) {
    printf("Error: %s\n", audio_result_to_string(result));
    // 处理错误...
    return -1;
}
```

### 常见错误处理模式

```c
// 资源清理模式
audio_manager_t* manager = NULL;
audio_stream_t* stream = NULL;

do {
    manager = audio_manager_create(&config);
    if (!manager) {
        result = AUDIO_RESULT_OUT_OF_MEMORY;
        break;
    }
    
    result = audio_manager_initialize(manager);
    if (result != AUDIO_RESULT_SUCCESS) {
        break;
    }
    
    result = audio_manager_create_stream(manager, &stream_config, &stream);
    if (result != AUDIO_RESULT_SUCCESS) {
        break;
    }
    
    // 正常处理...
    
} while (0);

// 清理资源
if (stream) {
    audio_manager_destroy_stream(manager, stream);
}
if (manager) {
    audio_manager_deinitialize(manager);
    audio_manager_destroy(manager);
}

return result;
```

---

## 线程安全

大多数API函数都是线程安全的，但有以下注意事项：

1. **管理器对象**：可以从多个线程安全访问
2. **流对象**：单个流应该只从一个线程访问
3. **缓冲区对象**：不是线程安全的，需要外部同步
4. **事件总线**：完全线程安全

### 线程安全示例

```c
// 安全：从不同线程访问管理器
void thread1_func(audio_manager_t* manager) {
    audio_manager_get_statistics(manager, &stats);
}

void thread2_func(audio_manager_t* manager) {
    audio_manager_enumerate_devices(manager, direction, devices, &count);
}

// 不安全：从多个线程访问同一个缓冲区
void unsafe_example(audio_buffer_t* buffer) {
    // 需要外部同步
    pthread_mutex_lock(&buffer_mutex);
    audio_apply_gain(buffer, 1.5f);
    pthread_mutex_unlock(&buffer_mutex);
}
```

---

## 性能优化建议

1. **使用合适的缓冲区大小**：通常64-2048帧
2. **避免频繁的内存分配**：重用缓冲区
3. **使用实时线程优先级**：对于低延迟应用
4. **选择合适的音频格式**：浮点格式处理更快
5. **批量处理**：一次处理多个样本

### 性能优化示例

```c
// 预分配缓冲区
audio_buffer_t* buffers[4];
for (int i = 0; i < 4; i++) {
    buffers[i] = audio_buffer_create(&format, 1024);
}

// 重用缓冲区而不是重新分配
for (int frame = 0; frame < total_frames; frame += 1024) {
    audio_buffer_t* current_buffer = buffers[frame % 4];
    // 处理音频数据...
    audio_clear_buffer(current_buffer); // 清零重用
}
```

---

本API参考文档涵盖了LinxOS音频系统的主要接口。更多详细信息请参考源代码中的注释和示例程序。