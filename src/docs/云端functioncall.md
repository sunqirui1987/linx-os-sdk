# LinxOS 云端 Function Call 集成指南

## 概述

本文档介绍如何在 LinxOS 中集成云端 Function Call 功能。LinxOS 已经内置了完整的 MCP (Model Context Protocol) 服务器实现，支持工具注册、调用和管理。云端 Function Call 功能基于现有的 WebSocket + JSON 通信架构，无需额外的 gRPC 服务器。

## 核心架构

LinxOS 的云端 Function Call 基于以下现有组件：

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   云端 AI 服务   │    │   LinxOS 设备    │    │   MCP 服务器     │
│                │    │                │    │                │
│ - Function Call │◄──►│ - WebSocket     │◄──►│ - 工具注册       │
│ - 参数解析      │    │ - JSON 协议     │    │ - 工具调用       │
│ - 结果返回      │    │ - 事件处理      │    │ - 结果返回       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 快速开始

### 1. 基础工具注册

```c
#include "linx_sdk.h"
#include "mcp/mcp_server.h"

// 创建 MCP 服务器
mcp_server_t* server = mcp_server_create("linx_device", "1.0.0");

// 注册简单工具
bool success = mcp_server_add_simple_tool(
    server,
    "get_system_info",
    "获取系统信息",
    NULL,  // 无参数
    system_info_callback
);
```

### 2. 工具回调实现

```c
// 系统信息工具回调
static mcp_return_value_t system_info_callback(const mcp_property_list_t* properties) {
    // 获取系统信息
    char result[512];
    snprintf(result, sizeof(result), 
        "{\"cpu_usage\": \"%.1f%%\", \"memory_usage\": \"%.1f%%\", \"uptime\": \"%ld\"}",
        get_cpu_usage(), get_memory_usage(), get_uptime());
    
    return mcp_return_string(result);
}
```

### 3. 集成到 LinxOS 应用

```c
#include "linx_sdk.h"
#include "mcp/mcp_server.h"

int main() {
    // 初始化 LinxOS SDK
    LinxSdk* sdk = linx_sdk_create();
    
    // 创建并配置 MCP 服务器
    mcp_server_t* mcp_server = mcp_server_create("my_device", "1.0.0");
    setup_device_tools(mcp_server);
    
    // 设置 MCP 服务器到 SDK
    linx_sdk_set_mcp_server(sdk, mcp_server);
    
    // 连接到云端服务
    linx_sdk_connect(sdk, "ws://your-server.com/v1/ws/");
    
    // 运行主循环
    linx_sdk_run(sdk);
    
    return 0;
}
```

## 实用工具示例

### 音频录制工具

```c
// 音频录制工具回调
static mcp_return_value_t audio_record_callback(const mcp_property_list_t* properties) {
    // 解析参数
    int duration = 5; // 默认5秒
    if (properties) {
        const mcp_property_t* duration_prop = mcp_property_list_find(properties, "duration");
        if (duration_prop && duration_prop->type == MCP_PROPERTY_TYPE_INTEGER) {
            duration = duration_prop->value.integer_value;
        }
    }
    
    // 开始录制
    AudioInterface* audio = get_audio_interface();
    if (!audio) {
        return mcp_return_error("音频接口不可用");
    }
    
    // 录制音频
    audio_buffer_t buffer;
    linx_audio_result_t result = audio_interface_record(audio, duration * 1000, &buffer);
    
    if (result != LINX_AUDIO_SUCCESS) {
        return mcp_return_error("录制失败");
    }
    
    // 编码为 Base64
    char* encoded = mcp_base64_encode((char*)buffer.data, buffer.size);
    
    // 构造返回结果
    char response[1024];
    snprintf(response, sizeof(response),
        "{\"status\": \"success\", \"duration\": %d, \"format\": \"pcm_s16le\", \"sample_rate\": 16000, \"data\": \"%s\"}",
        duration, encoded);
    
    free(encoded);
    audio_buffer_free(&buffer);
    
    return mcp_return_string(response);
}

// 注册音频录制工具
void register_audio_tools(mcp_server_t* server) {
    // 创建参数列表
    mcp_property_list_t* properties = mcp_property_list_create();
    mcp_property_list_add_integer(properties, "duration", "录制时长（秒）", 5, false);
    
    mcp_server_add_simple_tool(server, "audio_record", "录制音频", properties, audio_record_callback);
}
```

