# LinX OS SDK - 模块接口详细设计

## 概述

本文档详细描述了LinX OS SDK中各个核心模块的接口设计，包括音频、显示、摄像头、网络、GPIO等模块的C99接口定义和实现策略。

## 1. 音频模块 (Audio)

### 1.1 核心接口 (`src/audio/audio_interface.h`)

#### 结构体定义
```c
typedef struct AudioInterface {
    const AudioInterfaceVTable* vtable;
    void* impl_data;
    
    // 配置参数
    int sample_rate;           // 采样率
    int channels;              // 声道数
    int frame_size;            // 帧大小
    int periods;               // 周期数
    int buffer_size;           // 缓冲区大小
    int period_size;           // 周期大小
    
    // 状态信息
    bool is_initialized;       // 是否已初始化
    bool is_started;          // 是否已启动
    bool duplex_;             // 是否全双工
    bool input_enabled_;      // 输入是否启用
    bool output_enabled_;     // 输出是否启用
    int output_volume_;       // 输出音量 (0-100)
    
    // 输入输出配置
    int input_sample_rate;    // 输入采样率
    int output_sample_rate;   // 输出采样率
    int input_channels;       // 输入声道数
    int output_channels;      // 输出声道数
} AudioInterface;
```

#### 虚函数表
```c
typedef struct AudioInterfaceVTable {
    // 生命周期管理
    int (*init)(AudioInterface* self);
    int (*start)(AudioInterface* self);
    int (*destroy)(AudioInterface* self);
    
    // 音量控制
    int (*set_output_volume)(AudioInterface* self, int volume);
    int (*get_output_volume)(AudioInterface* self);
    
    // 输入输出控制
    int (*enable_input)(AudioInterface* self, bool enabled);
    int (*enable_output)(AudioInterface* self, bool enabled);
    
    // 数据处理
    int (*output_data)(AudioInterface* self, const int16_t* data, size_t samples);
    int (*input_data)(AudioInterface* self, int16_t* data, size_t samples);
    int (*read)(AudioInterface* self, void* buffer, size_t size);
    int (*write)(AudioInterface* self, const void* buffer, size_t size);
    
    // 配置管理
    int (*set_config)(AudioInterface* self, int sample_rate, int channels, int frame_size);
} AudioInterfaceVTable;
```

#### 错误码定义
```c
typedef enum {
    AUDIO_ERROR_NONE = 0,
    AUDIO_ERROR_INIT_FAILED = -1,
    AUDIO_ERROR_NOT_INITIALIZED = -2,
    AUDIO_ERROR_INVALID_PARAMETER = -3,
    AUDIO_ERROR_DEVICE_BUSY = -4,
    AUDIO_ERROR_BUFFER_OVERFLOW = -5,
    AUDIO_ERROR_BUFFER_UNDERFLOW = -6,
    AUDIO_ERROR_HARDWARE_FAILURE = -7
} AudioError;
```

### 1.2 音频编解码器接口 (`src/audio/audio_codec.h`)

#### 结构体定义
```c
typedef struct audio_codec_t {
    const audio_codec_vtable_t* vtable;
    void* impl_data;
    
    // 编解码器信息
    char* name;
    bool is_encoder;
    bool is_decoder;
    
    // 音频格式
    audio_format_t input_format;
    audio_format_t output_format;
    
    // 状态
    bool is_initialized;
} audio_codec_t;

typedef struct audio_format_t {
    int sample_rate;      // 采样率
    int channels;         // 声道数
    int bits_per_sample;  // 每样本位数
    int frame_size;       // 帧大小
} audio_format_t;
```

#### 虚函数表
```c
typedef struct audio_codec_vtable_t {
    // 初始化和销毁
    codec_error_t (*init_encoder)(audio_codec_t* self, const audio_format_t* format);
    codec_error_t (*init_decoder)(audio_codec_t* self, const audio_format_t* format);
    void (*destroy)(audio_codec_t* self);
    
    // 编解码操作
    codec_error_t (*encode)(audio_codec_t* self, const void* input, size_t input_size,
                           void* output, size_t* output_size);
    codec_error_t (*decode)(audio_codec_t* self, const void* input, size_t input_size,
                           void* output, size_t* output_size);
    
    // 信息获取
    const char* (*get_name)(audio_codec_t* self);
    codec_error_t (*reset)(audio_codec_t* self);
} audio_codec_vtable_t;
```

