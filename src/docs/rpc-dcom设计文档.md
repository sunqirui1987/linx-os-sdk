# LinxOS RPC-DCOM 嵌入式分布式通信设计文档

> **版本**: v2.0  
> **更新日期**: 2024年12月  
> **目标平台**: ESP32, ARM Cortex-M, RISC-V  
> **协议**: 简化RPC + WebSocket/TCP  
> **语言标准**: C99  
> **设计理念**: 轻量级分布式组件通信  

---

## 1. 概述

### 1.1 设计目标

LinxOS RPC-DCOM 是一个专为嵌入式RTOS设计的轻量级分布式通信框架。它简化了传统DCOM的复杂性，专注于：

- **内存友好**: 适配ESP32等资源受限设备（RAM < 512KB）
- **简单可靠**: 最小化协议开销，提高通信可靠性
- **实时响应**: 支持音频、摄像头等实时数据流
- **易于集成**: 简单的C API，便于嵌入式开发

### 1.2 系统约束

- **最大连接数**: 5个并发连接
- **缓冲区大小**: 4KB网络缓冲区
- **内存限制**: 总内存占用 < 64KB
- **实时性**: 音频延迟 < 100ms

---

## 2. 快速开始 

### 2.1 简单的RPC服务器 (ESP32设备端)

