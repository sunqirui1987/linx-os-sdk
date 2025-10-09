# LinX OS SDK 跨平台Board设计方案

## 1. 概述

本设计方案旨在实现一个跨平台的Board抽象层，使得LinX OS SDK能够在不同的硬件平台（ESP32、Mac等）上运行，同时保持统一的接口和功能。

## 2. 设计目标

- **跨平台兼容性**: 支持ESP32、Mac等不同平台
- **统一接口**: 提供一致的C99接口供上层应用使用
- **模块化设计**: 各功能模块独立，便于扩展和维护
- **向后兼容**: 与现有的bak目录下的实现保持功能一致
- **C++实现**: 平台特定代码使用C++实现，提供更好的面向对象支持

## 3. 架构设计

### 3.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│                    C99 SDK Interface                        │
│                   (src/common/*.h)                          │
├─────────────────────────────────────────────────────────────┤
│                  Platform Abstraction                       │
│                   (src/board/*.c)                           │
├─────────────────────────────────────────────────────────────┤
│              Platform-Specific Implementation               │
│         (board/esp32/common/*.cpp, board/mac/common/*.cpp)  │
├─────────────────────────────────────────────────────────────┤
│                    Hardware Layer                           │
│              (ESP32 IDF, macOS APIs, etc.)                  │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 目录结构

```
linx-os-sdk/
├── src/                           # C99 SDK核心代码
│   ├── board/                     # Board抽象层
│   │   ├── board.h               # Board接口定义
│   │   ├── board.c               # Board基础实现
│   │   └── test/                 # 测试代码
│   ├── common/                    # 通用C99接口
│   │   ├── audio/                # 音频接口
│   │   ├── display/              # 显示接口
│   │   ├── camera/               # 摄像头接口
│   │   ├── network/              # 网络接口
│   │   └── gpio/                 # GPIO接口
│   └── ...
├── board/                         # 平台特定实现
│   ├── esp32/                    # ESP32平台
│   │   ├── common/               # ESP32通用实现
│   │   │   ├── esp32_board.cpp   # ESP32 Board基类
│   │   │   ├── esp32_audio.cpp   # ESP32音频实现
│   │   │   ├── esp32_display.cpp # ESP32显示实现
│   │   │   ├── esp32_camera.cpp  # ESP32摄像头实现
│   │   │   ├── esp32_wifi.cpp    # ESP32 WiFi实现
│   │   │   └── esp32_gpio.cpp    # ESP32 GPIO实现
│   │   └── board/                # 具体板卡实现
│   │       └── bread-compact-wifi/
│   │           ├── compact_wifi_board.cpp
│   │           ├── config.h
│   │           └── config.json
│   └── mac/                      # macOS平台
│       └── common/               # macOS通用实现
│           ├── mac_board.cpp     # macOS Board基类
│           ├── mac_audio.cpp     # macOS音频实现
│           └── ...
└── design/                       # 设计文档
    └── cross_platform_board_design.md
```

## 4. 核心接口设计

### 4.1 Board接口 (src/board/board.h)

已有的C99接口保持不变，提供以下核心功能：

```c
// Board管理
Board* board_get_instance(void);
const char* board_get_board_type(Board* self);
const char* board_get_uuid(Board* self);

// 硬件组件访问
void* board_get_audio_codec(Board* self);
void* board_get_display(Board* self);
void* board_get_camera(Board* self);
void* board_get_led(Board* self);
void* board_get_network(Board* self);

// 系统功能
bool board_get_temperature(Board* self, float* temperature);
bool board_get_battery_level(Board* self, int* level, bool* charging, bool* discharging);
void board_set_power_save_mode(Board* self, bool enabled);
```

### 4.2 音频接口 (src/common/audio/audio_interface.h)

```c
typedef struct AudioInterface AudioInterface;

typedef struct {
    int (*init)(AudioInterface* self);
    int (*start_input)(AudioInterface* self);
    int (*stop_input)(AudioInterface* self);
    int (*start_output)(AudioInterface* self);
    int (*stop_output)(AudioInterface* self);
    int (*set_input_volume)(AudioInterface* self, int volume);
    int (*set_output_volume)(AudioInterface* self, int volume);
    int (*get_input_volume)(AudioInterface* self);
    int (*get_output_volume)(AudioInterface* self);
    void (*destroy)(AudioInterface* self);
} AudioInterfaceVTable;

struct AudioInterface {
    const AudioInterfaceVTable* vtable;
    void* data;
};
```

### 4.3 显示接口 (src/display/display.h)

已有接口保持不变，支持：
- 状态显示
- 通知显示
- 主题设置
- 电源管理

### 4.4 网络接口 (src/common/network/network_interface.h)

```c
typedef struct NetworkInterface NetworkInterface;

typedef struct {
    int (*init)(NetworkInterface* self);
    int (*connect)(NetworkInterface* self, const char* ssid, const char* password);
    int (*disconnect)(NetworkInterface* self);
    bool (*is_connected)(NetworkInterface* self);
    const char* (*get_ip_address)(NetworkInterface* self);
    const char* (*get_mac_address)(NetworkInterface* self);
    int (*get_signal_strength)(NetworkInterface* self);
    void (*destroy)(NetworkInterface* self);
} NetworkInterfaceVTable;

struct NetworkInterface {
    const NetworkInterfaceVTable* vtable;
    void* data;
};
```

## 5. 平台特定实现

### 5.1 ESP32平台实现

#### 5.1.1 ESP32 Board基类 (board/esp32/common/esp32_board.cpp)

```cpp
class ESP32Board : public BoardInterface {
protected:
    std::string uuid_;
    AudioInterface* audio_codec_;
    DisplayInterface* display_;
    NetworkInterface* network_;
    
public:
    ESP32Board();
    virtual ~ESP32Board();
    
    // 实现Board接口
    virtual const char* GetBoardType() override;
    virtual const char* GetUuid() override;
    virtual AudioInterface* GetAudioCodec() override;
    virtual DisplayInterface* GetDisplay() override;
    virtual NetworkInterface* GetNetwork() override;
    
    // ESP32特定功能
    virtual void InitializeGPIO() = 0;
    virtual void InitializeI2C() = 0;
    virtual void InitializeSPI() = 0;
};
```

#### 5.1.2 Compact WiFi Board实现 (board/esp32/board/bread-compact-wifi/compact_wifi_board.cpp)

```cpp
class CompactWifiBoard : public ESP32Board {
private:
    // 硬件组件
    i2c_master_bus_handle_t display_i2c_bus_;
    esp_lcd_panel_handle_t panel_;
    
    // 按钮管理
    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;
    
    void InitializeDisplayI2c();
    void InitializeSsd1306Display();
    void InitializeButtons();
    void InitializeAudio();
    
public:
    CompactWifiBoard();
    virtual ~CompactWifiBoard();
    
    // 重写基类方法
    virtual void InitializeGPIO() override;
    virtual void InitializeI2C() override;
    virtual void InitializeSPI() override;
    
    // 获取硬件组件
    virtual AudioInterface* GetAudioCodec() override;
    virtual DisplayInterface* GetDisplay() override;
    virtual LedInterface* GetLed() override;
};
```

### 5.2 macOS平台实现

#### 5.2.1 macOS Board基类 (board/mac/common/mac_board.cpp)

```cpp
class MacBoard : public BoardInterface {
protected:
    std::string uuid_;
    AudioInterface* audio_codec_;
    DisplayInterface* display_;
    NetworkInterface* network_;
    
public:
    MacBoard();
    virtual ~MacBoard();
    
    // 实现Board接口
    virtual const char* GetBoardType() override;
    virtual const char* GetUuid() override;
    virtual AudioInterface* GetAudioCodec() override;
    virtual DisplayInterface* GetDisplay() override;
    virtual NetworkInterface* GetNetwork() override;
    
    // macOS特定功能
    virtual void InitializeAudio();
    virtual void InitializeDisplay();
    virtual void InitializeNetwork();
};
```

## 6. C99接口桥接

### 6.1 Board桥接实现 (src/board/board.c)

```c
// 全局Board实例
static Board* g_board_instance = NULL;

Board* board_get_instance(void) {
    if (g_board_instance == NULL) {
        g_board_instance = create_board(); // 平台特定的创建函数
    }
    return g_board_instance;
}

const char* board_get_board_type(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_board_type) {
        return "unknown";
    }
    return self->vtable->get_board_type(self);
}

void* board_get_audio_codec(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_audio_codec) {
        return NULL;
    }
    return self->vtable->get_audio_codec(self);
}
```

### 6.2 平台特定的create_board实现

#### ESP32平台 (board/esp32/board/bread-compact-wifi/compact_wifi_board.cpp)

```cpp
extern "C" {
    Board* create_board(void) {
        static CompactWifiBoard* board = nullptr;
        if (board == nullptr) {
            board = new CompactWifiBoard();
        }
        return board->GetCInterface();
    }
}
```

## 7. 配置管理

### 7.1 配置文件结构

每个板卡都有自己的配置文件：

```json
{
    "target": "esp32s3",
    "board_type": "bread-compact-wifi",
    "version": "1.0.0",
    "features": {
        "audio": {
            "input_sample_rate": 16000,
            "output_sample_rate": 24000,
            "method": "simplex"
        },
        "display": {
            "type": "ssd1306",
            "width": 128,
            "height": 32,
            "i2c_address": "0x3C"
        },
        "gpio": {
            "builtin_led": 48,
            "boot_button": 0,
            "touch_button": 47,
            "volume_up": 40,
            "volume_down": 39
        }
    },
    "builds": [
        {
            "name": "bread-compact-wifi",
            "sdkconfig_append": [
                "CONFIG_OLED_SSD1306_128X32=y"
            ]
        }
    ]
}
```

### 7.2 配置头文件生成

基于JSON配置自动生成config.h文件：

```c
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio Configuration
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_I2S_METHOD_SIMPLEX

// GPIO Configuration
#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_47
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

// Display Configuration
#define DISPLAY_SDA_PIN GPIO_NUM_41
#define DISPLAY_SCL_PIN GPIO_NUM_42
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  32

#endif // _BOARD_CONFIG_H_
```

## 8. 构建系统集成

### 8.1 CMake配置

```cmake
# board/esp32/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 通用ESP32源文件
set(ESP32_COMMON_SOURCES
    common/esp32_board.cpp
    common/esp32_audio.cpp
    common/esp32_display.cpp
    common/esp32_camera.cpp
    common/esp32_wifi.cpp
    common/esp32_gpio.cpp
)

# 创建ESP32通用库
add_library(esp32_common STATIC ${ESP32_COMMON_SOURCES})

# 包含目录
target_include_directories(esp32_common PUBLIC
    common
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/board
    ${CMAKE_SOURCE_DIR}/src/common
)

# 链接ESP-IDF组件
target_link_libraries(esp32_common
    idf::driver
    idf::esp_lcd
    idf::esp_wifi
    idf::nvs_flash
)

# 添加板卡特定实现
add_subdirectory(board/bread-compact-wifi)
```

### 8.2 板卡特定构建

```cmake
# board/esp32/board/bread-compact-wifi/CMakeLists.txt
set(BOARD_SOURCES
    compact_wifi_board.cpp
)

add_library(bread_compact_wifi STATIC ${BOARD_SOURCES})

target_include_directories(bread_compact_wifi PUBLIC
    .
    ${CMAKE_SOURCE_DIR}/board/esp32/common
)

target_link_libraries(bread_compact_wifi
    esp32_common
    linx_sdk
)
```

## 9. 测试策略

### 9.1 单元测试

为每个模块提供单元测试：

```c
// src/board/test/board_test.c
#include "board.h"
#include <assert.h>
#include <stdio.h>

void test_board_creation() {
    Board* board = board_get_instance();
    assert(board != NULL);
    
    const char* board_type = board_get_board_type(board);
    assert(board_type != NULL);
    
    printf("Board type: %s\n", board_type);
}

void test_audio_interface() {
    Board* board = board_get_instance();
    void* audio = board_get_audio_codec(board);
    assert(audio != NULL);
    
    // 测试音频接口功能
}

int main() {
    test_board_creation();
    test_audio_interface();
    printf("All tests passed!\n");
    return 0;
}
```

### 9.2 集成测试

提供完整的功能测试：

```c
// examples/esp32/board_demo.c
#include "board.h"
#include "audio/audio_interface.h"
#include "display/display.h"

int main() {
    // 初始化Board
    Board* board = board_get_instance();
    
    // 测试显示功能
    DisplayInterface* display = board_get_display(board);
    if (display) {
        display->vtable->set_status(display, "System Ready");
    }
    
    // 测试音频功能
    AudioInterface* audio = board_get_audio_codec(board);
    if (audio) {
        audio->vtable->init(audio);
        audio->vtable->set_output_volume(audio, 50);
    }
    
    return 0;
}
```

## 10. 迁移计划

### 10.1 阶段一：基础框架搭建
1. 创建目录结构
2. 实现C99接口定义
3. 实现ESP32基础Board类
4. 迁移CompactWifiBoard基本功能

### 10.2 阶段二：功能模块迁移
1. 迁移音频模块
2. 迁移显示模块
3. 迁移网络模块
4. 迁移GPIO和按钮管理

### 10.3 阶段三：完善和优化
1. 添加macOS平台支持
2. 完善测试用例
3. 优化性能
4. 完善文档

### 10.4 阶段四：扩展支持
1. 添加更多板卡支持
2. 添加更多平台支持
3. 添加高级功能

## 11. 总结

本设计方案提供了一个完整的跨平台Board抽象层架构，具有以下优势：

1. **统一接口**: 通过C99接口提供统一的API
2. **平台隔离**: 平台特定代码与通用代码分离
3. **易于扩展**: 模块化设计便于添加新平台和新功能
4. **向后兼容**: 保持与现有实现的功能一致性
5. **类型安全**: C++实现提供更好的类型安全和面向对象支持

通过这个设计，LinX OS SDK将能够在多个平台上提供一致的开发体验，同时保持高度的灵活性和可扩展性。