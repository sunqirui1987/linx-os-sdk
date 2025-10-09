# LinX OS SDK - src目录架构设计方案

## 概述

本文档详细描述了LinX OS SDK中`src`目录的完整架构设计，包括各个模块的设计理念、接口规范、依赖关系以及跨平台兼容性策略。

## 设计原则

### 1. 分层架构
- **应用层**: 使用SDK的应用程序
- **SDK接口层**: 统一的C99接口，提供跨平台兼容性
- **模块实现层**: 各功能模块的具体实现
- **平台抽象层**: 平台特定的底层实现
- **硬件层**: 具体的硬件平台

### 2. 模块化设计
- 每个模块独立开发和测试
- 清晰的模块边界和接口定义
- 最小化模块间的耦合度
- 支持模块的独立升级和替换

### 3. C99兼容性
- 所有公共接口使用C99标准
- 支持C++项目的无缝集成
- 使用虚函数表实现多态性
- 避免使用C++特性以保持兼容性

### 4. 跨平台支持
- 统一的接口定义
- 平台特定的实现隔离
- 配置驱动的功能选择
- 条件编译支持

## 目录结构设计

```
src/
├── CMakeLists.txt              # 主构建配置
├── audio/                      # 音频处理模块
│   ├── audio/                  # 音频接口核心
│   ├── codecs/                 # 音频编解码器
│   ├── processor/              # 音频处理器
│   └── wake_words/             # 唤醒词检测
├── board/                      # 板级抽象层
├── camera/                     # 摄像头接口
├── common/                     # 通用工具和基础设施
│   ├── cjson/                  # JSON处理
│   ├── http/                   # HTTP客户端
│   ├── log/                    # 日志系统
│   └── std/                    # 标准库扩展
├── display/                    # 显示接口
│   ├── lvgl_display/           # LVGL显示实现
│   └── theme/                  # 主题系统
├── linxsdk/                    # 核心SDK
│   ├── mcp/                    # MCP协议支持
│   ├── ota/                    # OTA更新
│   └── protocols/              # 通信协议
└── third/                      # 第三方库
    ├── fonts/                  # 字体库
    ├── liblvgl/                # LVGL图形库
    ├── mongoose/               # HTTP/WebSocket库
    └── opus/                   # Opus音频编解码
```

## 核心模块设计

### 1. Board模块 (`src/board/`)

#### 设计目标
- 提供统一的板级抽象接口
- 支持多种硬件平台
- 简化硬件相关的配置和初始化

#### 接口设计
```c
typedef struct Board Board;

typedef struct {
    // 生命周期管理
    int (*init)(Board* self);
    void (*destroy)(Board* self);
    
    // 基本信息
    const char* (*get_board_type)(Board* self);
    const char* (*get_uuid)(Board* self);
    
    // 硬件接口获取
    void* (*get_audio_codec)(Board* self);
    void* (*get_display)(Board* self);
    void* (*get_network)(Board* self);
    void* (*get_camera)(Board* self);
    void* (*get_led)(Board* self);
    
    // 系统功能
    float (*get_temperature)(Board* self);
    int (*get_battery_level)(Board* self);
    void (*set_power_save_mode)(Board* self, bool enabled);
    
    // 配置管理
    const char* (*get_board_json)(Board* self);
    const char* (*get_device_status_json)(Board* self);
} BoardVTable;

struct Board {
    const BoardVTable* vtable;
    void* impl_data;
    char* board_type;
    char* uuid;
    bool is_initialized;
};
```

#### 实现策略
- 使用工厂模式创建特定平台的Board实例
- 通过配置文件驱动硬件参数
- 支持运行时硬件检测和配置

### 2. Audio模块 (`src/audio/`)

#### 设计目标
- 提供完整的音频处理解决方案
- 支持多种音频编解码格式
- 实现音频流的实时处理
- 支持唤醒词检测和语音处理

#### 子模块设计

