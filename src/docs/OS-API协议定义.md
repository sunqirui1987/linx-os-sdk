# LinxOS 分层架构 API 协议定义

> **版本**: v3.0  
> **更新日期**: 2025年10月  
> **芯片分层**: ESP32 (L1) | RK芯片 (L2) | 地平线 (L3) | NV芯片 (L4)  
> **应用场景**: AIoT设备 → 智能硬件 → 机器狗 → 人形机器人  

---

##  目录

- [1. 概述](#1-概述)
- [2. 芯片分层架构](#2-芯片分层架构)
- [3. 设计理念](#3-设计理念)
- [4. 快速开始](#4-快速开始)
- [5. 核心API模块](#5-核心api模块)
  - [5.1 网络通信](#51-网络通信)
  - [5.2 蓝牙配网](#52-蓝牙配网)
  - [5.3 音频系统](#53-音频系统)
  - [5.4 摄像头](#54-摄像头)
  - [5.5 显示系统](#55-显示系统)
  - [5.6 任务管理](#56-任务管理)
  - [5.7 系统工具](#57-系统工具)
  - [5.8 系统信息](#58-系统信息)
  - [5.9 导航与移动](#59-导航与移动)
  - [5.10 姿势控制](#510-姿势控制)
  - [5.11 事件系统](#511-事件系统)
  - [5.12 唤醒模块](#512-唤醒模块)

---

## 1. 概述

LinxOS 是专为不同算力芯片设计的**分层架构**操作系统API框架，从AIoT设备到人形机器人提供统一而灵活的接口体系。通过4层芯片适配策略，实现从基础物联网到高级机器人的全场景覆盖。

### 🎯 分层设计理念

- **L1-基础层 (ESP32)**: 专注AIoT设备的核心连接和基础交互功能
- **L2-标准层 (RK芯片)**: 面向智能硬件的多媒体和智能交互能力  
- **L3-增强层 (地平线)**: 支持机器狗等移动机器人的导航和运动控制
- **L4-专业层 (NV芯片)**: 为人形机器人提供高级AI和复杂决策能力

通过内置的对外服务器架构，LinxOS 提供双重通信能力：**WebSocket服务**用于实时控制与数据流传输，**gRPC服务**提供类似dcom的结构化远程调用能力。



---

## 2. 芯片分层架构

### 📊 芯片能力对比

| 层级 | 芯片平台 | 内存规格 | 算力特点 | 主要应用 | API支持度 |
|------|----------|----------|----------|----------|-----------|
| **L1** | ESP32 | 512KB RAM<br>4MB Flash | 240MHz双核<br>基础AI推理 | 智能开关、传感器<br>简单语音交互 | 基础模块 |
| **L2** | RK芯片 | 1-4GB RAM<br>8-32GB存储 | ARM Cortex-A<br>NPU 1-6TOPS | 智能音箱、平板<br>智能显示屏 | 标准模块 |
| **L3** | 地平线 | 8-16GB RAM<br>64GB+存储 | 专用BPU<br>5-20TOPS AI | 四足机器人<br>自主导航 | 增强模块 |
| **L4** | NV芯片 | 32GB+ RAM<br>1TB+存储 | GPU+Tensor Core<br>100+TOPS | 人形机器人<br>复杂AI交互 | 完整模块 |


### ⚙️ 编译配置

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


### � Hello World - 完整的智能助手演示

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

#### 🚀 核心API

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

### 5.2 蓝牙配网

> **模块说明**: 简化的蓝牙配网接口，支持一键配网和设备管理  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: 所有芯片都支持基础蓝牙配网，L2+支持更快的配网速度

#### 📶 核心API (4个实用函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `bluetooth_init()` | `void` | `int` | 初始化蓝牙模块 | L1+ |
| `bluetooth_provision_set_callback(callback)` | `provision_callback_t callback` | `int` | 设置配网状态回调函数 | L1+ |
| `bluetooth_provision_start(device_name)` | `const char* device_name` | `int` | 启动配网模式，设置设备名 | L1+ |
| `bluetooth_provision_stop()` | `void` | `int` | 停止配网模式 | L1+ |


**使用示例**:
```c


// 完整配网流程
int start_bluetooth_provisioning(void) {
    // 1. 初始化蓝牙
    if (bluetooth_init() != LINX_OK) {
        printf("蓝牙初始化失败\n");
        return -1;
    }
    
    // 2. 设置配网回调
    bluetooth_provision_set_callback(on_provision_status);
    
    // 3. 启动配网模式
    if (bluetooth_provision_start("LinxOS-Assistant") != LINX_OK) {
        printf("启动配网失败\n");
        return -1;
    }
    
    printf("蓝牙配网已启动，设备名: LinxOS-Assistant\n");
    return 0;
}

// 停止配网
void stop_bluetooth_provisioning(void) {
    bluetooth_provision_stop();
    printf("蓝牙配网已停止\n");
}
```

### 5.3 音频系统

> **模块说明**: 统一的音频接口，支持播放、录制、语音合成、实时音频流处理  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础播放和音频流，L2+支持TTS，L3+支持语音识别

#### 📋 音频流类型定义

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

#### 🚀 核心API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `audio_init()` | `void` | `int` | 初始化音频系统 | L1+ |
| `audio_play(source)` | `const char* source` | `int` | 播放音频(文件路径或URL) | L1+ |
| `audio_record(filename, duration)` | `const char* filename, int duration` | `int` | 录音到文件，指定时长(秒) | L1+ |
| `audio_say(text)` | `const char* text` | `int` | 语音合成播放文本 | L2+ |
| `audio_volume(level)` | `int level` | `int` | 设置音量(0-100)，-1获取当前音量 | L1+ |
| `audio_start(callback, type)` | `audio_callback_t callback, audio_format_t type` | `int` | 开始音频流，支持实时音频数据回调 | L1+ |
| `audio_stop()` | `void` | `int` | 停止音频流 | L1+ |

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

### 5.4 视觉模块

> **模块说明**: 摄像头硬件控制和图像AI分析的统一接口  
> **芯片支持**: 🔴 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础拍照，L2+支持视频流，L3+支持AI识别

#### 📷 摄像头硬件API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `camera_init(width, height, fps)` | `int width, int height, int fps` | `camera_handle_t*` | 初始化摄像头 | L1+ |
| `camera_capture(handle, callback)` | `camera_handle_t* handle, capture_callback_t callback` | `int` | 拍照捕获 | L1+ |
| `camera_stream_start(handle, callback)` | `camera_handle_t* handle, stream_callback_t callback` | `int` | 开始视频流 | L2+ |
| `camera_control(handle, cmd, value)` | `camera_handle_t* handle, int cmd, int value` | `int` | 摄像头控制 | L1+ |

#### 🤖 图像AI分析API

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `vision_detect_objects(image, objects, callback)` | `image_data_t* image, const char** objects, detect_callback_t callback` | `int` | 物体检测 | L3+ |
| `vision_recognize_faces(image, face_db, callback)` | `image_data_t* image, face_db_t* face_db, face_callback_t callback` | `int` | 人脸识别 | L3+ |
| `vision_analyze_scene(image, callback)` | `image_data_t* image, scene_callback_t callback` | `int` | 场景分析 | L4+ |
| `vision_process_stream(camera, ai_callback)` | `camera_handle_t* camera, ai_callback_t ai_callback` | `int` | 实时AI分析 | L3+ |

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

### 5.5 显示系统

> **模块说明**: 统一的显示接口，支持文本、图像、图形、帧缓冲，自动适配屏幕能力  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础文本图形，L2+支持复杂UI，L3+支持动态界面，L4支持3D渲染

#### 📺 核心API (仅4个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|--------|
| `display_init(config)` | `display_config_t* config` | `int` | 初始化显示，自动检测屏幕 | L1+ |
| `display_show(content, params)` | `void* content, show_params_t* params` | `int` | 显示内容，支持文本/图像/帧 | L1+ |
| `display_draw(shapes, count)` | `shape_t* shapes, int count` | `int` | 绘制图形，支持点线矩形圆 | L1+ |
| `display_control(cmd, value)` | `int cmd, int value` | `int` | 控制显示，亮度/休眠/清屏 | L1+ |


**使用示例**:
```c
// 初始化
display_config_t config = {80, 1, 1};
display_init(&config);

// 显示文本
show_params_t text_params = {0, 0, 0, 0, 0, 0xFFFFFF, 16, true, 1.0};
display_show("欢迎使用 LinxOS", &text_params);

// 显示图像
show_params_t img_params = {1, 10, 10, 200, 150, 0, 0, false, 1.0};
display_show("/images/logo.png", &img_params);

// 显示摄像头帧
show_params_t frame_params = {2, 0, 0, 320, 240, 0, 0, false, 1.0};
display_show(camera_frame, &frame_params);

// 绘制图形
shape_t shapes[] = {
    {2, 10, 10, 110, 60, 0xFF0000, true},   // 红色矩形
    {3, 150, 150, 180, 180, 0x00FF00, false} // 绿色圆圈
};
display_draw(shapes, 2);

// 控制显示
display_control(DISPLAY_BRIGHTNESS, 50);  // 设置亮度50%
display_control(DISPLAY_CLEAR, 0);        // 清屏
```

### 5.6 任务管理

> **模块说明**: 统一的任务管理接口，支持任务调度、闹钟、备忘录、定时器等功能  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1基础任务调度，L2+支持多进程，L3+支持优先级调度，L4支持智能资源分配

#### ⚙️ 核心API (仅4个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `task_create(name, function, params)` | `const char* name, task_function_t function, task_params_t* params` | `task_handle_t` | 创建任务，支持多种类型 | L1+ |
| `task_control(handle, cmd, value)` | `task_handle_t handle, int cmd, void* value` | `int` | 控制任务，暂停/恢复/删除 | L1+ |
| `task_schedule(type, time_spec, callback)` | `int type, const char* time_spec, schedule_callback_t callback` | `int` | 调度任务，闹钟/定时器/备忘录 | L1+ |
| `task_query(type, filter, results, max_count)` | `int type, const char* filter, void* results, int max_count` | `int` | 查询任务，列表/状态/搜索 | L1+ |


**使用示例**:
```c
// 创建任务
task_params_t params = {5, 4096, 0, true};
task_handle_t task = task_create("my_task", my_function, &params);

// 控制任务
task_control(task, TASK_SUSPEND, NULL);  // 暂停
task_control(task, TASK_RESUME, NULL);   // 恢复

// 设置闹钟
void alarm_callback(int type, const char* data) {
    audio_say("该起床了", NULL);
}
task_schedule(SCHEDULE_ALARM, "07:30:00 daily", alarm_callback);

// 设置定时器
task_schedule(SCHEDULE_TIMER, "300s once", timer_callback);

// 添加备忘录
task_schedule(SCHEDULE_MEMO, "会议提醒|下午3点开会", NULL);

// 查询任务列表
task_info_t tasks[10];
int count = task_query(QUERY_TASKS, NULL, tasks, 10);

// 搜索备忘录
memo_info_t memos[5];
task_query(QUERY_MEMOS, "会议", memos, 5);
```

### 5.7 系统工具

> **模块说明**: 统一的系统工具接口，提供状态查询、系统调用、信息获取等功能  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1基础状态查询，L2+支持详细系统信息，L3+支持性能监控，L4支持智能诊断

#### 🔋 核心API (仅3个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|--------|
| `system_query(type, info)` | `int type, void* info` | `int` | 查询系统信息，电池/内存/存储/硬件 | L1+ |
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

### 5.8 导航与移动

> **模块说明**: 统一的导航移动接口，支持定位、路径规划、移动控制，适用于具身智能硬件  
> **芯片支持**: 🔴 L1 (ESP32) | 🔴 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L3支持基础导航移动，L4支持高级路径规划和智能避障

#### 🗺️ 核心API (仅4个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `nav_goto(target, params)` | `const char* target, nav_params_t* params` | `int` | 导航到目标，支持多种模式 | L3+ |
| `nav_control(cmd, value)` | `int cmd, void* value` | `int` | 移动控制，速度/停止/转向 | L3+ |



**使用示例**:
```c
// 导航到客厅
nav_params_t params = {500, 0.5, true, 30};
nav_goto("LivingRoom", &params);

// 速度控制
velocity_t vel = {0.5, 0.0, 0.0};  // 前进
nav_control(NAV_VELOCITY, &vel);


```

### 5.9 姿势控制

> **模块说明**: 统一的姿势控制接口，支持动作执行、姿势管理，适用于具身智能硬件  
> **芯片支持**: 🔴 L1 (ESP32) | 🔴 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L3支持基础动作控制，L4支持复杂姿势规划和动态平衡

#### 🤸 核心API 

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `pose_execute(action, params)` | `const char* action, action_params_t* params` | `int` | 执行动作，支持参数控制 | L3+ |
| `pose_control(cmd, value)` | `int cmd, void* value` | `int` | 控制动作，停止/暂停/恢复 | L3+ |
| `pose_query(type, filter, results, max_count)` | `int type, const char* filter, void* results, int max_count` | `int` | 查询动作列表/状态/可用性 | L3+ |

**数据结构**:
```c
typedef struct {
    float speed;           // 动作速度
    int repeat;           // 重复次数
    bool blocking;        // 是否阻塞等待
    int timeout;          // 超时时间(秒)
} action_params_t;

// 控制命令
#define POSE_STOP        0  // 停止动作
#define POSE_PAUSE       1  // 暂停动作
#define POSE_RESUME      2  // 恢复动作

// 查询类型
#define QUERY_ACTIONS    0  // 动作列表
#define QUERY_STATUS     1  // 当前状态
#define QUERY_AVAILABLE  2  // 可用性检查
```

**使用示例**:
```c
// 执行动作
action_params_t params = {1.5, 1, false, 10};
pose_execute("wave_hand", &params);

// 控制动作
pose_control(POSE_STOP, NULL);

// 查询可用动作
action_info_t actions[10];
int count = pose_query(QUERY_ACTIONS, NULL, actions, 10);

// 检查动作可用性
bool available;
pose_query(QUERY_AVAILABLE, "custom_greeting", &available, 1);
```

### 5.10 设备管理

> **模块说明**: 外接设备的统一管理接口，支持USB摄像头、蓝牙设备、传感器等的即插即用  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础USB设备，L2+支持热插拔检测，L3+支持智能设备识别，L4支持设备AI管理

#### 🔌 核心API (5个实用函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|--------|
| `device_connect(type, name, config)` | `int type, const char* name, device_config_t* config` | `device_handle_t*` | 连接设备，返回设备句柄 | L1+ |
| `device_disconnect(handle)` | `device_handle_t* handle` | `int` | 断开设备连接，释放资源 | L1+ |
| `device_scan(type, callback)` | `int type, device_found_callback_t callback` | `int` | 扫描设备，异步回调通知 | L2+ |
| `device_control(handle, cmd, params)` | `device_handle_t* handle, int cmd, void* params` | `int` | 设备控制，参数/模式/功能切换 | L1+ |
| `device_get_info(handle, info_type)` | `device_handle_t* handle, int info_type` | `void*` | 获取设备信息，状态/能力/属性 | L1+ |


**使用示例**:
```c

// 3. 连接舵机 (L2+)
device_config_t servo_config = {
    .port = 18,             // GPIO18
    .baudrate = 50,         // 50Hz PWM
    .driver_name = "pwm"
};
device_handle_t* servo = device_connect(DEVICE_SERVO, "head_servo", &servo_config);
if (servo) {
    // 控制舵机转动
    int angle = 90;  // 90度
    device_control(servo, CTRL_SET_PARAM, &angle);
}
device_disconnect(servo);
```


### 5.11 事件系统

> **模块说明**: 统一的事件系统接口，支持语音、场景、时间、系统等多种事件类型  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持基础事件，L2+支持复杂事件，L3+支持智能事件，L4支持AI事件分析

#### 🎤 核心API (仅4个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|--------|
| `event_listen(type, config, callback)` | `int type, event_config_t* config, event_callback_t callback` | `int` | 监听事件，语音/触摸/手势/人脸 | L1+ |
| `event_trigger(type, name, data)` | `int type, const char* name, void* data` | `int` | 触发事件，场景/系统/自定义 | L1+ |
| `event_schedule(type, time_expr, callback)` | `int type, const char* time_expr, schedule_callback_t callback` | `int` | 定时事件，定时器/闹钟/定时任务 | L1+ |
| `event_manage(cmd, name, config)` | `int cmd, const char* name, void* config` | `int` | 事件管理，注册/注销/配置 | L1+ |

**使用示例**:
```c
// 语音事件监听
const char* wake_words[] = {"小助手", "你好", NULL};
event_config_t voice_config = {wake_words, 80, 5000, NULL};
event_listen(EVENT_VOICE, &voice_config, on_voice_detected);

// 物体检测事件 (L3+)
const char* objects[] = {"keys", "phone", NULL};
event_config_t object_config = {objects, 70, 0, NULL};
event_listen(EVENT_OBJECT, &object_config, on_object_found);

// 定时任务
event_schedule(SCHEDULE_CRON, "0 8 * * *", on_morning_alarm);

// 场景事件触发
scene_data_t scene_data = {"living_room", true, "18:00-22:00"};
event_trigger(EVENT_SCENE, "evening_mode", &scene_data);

// 统一事件回调
void on_voice_detected(int type, const char* data, float confidence) {
    if (type == EVENT_VOICE) {
        audio_say("我在这里，有什么需要帮助的吗？", NULL);
        display_play_expression("happy", NULL);
    }
}
```

### 5.12 唤醒模块

> **模块说明**: 统一的智能唤醒接口，支持语音、红外、人脸、触摸等多种方式，自动适配不同芯片能力  
> **芯片支持**: 🟢 L1 (ESP32) | 🟢 L2 (RK芯片) | 🟢 L3 (地平线) | 🟢 L4 (NV芯片)  
> **功能差异**: L1支持语音+触摸，L2+支持红外检测，L3+支持人脸识别，L4支持多模态融合

#### 🚀 核心API (仅5个函数)

| 函数 | 参数 | 返回值 | 描述 | 芯片级别 |
|------|------|--------|------|----------|
| `wakeup_init(config)` | `wakeup_config_t* config` | `int` | 初始化唤醒系统，自动检测芯片能力 | L1+ |
| `wakeup_enable(types, callback)` | `int types, wakeup_callback_t callback` | `int` | 启用指定唤醒方式，设置统一回调 | L1+ |
| `wakeup_config(type, params)` | `wakeup_type_t type, void* params` | `int` | 配置特定唤醒方式的参数 | L1+ |
| `wakeup_sleep(timeout)` | `int timeout` | `int` | 进入低功耗模式，超时自动唤醒 | L1+ |
| `wakeup_status()` | `void` | `wakeup_status_t` | 获取唤醒状态和统计信息 | L1+ |

**使用示例**:
```c
// 统一唤醒回调
void on_wakeup(int type, const char* data, float confidence) {
    switch (type) {
        case WAKEUP_VOICE:
            printf("语音唤醒: %s (%.2f)\n", data, confidence);
            audio_say("我在这里", NULL);
            break;
        case WAKEUP_FACE:
            printf("人脸识别: %s (%.2f)\n", data, confidence);
            audio_say("欢迎回来", NULL);
            break;
        case WAKEUP_IR:
            printf("红外检测: 距离%s米\n", data);
            audio_say("检测到接近", NULL);
            break;
        case WAKEUP_TOUCH:
            printf("触摸唤醒\n");
            audio_say("我被唤醒了", NULL);
            break;
    }
}

// 简单初始化
wakeup_config_t config = {
    .auto_detect_chip = 1,    // 自动检测芯片能力
    .default_timeout = 30,    // 30秒自动休眠
    .power_mode = 1          // 节能模式
};

wakeup_init(&config);

// 启用所有可用的唤醒方式
wakeup_enable(WAKEUP_ALL, on_wakeup);

// 配置语音唤醒词
char* keywords[] = {"小助手", "LinxOS", NULL};
wakeup_config(WAKEUP_VOICE, keywords);

// 配置人脸识别 (仅L3+可用)
char* authorized_users[] = {"主人", "家人", NULL};
wakeup_config(WAKEUP_FACE, authorized_users);

// 进入休眠，等待唤醒
wakeup_sleep(0);  // 0=无限等待

// 主循环
while (1) {
    wakeup_status_t status = wakeup_status();
    if (!status.is_sleeping) {
        // 处理用户交互
        linx_delay_ms(1000);
    }
}
```
```

---