```c
// linx_rpc_server.c - ESP32设备端RPC服务器
#include "linx_rpc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 定义服务接口
typedef struct {
    int (*get_system_info)(char* buffer, size_t size);
    int (*play_audio)(const uint8_t* data, size_t len);
    int (*capture_image)(uint8_t* buffer, size_t* size);
    int (*set_wifi)(const char* ssid, const char* password);
} linx_device_service_t;

// 服务实现
static int device_get_system_info(char* buffer, size_t size) {
    snprintf(buffer, size, 
        "{\"device\":\"ESP32\",\"memory\":%d,\"uptime\":%ld}", 
        esp_get_free_heap_size(), 
        xTaskGetTickCount() * portTICK_PERIOD_MS);
    return 0;
}

static int device_play_audio(const uint8_t* data, size_t len) {
    // 调用音频播放接口
    return audio_play(data, len);
}

static int device_capture_image(uint8_t* buffer, size_t* size) {
    // 调用摄像头接口
    return camera_capture(buffer, size);
}

static int device_set_wifi(const char* ssid, const char* password) {
    // 设置WiFi连接
    return wifi_connect(ssid, password);
}

// 服务注册
static linx_device_service_t device_service = {
    .get_system_info = device_get_system_info,
    .play_audio = device_play_audio,
    .capture_image = device_capture_image,
    .set_wifi = device_set_wifi
};

void app_main() {
    // 初始化RPC服务器
    linx_rpc_config_t config = {
        .port = 8080,
        .max_connections = 5,
        .buffer_size = 4096,
        .timeout_ms = 5000
    };
    
    linx_rpc_server_t* server = linx_rpc_server_create(&config);
    
    // 注册服务
    linx_rpc_register_service(server, "device", &device_service);
    
    // 启动服务器
    linx_rpc_server_start(server);
    
    printf("LinxOS RPC Server started on port 8080\n");
    
    // 主循环
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

### 2.2 简单的RPC客户端 (服务器)

```c
// linx_rpc_client.c - 控制端RPC客户端
#include "linx_rpc.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 连接到ESP32设备
    linx_rpc_client_t* client = linx_rpc_client_create("192.168.1.100", 8080);
    
    if (linx_rpc_client_connect(client) != 0) {
        printf("Failed to connect to device\n");
        return -1;
    }
    
    printf("Connected to LinxOS device\n");
    
    // 1. 获取系统信息
    char info_buffer[512];
    if (linx_rpc_call(client, "device.get_system_info", 
                      NULL, 0, info_buffer, sizeof(info_buffer)) == 0) {
        printf("System Info: %s\n", info_buffer);
    }
    
    // 2. 播放音频
    uint8_t audio_data[] = {0x01, 0x02, 0x03, 0x04}; // 示例音频数据
    if (linx_rpc_call(client, "device.play_audio", 
                      audio_data, sizeof(audio_data), NULL, 0) == 0) {
        printf("Audio playback started\n");
    }
    
    // 3. 拍照
    uint8_t image_buffer[8192];
    size_t image_size = sizeof(image_buffer);
    if (linx_rpc_call(client, "device.capture_image", 
                      NULL, 0, image_buffer, &image_size) == 0) {
        printf("Image captured, size: %zu bytes\n", image_size);
    }
    
    // 4. 设置WiFi
    const char* wifi_config = "{\"ssid\":\"MyWiFi\",\"password\":\"123456\"}";
    if (linx_rpc_call(client, "device.set_wifi", 
                      wifi_config, strlen(wifi_config), NULL, 0) == 0) {
        printf("WiFi configuration updated\n");
    }
    
    // 清理资源
    linx_rpc_client_destroy(client);
    return 0;
}
```


## 3. 服务端核心架构设计

### 3.1 服务端分层架构

```
┌─────────────────────────────────────┐
│         应用服务层 (Services)        │
│  ┌─────────────┐ ┌─────────────┐    │
│  │  音频服务   │ │  摄像头服务 │    │
│  │  系统服务   │ │  网络服务   │    │
│  └─────────────┘ └─────────────┘    │
├─────────────────────────────────────┤
│         RPC处理层 (RPC Core)        │
│  ┌─────────────┐ ┌─────────────┐    │
│  │  方法分发   │ │  消息序列化 │    │
│  │  流式处理   │ │  错误处理   │    │
│  └─────────────┘ └─────────────┘    │
├─────────────────────────────────────┤
│        网络传输层 (Transport)       │
│  ┌─────────────┐ ┌─────────────┐    │
│  │  WebSocket  │ │    TCP      │    │
│  │  连接管理   │ │  心跳检测   │    │
│  └─────────────┘ └─────────────┘    │
└─────────────────────────────────────┘
```

### 3.2 核心数据结构


#### 3.2.2 服务器配置和状态

```c
// RPC服务器配置
typedef struct {
    // 网络配置
    uint16_t port;                    // 监听端口
    char bind_addr[64];               // 绑定地址 ("0.0.0.0" 或具体IP)
    uint8_t max_connections;          // 最大连接数 (1-5)
    uint16_t buffer_size;             // 缓冲区大小 (建议4KB)
    uint32_t timeout_ms;              // 超时时间 (毫秒)
    
    // 性能配置
    uint8_t worker_threads;           // 工作线程数 (ESP32建议1-2)
    uint16_t max_pending_requests;    // 最大待处理请求数
    uint32_t keepalive_interval_ms;   // 心跳间隔
    
    // 安全配置
    bool enable_auth;                 // 是否启用认证
    char auth_token[64];              // 认证令牌
    
    // 调试配置
    bool enable_debug;                // 是否启用调试
    uint8_t log_level;                // 日志级别 (0-5)
} linx_rpc_config_t;

```

#### 3.2.3 服务方法注册和回调

```c
// 服务方法回调函数类型
typedef int (*linx_rpc_method_t)(
    const char* method,               // 方法名
    const void* input,                // 输入数据
    size_t input_len,                 // 输入数据长度
    void* output,                     // 输出缓冲区
    size_t* output_len,               // 输出数据长度(输入时为缓冲区大小)
    void* user_data                   // 用户数据
);

// 流式数据回调函数类型
typedef int (*linx_rpc_stream_handler_t)(
    const char* method,               // 方法名
    const void* data,                 // 流数据
    size_t data_len,                  // 数据长度
    bool is_end,                      // 是否为流结束
    void* user_data                   // 用户数据
);

// 服务方法注册表项
typedef struct {
    char name[64];                    // 方法名
    linx_rpc_method_t handler;        // 处理函数
    linx_rpc_stream_handler_t stream_handler; // 流处理函数(可选)
    void* user_data;                  // 用户数据
    uint32_t flags;                   // 方法标志
    char description[128];            // 方法描述
} linx_rpc_method_entry_t;