##### 2.1 音频接口 (`audio/`)
```c
typedef struct AudioInterface AudioInterface;

typedef struct {
    // 生命周期
    int (*init)(AudioInterface* self);
    int (*start)(AudioInterface* self);
    int (*destroy)(AudioInterface* self);
    
    // 音频流控制
    int (*enable_input)(AudioInterface* self, bool enable);
    int (*enable_output)(AudioInterface* self, bool enable);
    
    // 数据传输
    int (*input_data)(AudioInterface* self, int16_t* data, size_t samples);
    int (*output_data)(AudioInterface* self, const int16_t* data, size_t samples);
    
    // 配置管理
    void (*set_config)(AudioInterface* self, unsigned int sample_rate,
                      int frame_size, int channels, int periods,
                      int buffer_size, int period_size);
    
    // 音量控制
    int (*set_output_volume)(AudioInterface* self, int volume);
} AudioInterfaceVTable;
```

##### 2.2 音频编解码器 (`codecs/`)
```c
typedef struct audio_codec audio_codec_t;

typedef struct {
    // 编解码器初始化
    codec_error_t (*init_encoder)(audio_codec_t* codec, const audio_format_t* format);
    codec_error_t (*init_decoder)(audio_codec_t* codec, const audio_format_t* format);
    
    // 编解码操作
    codec_error_t (*encode)(audio_codec_t* codec, const int16_t* input, size_t input_size,
                           uint8_t* output, size_t output_size, size_t* encoded_size);
    codec_error_t (*decode)(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                           int16_t* output, size_t output_size, size_t* decoded_size);
    
    // 编解码器信息
    const char* (*get_codec_name)(const audio_codec_t* codec);
    int (*get_input_frame_size)(const audio_codec_t* codec);
    int (*get_max_output_size)(const audio_codec_t* codec);
} audio_codec_vtable_t;
```

##### 2.3 音频处理器 (`processor/`)
- 音频预处理（降噪、回声消除）
- 音频后处理（均衡器、音效）
- 实时音频流处理

##### 2.4 唤醒词检测 (`wake_words/`)
- 本地唤醒词检测
- 多唤醒词支持
- 低功耗检测模式

### 3. Display模块 (`src/display/`)

#### 设计目标
- 提供统一的显示接口
- 支持多种显示设备
- 实现丰富的UI组件和主题系统
- 支持动画和交互效果

#### 接口设计
```c
typedef struct DisplayInterface DisplayInterface;

typedef struct {
    // 生命周期
    int (*init)(DisplayInterface* self);
    int (*destroy)(DisplayInterface* self);
    
    // 显示控制
    void (*set_status)(DisplayInterface* self, const char* status);
    void (*show_notification)(DisplayInterface* self, const char* notification, int duration_ms);
    void (*set_emotion)(DisplayInterface* self, const char* emotion);
    void (*set_chat_message)(DisplayInterface* self, const char* role, const char* content);
    
    // 主题管理
    void (*set_theme)(DisplayInterface* self, DisplayTheme* theme);
    DisplayTheme* (*get_theme)(DisplayInterface* self);
    
    // 电源管理
    void (*set_power_save_mode)(DisplayInterface* self, bool on);
    
    // 锁定机制
    bool (*lock)(DisplayInterface* self, int timeout_ms);
    void (*unlock)(DisplayInterface* self);
} DisplayInterfaceVTable;
```

#### LVGL集成
- 基于LVGL v9的现代UI框架
- 自定义主题和样式系统
- 表情符号和图标支持
- 多语言字体支持

### 4. Camera模块 (`src/camera/`)

#### 设计目标
- 提供统一的摄像头接口
- 支持图像捕获和处理
- 集成AI视觉分析功能

