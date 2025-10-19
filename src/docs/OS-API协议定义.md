# LinxOS 分层架构 API 协议定义

> **版本**: v3.0  
> **更新日期**: 2025年10月  
> **芯片分层**: ESP32 (L1) | RK芯片 (L2) | 地平线 (L3) | NV芯片 (L4)  
> **应用场景**: AIoT设备 → 智能硬件 → 机器狗 → 人形机器人  

---


 ====== 设备能力 RPC ====
 device.lightOn

 ====== 设备能力 RPC 映射 服务端的API 【父集，扩展槽】 ==


 ===== 服务 ===

  {
  	"desc":"开灯",
  	"param":"true/false",
  	"code":“服务端的API”
  }

  

## 目录

- [1. 概述](#1-概述)
- [2. 芯片分层架构](#2-芯片分层架构)
- [3. 设计理念](#3-设计理念)
- [4. 快速开始](#4-快速开始)
- [5. 核心API模块](#5-核心api模块)
  - [5.1 网络通信](#51-网络通信)
  - [5.2 蓝牙配网](#52-蓝牙配网)
  - [5.3 音频系统](#53-音频系统)
  - [5.4 视觉模块](#54-视觉模块)
  - [5.5 显示系统](#55-显示系统)
  - [5.6 任务管理](#56-任务管理)
  - [5.7 系统工具](#57-系统工具)
  - [5.8 导航与移动](#58-导航与移动)
  - [5.9 姿势控制](#59-姿势控制)
  - [5.10 设备管理](#510-设备管理)
  - [5.11 事件系统](#511-事件系统)
  - [5.12 唤醒模块](#512-唤醒模块)

---

## 1. 概述

LinxOS 是专为不同算力芯片设计的**分层架构**操作系统API框架，从AIoT设备到人形机器人提供统一而灵活的接口体系。通过4层芯片适配策略，实现从基础物联网到高级机器人的全场景覆盖。

### 分层设计理念

- **L1-基础层 (ESP32)**: 专注AIoT设备的核心连接和基础交互功能
- **L2-标准层 (RK芯片)**: 面向智能硬件的多媒体和智能交互能力  
- **L3-增强层 (地平线)**: 支持机器狗等移动机器人的导航和运动控制
- **L4-专业层 (NV芯片)**: 为人形机器人提供高级AI和复杂决策能力

通过内置的对外服务器架构，LinxOS 提供双重通信能力：**WebSocket服务**用于实时控制与数据流传输，**gRPC服务**提供类似dcom的结构化远程调用能力。



---

## 2. 芯片分层架构

### 芯片能力对比

| 层级 | 芯片平台 | 内存规格 | 算力特点 | 主要应用 | API支持度 |
|------|----------|----------|----------|----------|-----------|
| **L1** | ESP32 | 512KB RAM<br>4MB Flash | 240MHz双核<br>基础AI推理 | 智能开关、传感器<br>简单语音交互 | 基础模块 |
| **L2** | RK芯片 | 1-4GB RAM<br>8-32GB存储 | ARM Cortex-A<br>NPU 1-6TOPS | 智能音箱、平板<br>智能显示屏 | 标准模块 |
| **L3** | 地平线 | 8-16GB RAM<br>64GB+存储 | 专用BPU<br>5-20TOPS AI | 四足机器人<br>自主导航 | 增强模块 |
| **L4** | NV芯片 | 32GB+ RAM<br>1TB+存储 | GPU+Tensor Core<br>100+TOPS | 人形机器人<br>复杂AI交互 | 完整模块 |


### 编译配置

通过预编译宏控制不同芯片层级的功能启用：

```c
// 芯片层级定义
#define LINX_CHIP_LEVEL_L1  1  // ESP32
#define LINX_CHIP_LEVEL_L2  2  // RK芯片  
#define LINX_CHIP_LEVEL_L3  3  // 地平线
#define LINX_CHIP_LEVEL_L4  4  // NV芯片

// 当前编译目标 (在编译时指定)
#ifndef LINX_CHIP_LEVEL
#define LINX_CHIP_LEVEL LINX_CHIP_LEVEL_L2  // 默认RK芯片
#endif
```

---




## 3. 设计理念

LinxOS 遵循**分层适配**、**模块化**、**轻量级**的设计原则，通过统一API接口适配不同算力芯片的能力差异。

---

## 4. 快速开始

### 架构设计

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


### Hello World - 完整的智能助手演示

> **演示场景**: 从设备配网到端到端音视频对话的完整LinxOS体验  
> **涵盖功能**: 蓝牙配网 → 唤醒检测 → 音视频对话 → 物体识别 → 智能交互

```c
#include "linx_api.h"

// 全局变量
static audio_handle_t* audio = NULL;

int main(void) {
    // 1. 系统初始化
    linx_init();
    printf("LinxOS Hello World 启动\n");
    
    // 2. 初始化音频系统
    audio = audio_init(16000, 1, AUDIO_FORMAT_PCM16);
    
    // 3. 初始化语音唤醒
    wakeup_init();
    wakeup_set_keywords("小灵", NULL);
    wakeup_set_callback(on_wakeup);
    
    // 4. 启动唤醒检测
    printf("请说'小灵'来唤醒设备\n");
    wakeup_start();
    
    // 5. 主循环
    while (1) {
        linx_delay_ms(100);
    }
    
    return 0;
}

// 唤醒回调函数
void on_wakeup(const char* keyword, float confidence) {
    printf("检测到唤醒词: %s (置信度: %.2f)\n", keyword, confidence);
    
    // 播放提示音
    audio_say("小灵已唤醒", NULL);
    
    // 开始语音识别
    printf("我在听，请说话...\n");
    audio_start(on_speech);
}

// 语音识别回调
void on_speech(const char* text, float confidence) {
    printf("识别结果: %s (置信度: %.2f)\n", text, confidence);
    
    if (confidence < 0.6) {
        audio_say("没听清楚，请再说一遍", NULL);
        audio_asr_start(on_speech);
        return;
    }
    
    // 简单的指令处理
    if (strstr(text, "你好")) {
        audio_say("你好，我是LinxOS智能助手", NULL);
    } else if (strstr(text, "时间")) {
        handle_time_query();
    } else {
        audio_say("我听到了，但还在学习中", NULL);
    }
    
    // 回到唤醒状态
    printf("回到待命状态，请说'小灵'唤醒\n");
    wakeup_start();
}

// 时间查询处理
void handle_time_query(void) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char time_str[32];
    
    snprintf(time_str, sizeof(time_str), "现在是%d点%d分", t->tm_hour, t->tm_min);
    audio_say(time_str, NULL);
}
```

**简化后的演示流程**:

1. **系统启动** → 初始化音频和语音唤醒
2. **语音唤醒** → 说"小灵"激活设备  
3. **语音交互** → 支持简单对话和时间查询
4. **循环待命** → 完成对话后自动回到唤醒状态

**支持的语音指令**:
- "小灵" → 唤醒设备
- "你好" → 简单问候
- "现在几点了" → 时间查询


---

## 5. 核心API模块

### 5.1 网络通信

> **模块说明**: 统一的网络通信接口，支持WiFi、HTTP、WebSocket等协议  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)

#### 核心API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `net_connect(config)` | `net_config_t* config` | `int` | 连接网络(WiFi/以太网)，自动配置 | L1+ |
| `net_request(method, url, data, response)` | `const char* method, const char* url, const char* data, char* response` | `int` | 统一HTTP请求(GET/POST/PUT/DELETE) | L1+ |
| `net_websocket(url, callback)` | `const char* url, ws_callback_t callback` | `int` | WebSocket连接，设置消息回调 | L1+ |
| `net_status()` | `void` | `net_status_t` | 获取网络状态和连接信息 | L1+ |


**使用示例**:
```c
// 连接WiFi
net_config_t config = {"HomeWiFi", "password", 1, 10000};
net_connect(&config);

// HTTP请求
char response[1024];
net_request("GET", "http://api.weather.com/current", NULL, response);
net_request("POST", "http://api.server.com/data", "{\"temp\":25}", response);

// WebSocket
void on_ws_message(const char* data, int len) {
    printf("收到: %.*s\n", len, data);
}
net_websocket("ws://localhost:8080/ws", on_ws_message);
```

### 5.3 音频系统

> **模块说明**: 统一的音频接口，支持播放、录制、语音合成、实时音频流处理  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础播放和音频流，L2+支持TTS，L3+支持语音识别

#### 音频流类型定义

```c
// 音频格式枚举
typedef enum {
    AUDIO_FORMAT_PCM_16BIT = 0,    // PCM 16位
    AUDIO_FORMAT_PCM_24BIT,        // PCM 24位
    AUDIO_FORMAT_PCM_32BIT,        // PCM 32位
    AUDIO_FORMAT_OPUS,             // Opus压缩格式
    AUDIO_FORMAT_AAC,              // AAC压缩格式
    AUDIO_FORMAT_MP3               // MP3压缩格式
} audio_format_t;

// 音频流回调函数类型
typedef void (*audio_callback_t)(const uint8_t* data, size_t length, audio_format_t format);
```

#### 核心API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `audio_init()` | `void` | `int` | 初始化音频系统 | L1+ |
| `audio_play(source)` | `const uint8* source, int type` | `int` | 播放音频(文件路径或URL或pcm，opus) | L1+ |
| `audio_record(filename, duration)` | `const char* filename, int duration` | `int` | 录音到设备文件，指定时长(秒) | L1+ |
| `audio_volume(level)` | `int level` | `int` | 设置音量(0-100)，-1获取当前音量 | L1+ |


**使用示例**:
```c
// 初始化音频
audio_init();

// 播放音频
audio_play("/sounds/welcome.wav");
audio_play("http://music.server.com/song.mp3");

// 录音10秒
audio_record("/recordings/voice.wav", 10);

// 语音合成
audio_say("欢迎使用LinxOS系统");

// 音量控制
audio_volume(80);  // 设置音量80%
int vol = audio_volume(-1);  // 获取当前音量

// 音频流处理回调函数
void audio_stream_callback(const uint8_t* data, size_t length, audio_format_t format) {
    // 处理实时音频数据
    printf("收到音频数据: %zu 字节, 格式: %d\n", length, format);
    
    // 可以在这里进行音频处理，如：
    // - 实时音频分析
    // - 语音识别
    // - 音频转发
    // - 音频效果处理等
}

// 开始音频流 (PCM 16位格式)
audio_start(audio_stream_callback, AUDIO_FORMAT_PCM_16BIT);

// 开始音频流 (Opus压缩格式)
audio_start(audio_stream_callback, AUDIO_FORMAT_OPUS);

// 停止音频流
audio_stop();
```

### 5.4 视觉模块 【思考驱动成】

> **模块说明**: 摄像头硬件控制和图像AI分析的统一接口  
> **芯片支持**: 🔴 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础拍照，L2+支持视频流，L3+支持AI识别

#### 摄像头硬件API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `camera_init(width, height, fps)` | `int width, int height, int fps` | `camera_handle_t*` | 初始化摄像头 | L1+ |
| `camera_capture(handle, callback)` | `camera_handle_t* handle, capture_callback_t callback` | `int` | 拍照捕获 | L1+ |


**数据结构**:
```c
typedef struct {
    void* data;
    int size;
    int width, height;
} image_data_t;

typedef struct {
    char name[32];
    float confidence;
    int x, y, w, h;
} detection_result_t;

// 回调函数
typedef void (*capture_callback_t)(int result, image_data_t* image);
typedef void (*stream_callback_t)(image_data_t* frame);
typedef void (*detect_callback_t)(detection_result_t* results, int count);
typedef void (*face_callback_t)(detection_result_t* faces, int count);
typedef void (*scene_callback_t)(const char* scene_type, int people_count);
```

**使用示例**:
```c
// 1. 基础拍照
camera_handle_t* camera = camera_init(640, 480, 15);

void on_photo_taken(int result, image_data_t* image) {
    if (result == 0) {
        // 保存照片
        save_image("/storage/photo.jpg", image);
        audio_say("拍照完成", NULL);
    }
}

camera_capture(camera, on_photo_taken);

// 2. 物体检测 (L3+)
void on_objects_found(detection_result_t* results, int count) {
    for (int i = 0; i < count; i++) {
        printf("发现 %s，置信度: %.2f\n", results[i].name, results[i].confidence);
        if (strcmp(results[i].name, "keys") == 0) {
            audio_say("找到钥匙了", NULL);
        }
    }
}

const char* targets[] = {"keys", "phone", "cup", NULL};
vision_detect_objects(image, targets, on_objects_found);

// 3. 实时AI分析 (L3+)
void on_ai_result(detection_result_t* results, int count) {
    // 处理AI分析结果
    for (int i = 0; i < count; i++) {
        printf("检测到: %s\n", results[i].name);
    }
}

vision_process_stream(camera, on_ai_result);
```


### 5.6 任务管理===》闹钟功能。CURD （ setEvent（eventtype， time， audio）  ） 通知功能（notify（eventtype））

> **模块说明**: 统一的任务管理接口，支持任务调度、闹钟、备忘录、定时器等功能  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1基础任务调度，L2+支持多进程，L3+支持优先级调度，L4支持智能资源分配



### 5.7 系统工具

> **模块说明**: 统一的系统工具接口，提供状态查询、系统调用、信息获取等功能  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1基础状态查询，L2+支持详细系统信息，L3+支持性能监控，L4支持智能诊断

#### 核心API (仅3个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|--------|
| `system_query(type, info)` | `int type, void* info` | `int` | 查询系统信息，电池/内存/存储/硬件/闹钟/备忘录 | L1+ |
| `system_control(cmd, value)` | `int cmd, int value` | `int` | 系统控制，重启/休眠/亮度 | L1+ |
| `system_call(command, args, result)` | `const char* command, const char* args, syscall_result_t* result` | `int` | 执行系统调用 | L1+ |

**数据结构**:
```c
// 查询类型
#define QUERY_BATTERY    0  // 电池电量
#define QUERY_MEMORY     1  // 内存信息
#define QUERY_STORAGE    2  // 存储信息
#define QUERY_HARDWARE   3  // 硬件信息
#define QUERY_NETWORK    4  // 网络信息

// 控制命令
#define CTRL_REBOOT      0  // 重启
#define CTRL_SLEEP       1  // 休眠
#define CTRL_BRIGHTNESS  2  // 亮度
```

**使用示例**:
```c
// 查询电池电量
int battery;
system_query(QUERY_BATTERY, &battery);
printf("电池电量: %d%%\n", battery);

// 查询硬件信息
hardware_info_t hw_info;
system_query(QUERY_HARDWARE, &hw_info);

// 系统控制
system_control(CTRL_SLEEP, 30);  // 休眠30秒
system_control(CTRL_REBOOT, 0);  // 重启

// 执行系统命令
syscall_result_t result;
system_call("ls", "-la /tmp", &result);
```