```

### 3.3 服务端核心API

#### 3.3.1 服务器生命周期管理

```c
/**
 * 创建RPC服务器
 * @param config 服务器配置
 * @return 服务器实例，失败返回NULL
 */
linx_rpc_server_t* linx_rpc_server_create(const linx_rpc_config_t* config);

/**
 * 启动RPC服务器
 * @param server 服务器实例
 * @return 0成功，负数为错误码
 */
int linx_rpc_server_start(linx_rpc_server_t* server);

/**
 * 停止RPC服务器
 * @param server 服务器实例
 * @return 0成功，负数为错误码
 */
int linx_rpc_server_stop(linx_rpc_server_t* server);

/**
 * 销毁RPC服务器
 * @param server 服务器实例
 */
void linx_rpc_server_destroy(linx_rpc_server_t* server);

/**
 * 获取服务器状态
 * @param server 服务器实例
 * @return 服务器状态
 */
linx_rpc_server_state_t linx_rpc_server_get_state(linx_rpc_server_t* server);

/**
 * 获取服务器统计信息
 * @param server 服务器实例
 * @param stats 统计信息输出
 * @return 0成功，负数为错误码
 */
int linx_rpc_server_get_stats(linx_rpc_server_t* server, linx_rpc_server_stats_t* stats);
```

#### 3.3.2 服务方法注册管理

```c
/**
 * 注册RPC方法
 * @param server 服务器实例
 * @param name 方法名
 * @param handler 处理函数
 * @param user_data 用户数据
 * @return 0成功，负数为错误码
 */
int linx_rpc_register_method(linx_rpc_server_t* server, 
                            const char* name, 
                            linx_rpc_method_t handler,
                            void* user_data);

/**
 * 注册流式RPC方法
 * @param server 服务器实例
 * @param name 方法名
 * @param stream_handler 流处理函数
 * @param user_data 用户数据
 * @return 0成功，负数为错误码
 */
int linx_rpc_register_stream_method(linx_rpc_server_t* server,
                                   const char* name,
                                   linx_rpc_stream_handler_t stream_handler,
                                   void* user_data);

/**
 * 注销RPC方法
 * @param server 服务器实例
 * @param name 方法名
 * @return 0成功，负数为错误码
 */
int linx_rpc_unregister_method(linx_rpc_server_t* server, const char* name);

/**
 * 列出所有注册的方法
 * @param server 服务器实例
 * @param methods 方法列表输出
 * @param max_count 最大方法数
 * @return 实际方法数，负数为错误码
 */
int linx_rpc_list_methods(linx_rpc_server_t* server, 
                         linx_rpc_method_entry_t* methods, 
                         size_t max_count);
```

#### 3.3.3 客户端连接管理

```c
/**
 * 客户端连接事件回调
 */
typedef void (*linx_rpc_client_connect_callback_t)(
    linx_rpc_server_t* server,
    const linx_rpc_client_info_t* client_info,
    void* user_data
);

/**
 * 客户端断开事件回调
 */
typedef void (*linx_rpc_client_disconnect_callback_t)(
    linx_rpc_server_t* server,
    const linx_rpc_client_info_t* client_info,
    void* user_data
);

/**
 * 设置客户端连接回调
 */
int linx_rpc_server_set_connect_callback(linx_rpc_server_t* server,
                                        linx_rpc_client_connect_callback_t callback,
                                        void* user_data);

/**
 * 设置客户端断开回调
 */
int linx_rpc_server_set_disconnect_callback(linx_rpc_server_t* server,
                                           linx_rpc_client_disconnect_callback_t callback,
                                           void* user_data);

/**
 * 获取当前连接的客户端列表
 * @param server 服务器实例
 * @param clients 客户端信息数组
 * @param max_count 最大客户端数
 * @return 实际客户端数，负数为错误码
 */
int linx_rpc_server_get_clients(linx_rpc_server_t* server,
                               linx_rpc_client_info_t* clients,
                               size_t max_count);

/**
 * 断开指定客户端连接
 * @param server 服务器实例
 * @param client_fd 客户端文件描述符
 * @return 0成功，负数为错误码
 */