### 系统监控工具

```c
// 系统监控工具回调
static mcp_return_value_t system_monitor_callback(const mcp_property_list_t* properties) {
    // 获取系统状态
    system_stats_t stats;
    get_system_stats(&stats);
    
    // 构造 JSON 响应
    char response[512];
    snprintf(response, sizeof(response),
        "{"
        "\"cpu_usage\": %.1f,"
        "\"memory_total\": %lu,"
        "\"memory_used\": %lu,"
        "\"memory_free\": %lu,"
        "\"uptime\": %ld,"
        "\"temperature\": %.1f"
        "}",
        stats.cpu_usage,
        stats.memory_total,
        stats.memory_used,
        stats.memory_free,
        stats.uptime,
        stats.temperature
    );
    
    return mcp_return_string(response);
}

// 注册系统监控工具
void register_system_tools(mcp_server_t* server) {
    mcp_server_add_simple_tool(server, "system_monitor", "获取系统监控信息", NULL, system_monitor_callback);
}
```

### 设备控制工具

```c
// LED 控制工具回调
static mcp_return_value_t led_control_callback(const mcp_property_list_t* properties) {
    if (!properties) {
        return mcp_return_error("缺少必要参数");
    }
    
    // 解析参数
    const mcp_property_t* action_prop = mcp_property_list_find(properties, "action");
    const mcp_property_t* color_prop = mcp_property_list_find(properties, "color");
    
    if (!action_prop || action_prop->type != MCP_PROPERTY_TYPE_STRING) {
        return mcp_return_error("缺少 action 参数");
    }
    
    const char* action = action_prop->value.string_value;
    
    if (strcmp(action, "on") == 0) {
        uint32_t color = 0xFFFFFF; // 默认白色
        if (color_prop && color_prop->type == MCP_PROPERTY_TYPE_STRING) {
            color = parse_color(color_prop->value.string_value);
        }
        led_set_color(color);
        led_turn_on();
    } else if (strcmp(action, "off") == 0) {
        led_turn_off();
    } else {
        return mcp_return_error("不支持的操作");
    }
    
    return mcp_return_string("{\"status\": \"success\"}");
}

// 注册设备控制工具
void register_device_tools(mcp_server_t* server) {
    mcp_property_list_t* properties = mcp_property_list_create();
    mcp_property_list_add_string(properties, "action", "操作类型 (on/off)", "on", true);
    mcp_property_list_add_string(properties, "color", "LED颜色 (hex)", "#FFFFFF", false);
    
    mcp_server_add_simple_tool(server, "led_control", "控制LED灯", properties, led_control_callback);
}
```

## 完整应用示例