## 2. 显示模块 (Display)

### 2.1 核心接口 (`src/display/display.h`)

#### 结构体定义
```c
typedef struct DisplayInterface {
    const DisplayInterfaceVTable* vtable;
    void* impl_data;
    
    // 显示属性
    int width;
    int height;
    int color_depth;
    
    // 状态信息
    bool is_initialized;
    bool is_power_save;
    bool is_locked;
    
    // 当前显示内容
    char* current_status;
    char* current_notification;
    char* current_emotion;
    char* current_chat_message;
    char* current_theme;
} DisplayInterface;
```

#### 虚函数表
```c
typedef struct DisplayInterfaceVTable {
    // 生命周期管理
    int (*init)(DisplayInterface* self);
    void (*destroy)(DisplayInterface* self);
    
    // 内容显示
    void (*set_status)(DisplayInterface* self, const char* status);
    void (*show_notification)(DisplayInterface* self, const char* message, int duration_ms);
    void (*set_emotion)(DisplayInterface* self, const char* emotion);
    void (*set_chat_message)(DisplayInterface* self, const char* message);
    
    // 主题和样式
    void (*set_theme)(DisplayInterface* self, const char* theme);
    void (*update_status_bar)(DisplayInterface* self, const char* info);
    
    // 电源管理
    void (*set_power_save_mode)(DisplayInterface* self, bool enabled);
    
    // 显示控制
    void (*lock_display)(DisplayInterface* self);
    void (*unlock_display)(DisplayInterface* self);
    void (*clear_display)(DisplayInterface* self);
    void (*refresh_display)(DisplayInterface* self);
} DisplayInterfaceVTable;
```

## 3. 摄像头模块 (Camera)

### 3.1 核心接口 (`src/camera/camera_interface.h`)

#### 结构体定义
```c
typedef struct CameraInterface {
    const CameraInterfaceVTable* vtable;
    void* impl_data;
    
    // 摄像头配置
    CameraConfig config;
    
    // 状态信息
    bool is_initialized;
    bool is_streaming;
    bool horizontal_mirror;
    bool vertical_flip;
    
    // 当前帧信息
    CameraFrameBuffer* current_frame;
    char* explain_url;
} CameraInterface;

typedef struct CameraConfig {
    int width;
    int height;
    int fps;
    int quality;
    int brightness;
    int contrast;
    int saturation;
} CameraConfig;

typedef struct CameraFrameBuffer {
    uint8_t* buffer;
    size_t size;
    int width;
    int height;
    int format;
    uint64_t timestamp;
} CameraFrameBuffer;
```

#### 虚函数表
```c
typedef struct CameraInterfaceVTable {
    // 生命周期管理
    int (*init)(CameraInterface* self);
    void (*destroy)(CameraInterface* self);
    
    // 配置管理
    int (*set_config)(CameraInterface* self, const CameraConfig* config);
    int (*get_config)(CameraInterface* self, CameraConfig* config);
    
    // 帧捕获
    int (*capture_frame)(CameraInterface* self, CameraFrameBuffer** frame);
    void (*release_frame)(CameraInterface* self, CameraFrameBuffer* frame);
    
    // 图像处理
    int (*set_horizontal_mirror)(CameraInterface* self, bool enabled);
    int (*set_vertical_flip)(CameraInterface* self, bool enabled);
    
    // AI功能
    int (*set_explain_url)(CameraInterface* self, const char* url);
    int (*explain_question)(CameraInterface* self, const char* question);
} CameraInterfaceVTable;
```

## 4. 网络模块 (Network)

### 4.1 核心接口 (基于现有设计)

#### 结构体定义
```c
typedef struct NetworkInterface {
    const NetworkInterfaceVTable* vtable;
    void* impl_data;
    
    // 网络配置
    NetworkConfig config;
    NetworkStatus status;
    
    // 状态信息
    bool is_initialized;
    NetworkState state;
    NetworkType type;
    
    // WiFi特定
    WiFiScanResult* scan_results;
    int scan_result_count;
    
    // 事件回调
    NetworkEventCallback event_callback;
    WiFiScanCallback scan_callback;
    void* callback_context;
} NetworkInterface;
```