int linx_rpc_server_disconnect_client(linx_rpc_server_t* server, int client_fd);
```

#### 3.3.4 工具和调试函数

```c
/**
 * 获取错误描述
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char* linx_rpc_get_error_string(int error_code);

/**
 * 设置日志回调
 * @param callback 日志回调函数
 * @param user_data 用户数据
 */
typedef void (*linx_rpc_log_callback_t)(int level, const char* message, void* user_data);
void linx_rpc_set_log_callback(linx_rpc_log_callback_t callback, void* user_data);

/**
 * 向指定客户端发送通知消息
 * @param server 服务器实例
 * @param client_fd 客户端文件描述符
 * @param method 方法名
 * @param data 数据
 * @param data_len 数据长度
 * @return 0成功，负数为错误码
 */
int linx_rpc_server_notify_client(linx_rpc_server_t* server,
                                 int client_fd,
                                 const char* method,
                                 const void* data,
                                 size_t data_len);

/**
 * 广播通知消息给所有客户端
 * @param server 服务器实例
 * @param method 方法名
 * @param data 数据
 * @param data_len 数据长度
 * @return 成功发送的客户端数，负数为错误码
 */
int linx_rpc_server_broadcast(linx_rpc_server_t* server,
                            const char* method,
                            const void* data,
                            size_t data_len);
```

### 3.4 客户端接口说明

> **注意**: 客户端API设计相对简单，主要包含连接、调用、断开等基本功能。
> 具体的客户端实现可以根据不同平台和语言的需求进行定制。

#### 基本客户端接口概览

```c
// 基本客户端接口 (简化版本，供参考)
typedef struct linx_rpc_client linx_rpc_client_t;

// 创建客户端
linx_rpc_client_t* linx_rpc_client_create(const char* host, uint16_t port);

// 连接服务器
int linx_rpc_client_connect(linx_rpc_client_t* client);

// 同步调用
int linx_rpc_call(linx_rpc_client_t* client, const char* method,
                  const void* input, size_t input_len,
                  void* output, size_t* output_len);

// 流式传输
int linx_rpc_stream_start(linx_rpc_client_t* client, const char* method);
int linx_rpc_stream_send(linx_rpc_client_t* client, const void* data, size_t len);
int linx_rpc_stream_recv(linx_rpc_client_t* client, void* data, size_t* len);
int linx_rpc_stream_stop(linx_rpc_client_t* client);

// 断开连接
int linx_rpc_client_disconnect(linx_rpc_client_t* client);

// 销毁客户端
void linx_rpc_client_destroy(linx_rpc_client_t* client);
```

---

## 4. 实际应用场景

### 4.1 音频流传输

```c
// 音频流服务器端
static int audio_stream_handler(const void* input, size_t input_len, 
                               void* output, size_t* output_len) {
    // 接收音频数据并播放
    const uint8_t* audio_data = (const uint8_t*)input;
    return audio_play_pcm(audio_data, input_len);
}

// 音频流客户端
void stream_audio_to_device(linx_rpc_client_t* client, const char* audio_file) {
    FILE* fp = fopen(audio_file, "rb");
    if (!fp) return;
    
    linx_rpc_stream_start(client, "audio.stream");
    
    uint8_t buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        linx_rpc_stream_send(client, buffer, bytes_read);
        usleep(20000); // 20ms间隔，模拟实时流
    }
    
    linx_rpc_stream_stop(client);
    fclose(fp);
}
```

### 4.2 摄像头控制

```c
// 摄像头服务
static int camera_start_handler(const void* input, size_t input_len, 
                               void* output, size_t* output_len) {
    // 解析摄像头配置
    const char* config = (const char*)input;
    // 启动摄像头
    return camera_start_with_config(config);
}

static int camera_capture_handler(const void* input, size_t input_len, 
                                 void* output, size_t* output_len) {
    // 拍照并返回图像数据
    return camera_capture_jpeg(output, output_len);
}

// 注册摄像头服务
void register_camera_service(linx_rpc_server_t* server) {
    linx_rpc_register_method(server, "camera.start", camera_start_handler);
    linx_rpc_register_method(server, "camera.capture", camera_capture_handler);
}
```