```c
#include "linx_sdk.h"
#include "mcp/mcp_server.h"
#include "audio/audio_interface.h"
#include "log/linx_log.h"

// 全局变量
static LinxSdk* g_sdk = NULL;
static mcp_server_t* g_mcp_server = NULL;
static bool g_running = true;

// 设置所有工具
void setup_all_tools(mcp_server_t* server) {
    register_audio_tools(server);
    register_system_tools(server);
    register_device_tools(server);
    
    LOG_INFO("已注册 %zu 个工具", server->tool_count);
}

// 事件处理回调
void event_handler(const LinxEvent* event, void* user_data) {
    switch (event->type) {
        case LINX_EVENT_CONNECTED:
            LOG_INFO("已连接到云端服务");
            break;
            
        case LINX_EVENT_DISCONNECTED:
            LOG_INFO("与云端服务断开连接");
            break;
            
        case LINX_EVENT_FUNCTION_CALL:
            LOG_INFO("收到 Function Call: %s", event->data.function_call.name);
            break;
            
        case LINX_EVENT_ERROR:
            LOG_ERROR("错误: %s", event->data.error.message);
            break;
            
        default:
            break;
    }
}

// 信号处理
void signal_handler(int sig) {
    LOG_INFO("收到信号 %d，正在退出...", sig);
    g_running = false;
    if (g_sdk) {
        linx_sdk_stop(g_sdk);
    }
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化日志
    linx_log_init(LINX_LOG_LEVEL_INFO);
    
    // 创建 LinxOS SDK
    g_sdk = linx_sdk_create();
    if (!g_sdk) {
        LOG_ERROR("创建 LinxOS SDK 失败");
        return -1;
    }
    
    // 创建 MCP 服务器
    g_mcp_server = mcp_server_create("smart_device", "1.0.0");
    if (!g_mcp_server) {
        LOG_ERROR("创建 MCP 服务器失败");
        linx_sdk_destroy(g_sdk);
        return -1;
    }
    
    // 设置工具
    setup_all_tools(g_mcp_server);
    
    // 配置 SDK
    linx_sdk_set_mcp_server(g_sdk, g_mcp_server);
    linx_sdk_set_event_handler(g_sdk, event_handler, NULL);
    
    // 连接到云端服务
    const char* server_url = (argc > 1) ? argv[1] : "ws://localhost:8080/v1/ws/";
    if (linx_sdk_connect(g_sdk, server_url) != LINX_SUCCESS) {
        LOG_ERROR("连接到服务器失败: %s", server_url);
        goto cleanup;
    }
    
    LOG_INFO("LinxOS Function Call 服务已启动");
    LOG_INFO("服务器地址: %s", server_url);
    LOG_INFO("已注册工具数量: %zu", g_mcp_server->tool_count);
    
    // 运行主循环
    while (g_running) {
        linx_sdk_process_events(g_sdk, 100); // 100ms 超时
    }
    
cleanup:
    LOG_INFO("正在清理资源...");
    
    if (g_sdk) {
        linx_sdk_disconnect(g_sdk);
        linx_sdk_destroy(g_sdk);
    }
    
    if (g_mcp_server) {
        mcp_server_destroy(g_mcp_server);
    }
    
    linx_log_cleanup();
    
    LOG_INFO("程序退出");
    return 0;
}
```

## 编译和部署

### CMakeLists.txt 配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(linx_function_call_demo)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)

# 查找 LinxOS SDK
find_package(LinxSDK REQUIRED)

# 添加可执行文件
add_executable(function_call_demo
    main.c
    tools/audio_tools.c
    tools/system_tools.c
    tools/device_tools.c
)

# 链接库
target_link_libraries(function_call_demo
    LinxSDK::Core
    LinxSDK::MCP
    LinxSDK::Audio
    LinxSDK::Protocol
)

# 包含目录
target_include_directories(function_call_demo PRIVATE
    ${LinxSDK_INCLUDE_DIRS}
)
```

### 编译命令

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j4

# 运行
./function_call_demo ws://your-server.com/v1/ws/
```

## 最佳实践

### 1. 错误处理

```c
// 统一的错误处理
static mcp_return_value_t handle_tool_error(const char* tool_name, const char* error_msg) {
    LOG_ERROR("工具 %s 执行失败: %s", tool_name, error_msg);
    
    char error_response[256];
    snprintf(error_response, sizeof(error_response),
        "{\"error\": \"%s\", \"tool\": \"%s\"}", error_msg, tool_name);
    
    return mcp_return_error(error_response);
}
```

### 2. 参数验证

```c
// 参数验证辅助函数
static bool validate_required_param(const mcp_property_list_t* properties, 
                                   const char* param_name, 
                                   mcp_property_type_t expected_type) {
    if (!properties) return false;
    
    const mcp_property_t* prop = mcp_property_list_find(properties, param_name);
    return prop && prop->type == expected_type;
}
```

### 3. 资源管理