#### 网络状态和类型
```c
typedef enum {
    NETWORK_STATE_DISCONNECTED = 0,
    NETWORK_STATE_CONNECTING,
    NETWORK_STATE_CONNECTED,
    NETWORK_STATE_DISCONNECTING,
    NETWORK_STATE_ERROR
} NetworkState;

typedef enum {
    NETWORK_TYPE_WIFI = 0,
    NETWORK_TYPE_ETHERNET,
    NETWORK_TYPE_CELLULAR
} NetworkType;

typedef enum {
    WIFI_SECURITY_NONE = 0,
    WIFI_SECURITY_WEP,
    WIFI_SECURITY_WPA,
    WIFI_SECURITY_WPA2,
    WIFI_SECURITY_WPA3
} WiFiSecurityType;
```

## 5. GPIO模块

### 5.1 核心接口 (基于现有设计)

#### 结构体定义
```c
typedef struct GPIOInterface {
    const GPIOInterfaceVTable* vtable;
    void* impl_data;
    
    // 状态信息
    bool is_initialized;
    
    // 中断回调
    GPIOInterruptCallback interrupt_callbacks[GPIO_MAX_PINS];
    void* interrupt_contexts[GPIO_MAX_PINS];
} GPIOInterface;

typedef struct GPIOConfig {
    int pin;
    GPIOMode mode;
    bool pull_up;
    bool pull_down;
    int drive_strength;
} GPIOConfig;
```

#### GPIO模式和中断类型
```c
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_OUTPUT,
    GPIO_MODE_ANALOG,
    GPIO_MODE_PWM
} GPIOMode;

typedef enum {
    GPIO_INTERRUPT_NONE = 0,
    GPIO_INTERRUPT_RISING,
    GPIO_INTERRUPT_FALLING,
    GPIO_INTERRUPT_BOTH,
    GPIO_INTERRUPT_LOW_LEVEL,
    GPIO_INTERRUPT_HIGH_LEVEL
} GPIOInterruptType;
```

## 6. 通用模块 (Common)

### 6.1 设置管理 (`src/common/settings.h`)

#### 结构体定义
```c
typedef struct Settings {
    const SettingsVTable* vtable;
    void* impl_data;
    
    // 状态信息
    bool is_initialized;
    char* storage_path;
} Settings;
```

#### 虚函数表
```c
typedef struct SettingsVTable {
    // 生命周期管理
    int (*init)(Settings* self, const char* storage_path);
    void (*destroy)(Settings* self);
    
    // 字符串操作
    int (*set_string)(Settings* self, const char* key, const char* value);
    const char* (*get_string)(Settings* self, const char* key, const char* default_value);
    
    // 整数操作
    int (*set_int)(Settings* self, const char* key, int value);
    int (*get_int)(Settings* self, const char* key, int default_value);
    
    // 布尔操作
    int (*set_bool)(Settings* self, const char* key, bool value);
    bool (*get_bool)(Settings* self, const char* key, bool default_value);
    
    // 浮点数操作
    int (*set_float)(Settings* self, const char* key, float value);
    float (*get_float)(Settings* self, const char* key, float default_value);
    
    // 键管理
    bool (*has_key)(Settings* self, const char* key);
    int (*remove_key)(Settings* self, const char* key);
    
    // 存储管理
    int (*save)(Settings* self);
    int (*load)(Settings* self);
    int (*clear)(Settings* self);
} SettingsVTable;
```

### 6.2 HTTP客户端 (`src/common/http/http_client.h`)

#### 结构体定义
```c
typedef struct HttpClient {
    const HttpClientVTable* vtable;
    void* impl_data;
    
    // 配置
    int timeout_ms;
    char* user_agent;
    char* base_url;
    
    // 状态
    bool is_initialized;
    int last_response_code;
} HttpClient;

typedef struct HttpRequest {
    char* method;
    char* url;
    char* headers;
    char* body;
    size_t body_size;
} HttpRequest;

typedef struct HttpResponse {
    int status_code;
    char* headers;
    char* body;
    size_t body_size;
} HttpResponse;
```