#### 接口设计
```c
typedef struct CameraInterface CameraInterface;

typedef struct {
    // 生命周期
    int (*init)(CameraInterface* self);
    int (*destroy)(CameraInterface* self);
    
    // 配置管理
    int (*set_config)(CameraInterface* self, const CameraConfig* config);
    
    // 图像捕获
    int (*capture)(CameraInterface* self, CameraFrameBuffer* frame);
    int (*release_frame)(CameraInterface* self, CameraFrameBuffer* frame);
    
    // 图像处理
    int (*set_h_mirror)(CameraInterface* self, bool enabled);
    int (*set_v_flip)(CameraInterface* self, bool enabled);
    
    // AI功能
    int (*set_explain_url)(CameraInterface* self, const char* url, const char* token);
    int (*explain)(CameraInterface* self, const char* question, char* response, size_t response_size);
} CameraInterfaceVTable;
```

### 5. Common模块 (`src/common/`)

#### 设计目标
- 提供基础设施和工具函数
- 实现跨模块的通用功能
- 提供标准库的扩展

#### 子模块设计

##### 5.1 日志系统 (`log/`)
```c
typedef enum {
    LINX_LOG_LEVEL_ERROR = 0,
    LINX_LOG_LEVEL_WARN,
    LINX_LOG_LEVEL_INFO,
    LINX_LOG_LEVEL_DEBUG
} linx_log_level_t;

void linx_log_init(linx_log_level_t level);
void linx_log(linx_log_level_t level, const char* tag, const char* format, ...);

#define LINX_LOGE(tag, format, ...) linx_log(LINX_LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define LINX_LOGW(tag, format, ...) linx_log(LINX_LOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#define LINX_LOGI(tag, format, ...) linx_log(LINX_LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define LINX_LOGD(tag, format, ...) linx_log(LINX_LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
```

##### 5.2 配置管理 (`settings.h`)
```c
typedef struct Settings Settings;

Settings* settings_create(const char* namespace, bool auto_save);
void settings_destroy(Settings* settings);

const char* settings_get_string(Settings* settings, const char* key);
bool settings_set_string(Settings* settings, const char* key, const char* value);
int settings_get_int(Settings* settings, const char* key, int default_value);
bool settings_set_int(Settings* settings, const char* key, int value);
bool settings_get_bool(Settings* settings, const char* key, bool default_value);
bool settings_set_bool(Settings* settings, const char* key, bool value);
```

##### 5.3 HTTP客户端 (`http/`)
- 基于Mongoose的HTTP客户端实现
- 支持HTTPS和认证
- 异步请求处理

##### 5.4 标准库扩展 (`std/`)
- 动态数组（vector）
- 字符串处理工具
- 内存管理工具

### 6. LinX SDK核心 (`src/linxsdk/`)

#### 设计目标
- 提供统一的SDK入口点
- 集成所有功能模块
- 实现智能语音交互功能

#### 核心接口
```c
typedef struct LinxSdk LinxSdk;

typedef struct {
    char server_url[256];
    uint32_t sample_rate;
    uint16_t channels;
    uint32_t timeout_ms;
    char auth_token[256];
    char device_id[64];
    char client_id[64];
} LinxSdkConfig;

// SDK生命周期
LinxSdk* linx_sdk_create(const LinxSdkConfig* config);
void linx_sdk_destroy(LinxSdk* sdk);

// 连接管理
LinxSdkError linx_sdk_connect(LinxSdk* sdk);
LinxSdkError linx_sdk_disconnect(LinxSdk* sdk);

// 消息处理
LinxSdkError linx_sdk_send_text(LinxSdk* sdk, const char* text);
LinxSdkError linx_sdk_send_audio(LinxSdk* sdk, const uint8_t* data, size_t size);

// 事件处理
LinxSdkError linx_sdk_set_event_callback(LinxSdk* sdk, LinxEventCallback callback, void* user_data);
```

#### 子模块设计

##### 6.1 MCP协议 (`mcp/`)
- Model Context Protocol实现
- 工具调用和属性管理
- 服务器和客户端支持

##### 6.2 通信协议 (`protocols/`)
- WebSocket协议实现
- 自定义LinX协议
- 消息序列化和反序列化

##### 6.3 OTA更新 (`ota/`)
- 固件更新管理
- 增量更新支持
- 安全验证机制

