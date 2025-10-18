# LinxOS 智能硬件 API 协议定义

> **版本**: v2.0  
> **更新日期**: 2024年12月  
> **适用平台**: ESP32, ARM, RISC-V, x86  

---

## 📋 目录

- [1. 概述](#1-概述)
- [2. 设计理念](#2-设计理念)
- [3. 快速开始](#3-快速开始)
- [4. 核心API模块](#4-核心api模块)
  - [4.1 网络通信](#41-网络通信)
  - [4.2 音频系统](#42-音频系统)
  - [4.3 摄像头](#43-摄像头)
  - [4.4 显示系统](#44-显示系统)
  - [4.5 任务管理](#45-任务管理)
  - [4.6 系统工具](#46-系统工具)
  - [4.7 系统信息](#47-系统信息)
- [5. 数据结构定义](#5-数据结构定义)
- [6. 错误码与异常处理](#6-错误码与异常处理)
- [7. 应用示例](#7-应用示例)
- [8. 开发指南](#8-开发指南)
- [9. 性能优化](#9-性能优化)
- [10. 常见问题](#10-常见问题)

---

## 1. 概述

LinxOS 是专为智能硬件设计的轻量级操作系统API框架，提供精炼、实用的接口集合。专注于核心功能：**网络通信**、**音频处理**、**摄像头控制**、**显示管理**、**任务调度**和**系统监控**。

通过内置的对外服务器架构，LinxOS 提供双重通信能力：**WebSocket服务**用于实时控制与数据流传输，**gRPC服务**提供类似dcom的结构化远程调用能力。

### 🎯 核心特性

- **🚀 精炼实用**: 只保留最核心、最常用的功能，一个函数解决一个问题
- **⚡ 高效稳定**: 针对嵌入式硬件深度优化，内存占用 < 512KB
- **🔧 统一接口**: 所有平台使用相同API，跨平台代码无需修改
- **🌐 双重通信**: WebSocket实时通信 + gRPC结构化调用
- **📡 即插即用**: 无需复杂配置，开箱即用

---

## 2. 设计理念


### 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                      │
├─────────────────────────────────────────────────────────────┤
│                    LinxOS API 层                            │
├─────────────────────────────────────────────────────────────┤
│                   对外服务器 (External Server)               │
│  ┌─────────────────────────┐  ┌─────────────────────────┐   │
│  │    WebSocket 服务       │  │      gRPC 服务          │   │
│  │   (控制与数据流)         │  │   (类似 dcom 能力)       │   │
│  └─────────────────────────┘  └─────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                   硬件抽象层 (HAL)                           │
├─────────────────────────────────────────────────────────────┤
│                   底层驱动 (Drivers)                         │
└─────────────────────────────────────────────────────────────┘
```

---


### 🚀 Hello World

```c
#include "linx_api.h"

int main(void) {
    // 初始化系统
    linx_init();
    
    // 初始化显示
    display_init();
    display_text_center("Hello LinxOS!");
    
    // 连接WiFi
    if (wifi_connect("MyWiFi", "password") == LINX_OK) {
        display_text_center("WiFi Connected!");
    }
    
    // 主循环
    while (1) {
        linx_delay_ms(1000);
    }
    
    return 0;
}
```

---

## 4. 核心API模块

### 4.1 网络通信

> **模块说明**: 提供WiFi连接、HTTP通信、文件传输等网络功能

#### 📡 WiFi 管理

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `wifi_connect(ssid, password)` | `const char* ssid, const char* password` | `int` | 连接WiFi网络 | 高 |
| `wifi_disconnect()` | `void` | `int` | 断开WiFi连接 | 高 |
| `wifi_get_status()` | `void` | `int` | 获取连接状态 (0=断开, 1=连接中, 2=已连接) | 高 |
| `wifi_get_rssi()` | `void` | `int` | 获取信号强度 (dBm) | 中 |
| `wifi_scan(results, max_count)` | `wifi_ap_t* results, int max_count` | `int` | 扫描可用网络 | 中 |

#### 🌐 HTTP 通信

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `http_get(url, response, max_len)` | `const char* url, char* response, int max_len` | `int` | HTTP GET请求 | 高 |
| `http_post(url, data, response, max_len)` | `const char* url, const char* data, char* response, int max_len` | `int` | HTTP POST请求 | 高 |

#### 🔌 WebSocket 通信

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `websocket_connect(url)` | `const char* url` | `int` | 连接WebSocket服务器 | 高 |
| `websocket_send(data, len)` | `const char* data, int len` | `int` | 发送数据 | 高 |
| `websocket_receive(buffer, max_len)` | `char* buffer, int max_len` | `int` | 接收数据 | 高 |
| `websocket_disconnect()` | `void` | `int` | 断开WebSocket连接 | 高 |
| `websocket_set_callback(callback)` | `void (*callback)(const char* data, int len)` | `int` | 设置消息回调函数 | 中 |

#### 📡 MQTT 通信

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `mqtt_connect(broker, port, client_id)` | `const char* broker, int port, const char* client_id` | `int` | 连接MQTT代理 | 高 |
| `mqtt_publish(topic, message, qos)` | `const char* topic, const char* message, int qos` | `int` | 发布消息 | 高 |
| `mqtt_subscribe(topic, callback)` | `const char* topic, void (*callback)(const char* topic, const char* message)` | `int` | 订阅主题 | 高 |
| `mqtt_unsubscribe(topic)` | `const char* topic` | `int` | 取消订阅 | 中 |
| `mqtt_disconnect()` | `void` | `int` | 断开MQTT连接 | 高 |


**使用示例**:
```c
// WiFi连接
if (wifi_connect("HomeWiFi", "mypassword") == LINX_OK) {
    printf("WiFi连接成功\n");
}

// HTTP请求
char response[1024];
if (http_get("http://api.weather.com/current", response, sizeof(response)) == LINX_OK) {
    printf("天气数据: %s\n", response);
}

// WebSocket通信
void on_message(const char* data, int len) {
    printf("收到WebSocket消息: %.*s\n", len, data);
}

if (websocket_connect("ws://localhost:8080/ws") == LINX_OK) {
    websocket_set_callback(on_message);
    websocket_send("Hello WebSocket", 15);
}

// MQTT通信
void on_mqtt_message(const char* topic, const char* message) {
    printf("MQTT消息 [%s]: %s\n", topic, message);
}

if (mqtt_connect("mqtt.broker.com", 1883, "linx_device_001") == LINX_OK) {
    mqtt_subscribe("sensors/temperature", on_mqtt_message);
    mqtt_publish("status/online", "true", 0);
}
```

### 4.2 音频系统

> **模块说明**: 提供音频播放、录制、音量控制等功能

#### 🔊 音频播放

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `audio_play(filename，opus, pcm)` | `const char* filename` | `int` | 播放音频文件 | 高 |
| `audio_stop()` | `void` | `int` | 停止播放 | 高 |
| `audio_pause()` | `void` | `int` | 暂停播放 | 中 |
| `audio_resume()` | `void` | `int` | 恢复播放 | 中 |

#### 🎤 音频录制

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `audio_record_start(filename)` | `const char* filename` | `int` | 开始录音 | 高 |
| `audio_record_stop()` | `void` | `int` | 停止录音 | 高 |
| `audio_record_duration(filename, seconds)` | `const char* filename, int seconds` | `int` | 定时录音 | 中 |

#### 🔈 音量控制

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `audio_set_volume(level)` | `int level` | `int` | 设置音量 (0-100) | 中 |
| `audio_get_volume()` | `void` | `int` | 获取当前音量 | 中 |
| `audio_mute(enable)` | `bool enable` | `int` | 静音控制 | 中 |

**使用示例**:
```c
// 播放欢迎音
audio_play("/sounds/welcome.wav");

// 录制语音备忘录
audio_record_duration("/memos/voice_memo.wav", 10);

// 调整音量
audio_set_volume(80);
```

### 4.3 摄像头

> **模块说明**: 提供拍照、录像、图像处理等功能

#### 📷 拍照功能

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `camera_init()` | `void` | `int` | 初始化摄像头 | 高 |
| `camera_capture(filename)` | `const char* filename` | `int` | 拍照保存 | 高 |
| `camera_capture_thumb(filename)` | `const char* filename` | `int` | 拍照缩略图 | 中 |
| `camera_capture_buffer(buffer, size)` | `uint8_t* buffer, int* size` | `int` | 拍照到内存 | 中 |
| `camera_deinit()` | `void` | `int` | 关闭摄像头 | 高 |

#### ⚙️ 参数设置

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `camera_set_quality(quality)` | `int quality` | `int` | 设置画质 (1-10) | 中 |
| `camera_set_resolution(width, height)` | `int width, int height` | `int` | 设置分辨率 | 中 |
| `camera_set_brightness(level)` | `int level` | `int` | 设置亮度 (-2 到 +2) | 低 |
| `camera_set_contrast(level)` | `int level` | `int` | 设置对比度 (-2 到 +2) | 低 |

**使用示例**:
```c
// 初始化并拍照
camera_init();
camera_set_quality(8);
camera_capture("/photos/snapshot.jpg");
camera_deinit();
```

### 4.4 显示系统

> **模块说明**: 提供文本显示、图像显示、图形绘制等功能

#### 📺 显示控制

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `display_init()` | `void` | `int` | 初始化显示 | 高 |
| `display_clear()` | `void` | `int` | 清屏 | 高 |
| `display_brightness(level)` | `int level` | `int` | 设置亮度 (0-100) | 中 |
| `display_sleep()` | `void` | `int` | 显示休眠 | 中 |
| `display_wake()` | `void` | `int` | 唤醒显示 | 中 |

#### 📝 文本显示

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `display_text(x, y, text)` | `int x, int y, const char* text` | `int` | 显示文本 | 高 |
| `display_text_center(text)` | `const char* text` | `int` | 居中显示文本 | 高 |
| `display_set_font_size(size)` | `int size` | `int` | 设置字体大小 | 中 |
| `display_set_text_color(color)` | `uint32_t color` | `int` | 设置文本颜色 | 中 |

#### 🖼️ 图像显示

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `display_image(x, y, filename)` | `int x, int y, const char* filename` | `int` | 显示图片 | 高 |
| `display_image_fit(filename)` | `const char* filename` | `int` | 图片适应屏幕 | 高 |
| `display_image_scale(filename, scale)` | `const char* filename, float scale` | `int` | 缩放显示图片 | 中 |

#### ✏️ 图形绘制

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `display_pixel(x, y, color)` | `int x, int y, uint32_t color` | `int` | 画点 | 低 |
| `display_line(x1, y1, x2, y2, color)` | `int x1, int y1, int x2, int y2, uint32_t color` | `int` | 画线 | 低 |
| `display_rect(x, y, w, h, color)` | `int x, int y, int w, int h, uint32_t color` | `int` | 画矩形 | 低 |
| `display_circle(x, y, radius, color)` | `int x, int y, int radius, uint32_t color` | `int` | 画圆 | 低 |

**使用示例**:
```c
// 显示欢迎界面
display_init();
display_clear();
display_text_center("欢迎使用 LinxOS");
display_image_fit("/images/logo.png");
```

### 4.5 任务管理

> **模块说明**: 提供闹钟、备忘录、定时器等任务调度功能

#### ⏰ 闹钟管理

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `alarm_set(hour, minute, message)` | `int hour, int minute, const char* message` | `int` | 设置一次性闹钟 | 高 |
| `alarm_set_repeat(hour, minute, weekdays)` | `int hour, int minute, int weekdays` | `int` | 设置重复闹钟 | 高 |
| `alarm_cancel(alarm_id)` | `int alarm_id` | `int` | 取消闹钟 | 高 |
| `alarm_list(alarms, max_count)` | `alarm_info_t* alarms, int max_count` | `int` | 列出所有闹钟 | 中 |
| `alarm_enable(alarm_id, enable)` | `int alarm_id, bool enable` | `int` | 启用/禁用闹钟 | 中 |

#### 📝 备忘录管理

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `memo_add(title, content)` | `const char* title, const char* content` | `int` | 添加备忘录 | 高 |
| `memo_delete(memo_id)` | `int memo_id` | `int` | 删除备忘录 | 高 |
| `memo_update(memo_id, title, content)` | `int memo_id, const char* title, const char* content` | `int` | 更新备忘录 | 中 |
| `memo_list(memos, max_count)` | `memo_info_t* memos, int max_count` | `int` | 列出所有备忘录 | 中 |
| `memo_search(keyword, results, max_count)` | `const char* keyword, memo_info_t* results, int max_count` | `int` | 搜索备忘录 | 中 |

#### ⏲️ 定时器管理

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `timer_set(seconds, callback)` | `int seconds, timer_callback_t callback` | `int` | 设置一次性定时器 | 高 |
| `timer_repeat(interval, callback)` | `int interval, timer_callback_t callback` | `int` | 设置重复定时器 | 中 |
| `timer_cancel(timer_id)` | `int timer_id` | `int` | 取消定时器 | 高 |
| `timer_pause(timer_id)` | `int timer_id` | `int` | 暂停定时器 | 中 |
| `timer_resume(timer_id)` | `int timer_id` | `int` | 恢复定时器 | 中 |

**使用示例**:
```c
// 设置每日闹钟
int alarm_id = alarm_set_repeat(7, 30, 0x7F); // 每天7:30

// 添加备忘录
memo_add("会议提醒", "下午3点开会");

// 设置5分钟定时器
timer_set(300, my_timer_callback);
```

### 4.6 系统工具

> **模块说明**: 提供系统状态查询等工具功能


#### 🔋 系统状态

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `system_get_battery()` | `void` | `int` | 获取电池电量 (0-100) | 中 |
| `system_get_memory()` | `void` | `int` | 获取可用内存 (KB) | 低 |
| `system_get_storage()` | `void` | `int` | 获取可用存储 (KB) | 低 |
| `system_reboot()` | `void` | `int` | 重启系统 | 低 |
| `system_sleep(seconds)` | `int seconds` | `int` | 系统休眠 | 中 |

**使用示例**:
```c
// 获取当前时间
time_info_t current_time;
time_get(&current_time);
printf("当前时间: %d-%02d-%02d %02d:%02d:%02d\n", 
       current_time.year, current_time.month, current_time.day,
       current_time.hour, current_time.minute, current_time.second);

// 检查存储空间
int free_space = system_get_storage();
if (free_space < 1024) {
    printf("存储空间不足，仅剩 %d KB\n", free_space);
}
```

### 4.7 系统信息

> **模块说明**: 提供设备向gRPC服务的核心能力接口，包括系统状态查询、系统调用执行、进程管理等底层系统功能

#### 🖥️ 硬件信息

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `sysinfo_get_hardware(info)` | `hardware_info_t* info` | `int` | 获取硬件信息 | 高 |
| `sysinfo_get_os(info)` | `os_info_t* info` | `int` | 获取操作系统信息 | 高 |
| `sysinfo_get_runtime(info)` | `runtime_info_t* info` | `int` | 获取运行时信息 | 高 |
| `sysinfo_get_network(info)` | `network_info_t* info` | `int` | 获取网络信息 | 高 |


#### 🛠️ 系统调用

| 函数 | 参数 | 返回值 | 描述 | 优先级 |
|------|------|--------|------|--------|
| `call_system(command, args, result)` | `const char* command, const char* args, syscall_result_t* result` | `int` | 执行系统调用 | 高 |



**使用示例**:
```c
// 获取系统硬件信息
hardware_info_t hw_info;
sysinfo_get_hardware(&hw_info);
printf("CPU: %s, 核心数: %d, 内存: %d KB\n", 
       hw_info.cpu_model, hw_info.cpu_cores, hw_info.memory_total);

// 执行系统命令
syscall_result_t result;
call_system("ls", "-la /tmp", &result);
printf("命令输出: %s\n", result.output);
```

---

## 5. 数据结构定义

### 📅 时间相关结构

```c
// 时间信息结构
typedef struct {
    int year;           // 年份 (2024-)
    int month;          // 月份 (1-12)
    int day;            // 日期 (1-31)
    int hour;           // 小时 (0-23)
    int minute;         // 分钟 (0-59)
    int second;         // 秒钟 (0-59)
    int weekday;        // 星期 (0=周日, 1=周一, ..., 6=周六)
    int timezone;       // 时区偏移 (小时)
} time_info_t;

// 闹钟信息结构
typedef struct {
    int id;             // 闹钟ID (唯一标识)
    int hour;           // 小时 (0-23)
    int minute;         // 分钟 (0-59)
    int weekdays;       // 重复日期位掩码 (bit0=周日, bit1=周一, ...)
    char message[64];   // 闹钟消息
    int enabled;        // 是否启用 (0=禁用, 1=启用)
    time_info_t created_time;  // 创建时间
} alarm_info_t;

// 备忘录信息结构
typedef struct {
    int id;             // 备忘录ID
    char title[32];     // 标题
    char content[256];  // 内容 (扩展到256字符)
    time_info_t created_time;   // 创建时间
    time_info_t modified_time;  // 修改时间
    int priority;       // 优先级 (1=低, 2=中, 3=高)
} memo_info_t;
```