#### 虚函数表
```c
typedef struct HttpClientVTable {
    // 生命周期管理
    int (*init)(HttpClient* self);
    void (*destroy)(HttpClient* self);
    
    // 配置
    void (*set_timeout)(HttpClient* self, int timeout_ms);
    void (*set_user_agent)(HttpClient* self, const char* user_agent);
    void (*set_base_url)(HttpClient* self, const char* base_url);
    
    // HTTP请求
    int (*get)(HttpClient* self, const char* url, HttpResponse** response);
    int (*post)(HttpClient* self, const char* url, const char* data, HttpResponse** response);
    int (*put)(HttpClient* self, const char* url, const char* data, HttpResponse** response);
    int (*delete)(HttpClient* self, const char* url, HttpResponse** response);
    
    // 通用请求
    int (*request)(HttpClient* self, const HttpRequest* request, HttpResponse** response);
    
    // 响应管理
    void (*free_response)(HttpClient* self, HttpResponse* response);
} HttpClientVTable;
```

### 6.3 日志系统 (`src/common/log/log.h`)

#### 日志级别
```c
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;
```

#### 日志接口
```c
typedef struct Logger {
    const LoggerVTable* vtable;
    void* impl_data;
    
    // 配置
    LogLevel level;
    char* tag;
    bool enabled;
} Logger;

typedef struct LoggerVTable {
    // 生命周期管理
    int (*init)(Logger* self, const char* tag);
    void (*destroy)(Logger* self);
    
    // 配置
    void (*set_level)(Logger* self, LogLevel level);
    void (*set_enabled)(Logger* self, bool enabled);
    
    // 日志输出
    void (*log)(Logger* self, LogLevel level, const char* format, ...);
    void (*trace)(Logger* self, const char* format, ...);
    void (*debug)(Logger* self, const char* format, ...);
    void (*info)(Logger* self, const char* format, ...);
    void (*warn)(Logger* self, const char* format, ...);
    void (*error)(Logger* self, const char* format, ...);
    void (*fatal)(Logger* self, const char* format, ...);
} LoggerVTable;
```

## 7. LinX SDK核心 (`src/linxsdk/linx_sdk.h`)

### 7.1 主要结构体

#### LinX SDK主结构
```c
typedef struct LinxSdk {
    // 版本信息
    int major_version;
    int minor_version;
    int patch_version;
    
    // 配置
    LinxSdkConfig config;
    
    // 状态
    LinxDeviceState state;
    LinxSdkError last_error;
    char* error_message;
    
    // 事件处理
    LinxEventCallback event_callback;
    void* event_callback_context;
    
    // 连接状态
    bool is_connected;
    bool is_listening;
    char* session_id;
    
    // 内部组件
    void* websocket_protocol;
    void* event_thread;
    void* mcp_server;
    
    // 实现数据
    void* impl_data;
} LinxSdk;
```

#### 配置结构
```c
typedef struct LinxSdkConfig {
    char* server_url;
    int sample_rate;
    int channels;
    int connect_timeout_ms;
    int read_timeout_ms;
    char* auth_token;
    char* device_id;
    char* client_id;
    bool auto_reconnect;
    int max_reconnect_attempts;
    bool listening_mode;
} LinxSdkConfig;
```

#### 事件系统
```c
typedef enum {
    LINX_EVENT_STATE_CHANGED = 0,
    LINX_EVENT_MESSAGE_RECEIVED,
    LINX_EVENT_AUDIO_DATA,
    LINX_EVENT_ERROR,
    LINX_EVENT_WEBSOCKET_CONNECTED,
    LINX_EVENT_WEBSOCKET_DISCONNECTED,
    LINX_EVENT_WEBSOCKET_ERROR,
    LINX_EVENT_SESSION_STARTED,
    LINX_EVENT_SESSION_ENDED,
    LINX_EVENT_TTS_STARTED,
    LINX_EVENT_TTS_FINISHED,
    LINX_EVENT_MCP_MESSAGE
} LinxEventType;

typedef struct LinxEvent {
    LinxEventType type;
    void* data;
    size_t data_size;
    uint64_t timestamp;
} LinxEvent;

typedef void (*LinxEventCallback)(const LinxEvent* event, void* context);
```

### 7.2 API函数