```c
// 自动清理宏
#define AUTO_CLEANUP(cleanup_func) __attribute__((cleanup(cleanup_func)))

static void cleanup_buffer(audio_buffer_t** buffer) {
    if (buffer && *buffer) {
        audio_buffer_free(*buffer);
        *buffer = NULL;
    }
}

// 使用示例
static mcp_return_value_t safe_audio_tool(const mcp_property_list_t* properties) {
    AUTO_CLEANUP(cleanup_buffer) audio_buffer_t* buffer = NULL;
    
    // 使用 buffer，函数结束时自动清理
    // ...
    
    return mcp_return_string("{\"status\": \"success\"}");
}
```

## 性能优化

### 1. 缓冲池管理

```c
// 音频缓冲池
static audio_buffer_t* g_buffer_pool[MAX_BUFFERS];
static size_t g_pool_size = 0;
static pthread_mutex_t g_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

audio_buffer_t* get_buffer_from_pool(size_t size) {
    pthread_mutex_lock(&g_pool_mutex);
    
    for (size_t i = 0; i < g_pool_size; i++) {
        if (g_buffer_pool[i] && g_buffer_pool[i]->capacity >= size) {
            audio_buffer_t* buffer = g_buffer_pool[i];
            g_buffer_pool[i] = NULL;
            pthread_mutex_unlock(&g_pool_mutex);
            return buffer;
        }
    }
    
    pthread_mutex_unlock(&g_pool_mutex);
    return audio_buffer_create(size);
}
```

### 2. 异步处理

```c
// 异步工具执行
typedef struct {
    mcp_server_t* server;
    int request_id;
    char tool_name[64];
    mcp_property_list_t* properties;
} async_tool_context_t;

static void* async_tool_worker(void* arg) {
    async_tool_context_t* ctx = (async_tool_context_t*)arg;
    
    // 执行工具
    const mcp_tool_t* tool = mcp_server_find_tool(ctx->server, ctx->tool_name);
    if (tool) {
        mcp_return_value_t result = tool->callback(ctx->properties);
        // 发送结果
        mcp_server_reply_result(ctx->request_id, result.value.string_value);
    }
    
    // 清理资源
    mcp_property_list_destroy(ctx->properties);
    free(ctx);
    
    return NULL;
}
```

## 故障排除

### 常见问题

1. **工具注册失败**
   - 检查工具名称是否重复
   - 验证回调函数是否有效
   - 确认 MCP 服务器已正确初始化

2. **参数解析错误**
   - 验证参数类型匹配
   - 检查必需参数是否提供
   - 确认 JSON 格式正确

3. **音频录制失败**
   - 检查音频设备权限
   - 验证音频接口初始化
   - 确认采样率和格式支持

### 调试技巧

```c
// 启用详细日志
linx_log_set_level(LINX_LOG_LEVEL_DEBUG);

// 工具调用跟踪
#define TRACE_TOOL_CALL(name) \
    LOG_DEBUG("工具调用开始: %s", name); \
    struct timeval start_time; \
    gettimeofday(&start_time, NULL);

#define TRACE_TOOL_RETURN(name) \
    struct timeval end_time; \
    gettimeofday(&end_time, NULL); \
    long duration = (end_time.tv_sec - start_time.tv_sec) * 1000000 + \
                   (end_time.tv_usec - start_time.tv_usec); \
    LOG_DEBUG("工具调用完成: %s, 耗时: %ld μs", name, duration);
```

## 总结

LinxOS 的云端 Function Call 功能基于现有的 MCP 架构，提供了简洁而强大的工具注册和调用机制。通过本指南，您可以：

1. 快速集成云端 Function Call 功能
2. 实现各种实用的设备工具
3. 构建完整的智能设备应用
4. 优化性能和处理错误

关键优势：
- **简单易用**：基于现有 MCP 架构，无需额外复杂配置
- **高度集成**：与 LinxOS 核心功能深度集成
- **性能优化**：针对嵌入式设备优化的轻量级实现
- **扩展性强**：支持自定义工具和复杂业务逻辑

更多信息请参考 [LinxOS SDK 文档](../README.md) 和 [MCP 协议规范](../mcp/README.md)。