## 构建系统设计

### CMake配置策略

#### 1. 分层构建
```cmake
# 主CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(linx_sdk C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 第三方库
add_subdirectory(third/mongoose)
add_subdirectory(third/opus)
add_subdirectory(third/fonts)
add_subdirectory(third/liblvgl)

# 核心模块
add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(audio)
add_subdirectory(display)
add_subdirectory(linxsdk)
```

#### 2. 模块化构建
每个模块都有独立的CMakeLists.txt：
- 定义模块特定的源文件
- 配置模块依赖关系
- 设置编译选项和链接库

#### 3. 平台适配
```cmake
# 平台特定配置
if(ESP32)
    add_definitions(-DESP32_PLATFORM)
    include(cmake/esp32.cmake)
elseif(APPLE)
    add_definitions(-DMACOS_PLATFORM)
    include(cmake/macos.cmake)
endif()
```

### 依赖管理

#### 1. 第三方库管理
- Mongoose: HTTP/WebSocket通信
- Opus: 音频编解码
- LVGL: 图形用户界面
- cJSON: JSON处理

#### 2. 模块依赖关系
```
linxsdk -> audio, display, camera, common
audio -> common
display -> common
camera -> common
common -> (基础模块，无依赖)
```

## 跨平台兼容性设计

### 1. 接口抽象
- 所有平台相关功能通过接口抽象
- 使用虚函数表实现多态
- 编译时选择具体实现

### 2. 条件编译
```c
#ifdef ESP32_PLATFORM
    #include "esp32_audio_impl.h"
#elif defined(MACOS_PLATFORM)
    #include "macos_audio_impl.h"
#else
    #include "stub_audio_impl.h"
#endif
```

### 3. 配置驱动
- 使用JSON配置文件定义硬件参数
- 运行时读取配置并初始化硬件
- 支持配置的动态更新

## 测试策略

### 1. 单元测试
- 每个模块都有对应的测试用例
- 使用模拟对象测试接口
- 自动化测试集成到CI/CD

### 2. 集成测试
- 模块间接口测试
- 端到端功能测试
- 性能和稳定性测试

### 3. 平台测试
- 在目标平台上进行实际测试
- 硬件兼容性验证
- 功能完整性检查

## 文档和示例

### 1. API文档
- 详细的接口文档
- 使用示例和最佳实践
- 平台特定的配置指南

### 2. 示例代码
- 基本功能演示
- 完整应用示例
- 平台移植指南

### 3. 开发指南
- 模块开发规范
- 代码风格指南
- 贡献指南

## 性能优化

### 1. 内存管理
- 最小化内存分配
- 对象池和缓存机制
- 内存泄漏检测

### 2. 实时性能
- 音频处理的实时性保证
- 中断处理优化
- 任务调度优化

### 3. 功耗优化
- 动态功耗管理
- 硬件模块的按需启用
- 睡眠模式支持

## 安全性设计

### 1. 数据安全
- 敏感数据加密存储
- 安全的通信协议
- 访问控制机制

### 2. 代码安全
- 缓冲区溢出防护
- 输入验证和清理
- 安全编码规范

### 3. 更新安全
- 固件签名验证
- 安全的OTA更新
- 回滚机制

## 未来扩展

### 1. 新平台支持
- 模块化设计便于添加新平台
- 标准化的移植接口
- 平台特定优化

### 2. 新功能模块
- 插件式架构支持
- 动态模块加载
- 功能的热更新

### 3. 性能提升
- 硬件加速支持
- 算法优化
- 并行处理能力

## 总结

LinX OS SDK的src目录架构设计遵循了模块化、分层化和跨平台的设计原则。通过清晰的接口定义、统一的C99标准和灵活的构建系统，实现了一个可扩展、可维护和高性能的智能语音交互SDK。

这个架构不仅支持当前的ESP32和macOS平台，还为未来的平台扩展和功能增强提供了坚实的基础。通过持续的优化和改进，LinX OS SDK将成为智能设备开发的强大工具。