#### 生命周期管理
```c
LinxSdk* linx_sdk_create(const LinxSdkConfig* config);
void linx_sdk_destroy(LinxSdk* sdk);
```

#### 事件处理
```c
void linx_sdk_set_event_callback(LinxSdk* sdk, LinxEventCallback callback, void* context);
```

#### 连接管理
```c
LinxSdkError linx_sdk_connect(LinxSdk* sdk);
LinxSdkError linx_sdk_disconnect(LinxSdk* sdk);
bool linx_sdk_is_connected(LinxSdk* sdk);
```

#### 消息发送
```c
LinxSdkError linx_sdk_send_text(LinxSdk* sdk, const char* text);
LinxSdkError linx_sdk_send_audio(LinxSdk* sdk, const void* audio_data, size_t size);
```

#### 状态查询
```c
LinxDeviceState linx_sdk_get_state(LinxSdk* sdk);
LinxSdkError linx_sdk_get_last_error(LinxSdk* sdk);
const char* linx_sdk_get_error_message(LinxSdk* sdk);
```

#### 高级功能
```c
LinxSdkError linx_sdk_abort_speaking(LinxSdk* sdk);
LinxSdkError linx_sdk_send_wake_word(LinxSdk* sdk, const char* wake_word);
LinxSdkError linx_sdk_add_mcp_tool(LinxSdk* sdk, const char* tool_name, const char* tool_config);
```

## 8. 接口设计原则

### 8.1 一致性原则
- 所有接口都使用相同的命名约定
- 统一的错误处理机制
- 一致的内存管理策略

### 8.2 可扩展性原则
- 使用虚函数表支持多态
- 预留扩展字段
- 版本兼容性考虑

### 8.3 性能原则
- 最小化内存分配
- 避免不必要的数据拷贝
- 支持零拷贝操作

### 8.4 安全性原则
- 参数验证
- 缓冲区溢出保护
- 资源泄漏防护

## 9. 错误处理策略

### 9.1 统一错误码
```c
typedef enum {
    LINX_ERROR_NONE = 0,
    LINX_ERROR_INVALID_PARAMETER = -1,
    LINX_ERROR_NOT_INITIALIZED = -2,
    LINX_ERROR_ALREADY_INITIALIZED = -3,
    LINX_ERROR_OUT_OF_MEMORY = -4,
    LINX_ERROR_DEVICE_NOT_FOUND = -5,
    LINX_ERROR_DEVICE_BUSY = -6,
    LINX_ERROR_TIMEOUT = -7,
    LINX_ERROR_NETWORK_ERROR = -8,
    LINX_ERROR_PROTOCOL_ERROR = -9,
    LINX_ERROR_AUTHENTICATION_FAILED = -10,
    LINX_ERROR_PERMISSION_DENIED = -11,
    LINX_ERROR_NOT_SUPPORTED = -12,
    LINX_ERROR_INTERNAL_ERROR = -13
} LinxError;
```

### 9.2 错误信息获取
```c
const char* linx_error_to_string(LinxError error);
void linx_set_last_error(LinxError error, const char* message);
LinxError linx_get_last_error(char** message);
```

## 10. 内存管理策略

### 10.1 内存分配器接口
```c
typedef struct LinxAllocator {
    void* (*malloc)(size_t size);
    void* (*calloc)(size_t count, size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
} LinxAllocator;

void linx_set_allocator(const LinxAllocator* allocator);
const LinxAllocator* linx_get_allocator(void);
```

### 10.2 资源管理
- RAII模式的C实现
- 自动资源清理
- 引用计数管理

## 总结

这个模块接口设计提供了：

1. **统一的C99接口**: 保证跨平台兼容性和语言互操作性
2. **模块化架构**: 每个模块都有清晰的职责边界
3. **可扩展设计**: 使用虚函数表支持多态和扩展
4. **完整的功能覆盖**: 涵盖音频、显示、摄像头、网络、GPIO等所有硬件接口
5. **统一的错误处理**: 一致的错误码和错误信息管理
6. **高性能设计**: 最小化开销，支持实时应用
7. **安全性保证**: 参数验证和资源保护机制

通过这些接口设计，LinX OS SDK可以为上层应用提供稳定、高效、易用的硬件抽象层。