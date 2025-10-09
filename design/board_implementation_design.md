# LinX OS SDK - Board模块实现设计方案

## 概述

本文档详细描述了LinX OS SDK中Board模块的实现设计，包括跨平台抽象层的设计、具体平台的实现策略以及与现有bak目录功能的兼容性方案。

## 设计目标

### 1. 统一抽象
- 提供统一的C99板级抽象接口
- 隐藏平台特定的实现细节
- 支持多种硬件平台的无缝切换

### 2. 功能完整性
- 完全复制bak目录中的现有功能
- 保持API的向后兼容性
- 支持所有硬件外设的访问

### 3. 可扩展性
- 易于添加新的硬件平台
- 支持硬件配置的动态检测
- 模块化的外设接口设计

## 目录结构设计

```
board/
├── esp32/                          # ESP32平台实现
│   ├── common/                     # ESP32通用实现
│   │   ├── esp32_board.cpp         # ESP32基础板类
│   │   ├── esp32_board.h           # ESP32板接口定义
│   │   ├── esp32_audio.cpp         # ESP32音频实现
│   │   ├── esp32_audio.h
│   │   ├── esp32_display.cpp       # ESP32显示实现
│   │   ├── esp32_display.h
│   │   ├── esp32_network.cpp       # ESP32网络实现
│   │   ├── esp32_network.h
│   │   ├── esp32_gpio.cpp          # ESP32 GPIO实现
│   │   ├── esp32_gpio.h
│   │   ├── esp32_camera.cpp        # ESP32摄像头实现
│   │   ├── esp32_camera.h
│   │   └── CMakeLists.txt
│   └── board/                      # 具体板型实现
│       └── bread-compact-wifi/     # 面包板WiFi版
│           ├── compact_wifi_board.cpp
│           ├── compact_wifi_board.h
│           ├── config.json         # 板级配置
│           ├── config.h            # 自动生成的配置头文件
│           └── CMakeLists.txt
├── mac/                            # macOS平台实现
│   ├── common/                     # macOS通用实现
│   │   ├── mac_board.cpp
│   │   ├── mac_board.h
│   │   ├── mac_audio.cpp
│   │   ├── mac_audio.h
│   │   ├── mac_display.cpp
│   │   ├── mac_display.h
│   │   ├── mac_network.cpp
│   │   ├── mac_network.h
│   │   └── CMakeLists.txt
│   └── board/
│       └── mac-dev/                # macOS开发板
│           ├── mac_dev_board.cpp
│           ├── mac_dev_board.h
│           ├── config.json
│           └── CMakeLists.txt
└── CMakeLists.txt                  # 主构建配置
```

## 核心接口设计

### 1. Board基础接口

#### C99接口定义 (`src/board/board.h`)
```c
#ifndef BOARD_H
#define BOARD_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct Board Board;
typedef struct AudioInterface AudioInterface;
typedef struct DisplayInterface DisplayInterface;
typedef struct NetworkInterface NetworkInterface;
typedef struct CameraInterface CameraInterface;
typedef struct GPIOInterface GPIOInterface;

// Board虚函数表
typedef struct {
    // 生命周期管理
    int (*init)(Board* self);
    void (*destroy)(Board* self);
    
    // 基本信息
    const char* (*get_board_type)(Board* self);
    const char* (*get_uuid)(Board* self);
    
    // 硬件接口获取
    AudioInterface* (*get_audio_codec)(Board* self);
    DisplayInterface* (*get_display)(Board* self);
    NetworkInterface* (*get_network)(Board* self);
    CameraInterface* (*get_camera)(Board* self);
    GPIOInterface* (*get_gpio)(Board* self);
    void* (*get_led)(Board* self);
    
    // 系统功能
    float (*get_temperature)(Board* self);
    int (*get_battery_level)(Board* self);
    void (*set_power_save_mode)(Board* self, bool enabled);
    
    // 配置管理
    const char* (*get_board_json)(Board* self);
    const char* (*get_device_status_json)(Board* self);
    const char* (*get_system_info_json)(Board* self);
    
    // 网络特定功能（WiFi板）
    int (*enter_wifi_config_mode)(Board* self);
    int (*start_network)(Board* self);
    int (*reset_wifi_config)(Board* self);
} BoardVTable;

// Board结构体
struct Board {
    const BoardVTable* vtable;
    void* impl_data;
    
    // 基本信息
    char* board_type;
    char* uuid;
    bool is_initialized;
    
    // 硬件接口实例
    AudioInterface* audio_interface;
    DisplayInterface* display_interface;
    NetworkInterface* network_interface;
    CameraInterface* camera_interface;
    GPIOInterface* gpio_interface;
    void* led_interface;
};

// 工厂函数
Board* board_create(void);
void board_destroy(Board* board);
Board* board_get_instance(void);

// 便利函数
int board_init(Board* self);
const char* board_get_board_type(Board* self);
const char* board_get_uuid(Board* self);
AudioInterface* board_get_audio_codec(Board* self);
DisplayInterface* board_get_display(Board* self);
NetworkInterface* board_get_network(Board* self);
CameraInterface* board_get_camera(Board* self);
GPIOInterface* board_get_gpio(Board* self);
void* board_get_led(Board* self);
float board_get_temperature(Board* self);
int board_get_battery_level(Board* self);
void board_set_power_save_mode(Board* self, bool enabled);
const char* board_get_board_json(Board* self);
const char* board_get_device_status_json(Board* self);
const char* board_get_system_info_json(Board* self);

// WiFi板特定功能
int board_enter_wifi_config_mode(Board* self);
int board_start_network(Board* self);
int board_reset_wifi_config(Board* self);

#ifdef __cplusplus
}
#endif

#endif // BOARD_H
```

### 2. 平台特定接口

#### ESP32平台接口 (`board/esp32/common/esp32_board.h`)
```cpp
#ifndef ESP32_BOARD_H
#define ESP32_BOARD_H

#include "board/board.h"
#include "audio/audio/audio_interface.h"
#include "display/display.h"
#include "camera/camera_interface.h"
#include "esp32_audio.h"
#include "esp32_display.h"
#include "esp32_network.h"
#include "esp32_gpio.h"
#include "esp32_camera.h"

#include <string>
#include <memory>

extern "C" {
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
}

class ESP32Board {
public:
    ESP32Board();
    virtual ~ESP32Board();
    
    // 初始化
    virtual int Initialize();
    virtual void Destroy();
    
    // 基本信息
    virtual const char* GetBoardType() const = 0;
    virtual const char* GetUUID();
    
    // 硬件接口
    virtual AudioInterface* GetAudioCodec();
    virtual DisplayInterface* GetDisplay();
    virtual NetworkInterface* GetNetwork();
    virtual CameraInterface* GetCamera();
    virtual GPIOInterface* GetGPIO();
    virtual void* GetLed() = 0;
    
    // 系统功能
    virtual float GetTemperature();
    virtual int GetBatteryLevel();
    virtual void SetPowerSaveMode(bool enabled);
    
    // 配置管理
    virtual const char* GetBoardJson();
    virtual const char* GetDeviceStatusJson();
    virtual const char* GetSystemInfoJson();
    
    // C接口桥接
    Board* GetCInterface();

protected:
    // 初始化子系统
    virtual int InitializeNVS();
    virtual int InitializeI2C();
    virtual int InitializeGPIO();
    virtual int InitializeAudio();
    virtual int InitializeDisplay();
    virtual int InitializeNetwork();
    virtual int InitializeCamera();
    
    // 成员变量
    std::string uuid_;
    bool is_initialized_;
    
    // 硬件接口实例
    std::unique_ptr<ESP32AudioInterface> audio_interface_;
    std::unique_ptr<ESP32DisplayInterface> display_interface_;
    std::unique_ptr<ESP32NetworkInterface> network_interface_;
    std::unique_ptr<ESP32CameraInterface> camera_interface_;
    std::unique_ptr<ESP32GPIOInterface> gpio_interface_;
    
    // I2C配置
    i2c_port_t i2c_port_;
    bool i2c_initialized_;
    
    // C接口桥接
    Board* c_interface_;
    BoardVTable* c_vtable_;
    
private:
    // 禁用拷贝
    ESP32Board(const ESP32Board&) = delete;
    ESP32Board& operator=(const ESP32Board&) = delete;
    
    // C接口回调函数
    static int c_init(Board* self);
    static void c_destroy(Board* self);
    static const char* c_get_board_type(Board* self);
    static const char* c_get_uuid(Board* self);
    static AudioInterface* c_get_audio_codec(Board* self);
    static DisplayInterface* c_get_display(Board* self);
    static NetworkInterface* c_get_network(Board* self);
    static CameraInterface* c_get_camera(Board* self);
    static GPIOInterface* c_get_gpio(Board* self);
    static void* c_get_led(Board* self);
    static float c_get_temperature(Board* self);
    static int c_get_battery_level(Board* self);
    static void c_set_power_save_mode(Board* self, bool enabled);
    static const char* c_get_board_json(Board* self);
    static const char* c_get_device_status_json(Board* self);
    static const char* c_get_system_info_json(Board* self);
};

// WiFi板基类
class ESP32WifiBoard : public ESP32Board {
public:
    ESP32WifiBoard();
    virtual ~ESP32WifiBoard();
    
    // WiFi特定功能
    virtual int EnterWifiConfigMode();
    virtual int StartNetwork();
    virtual int ResetWifiConfig();
    
protected:
    // WiFi初始化
    virtual int InitializeWifi();
    
private:
    // C接口WiFi回调
    static int c_enter_wifi_config_mode(Board* self);
    static int c_start_network(Board* self);
    static int c_reset_wifi_config(Board* self);
};

#endif // ESP32_BOARD_H
```

## 具体板型实现

### 1. Compact WiFi Board实现

#### 配置文件 (`board/esp32/board/bread-compact-wifi/config.json`)
```json
{
  "board": {
    "name": "bread-compact-wifi",
    "type": "esp32s3",
    "version": "1.0.0",
    "description": "Compact WiFi development board"
  },
  "build_configs": [
    {
      "name": "bread-compact-wifi-32",
      "defines": {
        "CONFIG_OLED_SSD1306_128X32": true,
        "AUDIO_I2S_METHOD_SIMPLEX": true
      }
    },
    {
      "name": "bread-compact-wifi-64", 
      "defines": {
        "CONFIG_OLED_SSD1306_128X64": true,
        "AUDIO_I2S_METHOD_SIMPLEX": true
      }
    }
  ],
  "hardware": {
    "audio": {
      "sample_rates": {
        "input": 16000,
        "output": 16000
      },
      "i2s": {
        "bck_pin": 4,
        "ws_pin": 5,
        "data_in_pin": 6,
        "data_out_pin": 7
      }
    },
    "display": {
      "type": "ssd1306",
      "interface": "i2c",
      "width": 128,
      "height": 32,
      "i2c": {
        "sda_pin": 8,
        "scl_pin": 9,
        "frequency": 400000
      }
    },
    "gpio": {
      "led_builtin": 2,
      "boot_button": 0,
      "touch_button": 14,
      "volume_up": 12,
      "volume_down": 13
    },
    "network": {
      "type": "wifi",
      "mode": "sta"
    }
  }
}
```

#### 实现类 (`board/esp32/board/bread-compact-wifi/compact_wifi_board.cpp`)
```cpp
#include "compact_wifi_board.h"
#include "config.h"
#include "esp32_board.h"

extern "C" {
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

// 按钮上下文结构
struct ButtonContext {
    CompactWifiBoard* board;
    int gpio_num;
    const char* name;
    bool long_press_enabled;
    uint32_t press_start_time;
    bool is_pressed;
};

CompactWifiBoard::CompactWifiBoard() 
    : ESP32WifiBoard()
    , display_panel_handle_(nullptr)
    , display_io_handle_(nullptr)
    , display_(nullptr)
    , led_(nullptr)
    , button_contexts_{}
{
}

CompactWifiBoard::~CompactWifiBoard() {
    Destroy();
}

const char* CompactWifiBoard::GetBoardType() const {
    return "bread-compact-wifi";
}

int CompactWifiBoard::Initialize() {
    // 调用基类初始化
    int ret = ESP32WifiBoard::Initialize();
    if (ret != 0) {
        return ret;
    }
    
    // 初始化显示器
    ret = InitializeDisplay();
    if (ret != 0) {
        return ret;
    }
    
    // 初始化LED
    ret = InitializeLed();
    if (ret != 0) {
        return ret;
    }
    
    // 初始化按钮
    ret = InitializeButtons();
    if (ret != 0) {
        return ret;
    }
    
    return 0;
}

void* CompactWifiBoard::GetLed() {
    return led_.get();
}

int CompactWifiBoard::InitializeDisplay() {
    // I2C配置
    i2c_config_t i2c_conf = {};
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = DISPLAY_SDA_PIN;
    i2c_conf.scl_io_num = DISPLAY_SCL_PIN;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = DISPLAY_I2C_FREQ;
    
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0));
    
    // LCD面板IO配置
    esp_lcd_panel_io_i2c_config_t io_config = {};
    io_config.dev_addr = 0x3C;  // SSD1306 I2C地址
    io_config.control_phase_bytes = 1;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, 
                                            &io_config, &display_io_handle_));
    
    // LCD面板配置
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = -1;
    panel_config.bits_per_pixel = 1;
    
#ifdef CONFIG_OLED_SSD1306_128X32
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(display_io_handle_, 
                                              &panel_config, &display_panel_handle_));
#elif defined(CONFIG_OLED_SSD1306_128X64)
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(display_io_handle_, 
                                              &panel_config, &display_panel_handle_));
#elif defined(CONFIG_OLED_SH1106_128X64)
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1106(display_io_handle_, 
                                             &panel_config, &display_panel_handle_));
#endif
    
    // 初始化面板
    ESP_ERROR_CHECK(esp_lcd_panel_reset(display_panel_handle_));
    ESP_ERROR_CHECK(esp_lcd_panel_init(display_panel_handle_));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(display_panel_handle_, true));
    
    // 创建Display对象
    display_ = std::make_unique<OledDisplay>(display_panel_handle_, 
                                           DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    return 0;
}

int CompactWifiBoard::InitializeLed() {
    // 配置LED GPIO
    gpio_config_t led_conf = {};
    led_conf.intr_type = GPIO_INTR_DISABLE;
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pin_bit_mask = (1ULL << LED_BUILTIN_PIN);
    led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    ESP_ERROR_CHECK(gpio_config(&led_conf));
    
    // 创建LED对象
    led_ = std::make_unique<SingleLed>(LED_BUILTIN_PIN);
    
    return 0;
}

int CompactWifiBoard::InitializeButtons() {
    // 按钮GPIO配置
    gpio_config_t button_conf = {};
    button_conf.intr_type = GPIO_INTR_ANYEDGE;
    button_conf.mode = GPIO_MODE_INPUT;
    button_conf.pin_bit_mask = (1ULL << BOOT_BUTTON_PIN) | 
                              (1ULL << TOUCH_BUTTON_PIN) |
                              (1ULL << VOLUME_UP_PIN) | 
                              (1ULL << VOLUME_DOWN_PIN);
    button_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    button_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    
    ESP_ERROR_CHECK(gpio_config(&button_conf));
    
    // 安装GPIO中断服务
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    // 初始化按钮上下文
    button_contexts_[0] = {this, BOOT_BUTTON_PIN, "boot", true, 0, false};
    button_contexts_[1] = {this, TOUCH_BUTTON_PIN, "touch", false, 0, false};
    button_contexts_[2] = {this, VOLUME_UP_PIN, "volume_up", true, 0, false};
    button_contexts_[3] = {this, VOLUME_DOWN_PIN, "volume_down", true, 0, false};
    
    // 注册中断处理函数
    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)button_contexts_[i].gpio_num,
                                           button_isr_handler, &button_contexts_[i]));
    }
    
    return 0;
}

void IRAM_ATTR CompactWifiBoard::button_isr_handler(void* arg) {
    ButtonContext* ctx = static_cast<ButtonContext*>(arg);
    uint32_t current_time = xTaskGetTickCountFromISR();
    int level = gpio_get_level((gpio_num_t)ctx->gpio_num);
    
    if (level == 0) {  // 按下
        ctx->is_pressed = true;
        ctx->press_start_time = current_time;
    } else {  // 释放
        if (ctx->is_pressed) {
            uint32_t press_duration = current_time - ctx->press_start_time;
            
            if (press_duration > pdMS_TO_TICKS(50)) {  // 防抖
                if (press_duration > pdMS_TO_TICKS(2000) && ctx->long_press_enabled) {
                    // 长按处理
                    ctx->board->HandleButtonLongPress(ctx->gpio_num);
                } else {
                    // 短按处理
                    ctx->board->HandleButtonPress(ctx->gpio_num);
                }
            }
            
            ctx->is_pressed = false;
        }
    }
}

void CompactWifiBoard::HandleButtonPress(int gpio_num) {
    switch (gpio_num) {
        case BOOT_BUTTON_PIN:
            // 切换应用状态
            if (display_) {
                display_->ShowNotification("App State Toggle", 2000);
            }
            break;
            
        case TOUCH_BUTTON_PIN:
            // 触摸按钮功能
            if (display_) {
                display_->ShowNotification("Touch Button", 1000);
            }
            break;
            
        case VOLUME_UP_PIN:
            // 音量增加
            if (audio_interface_) {
                int current_volume = audio_interface_->output_volume_;
                int new_volume = (current_volume + 10 > 100) ? 100 : current_volume + 10;
                audio_interface_set_output_volume(audio_interface_.get(), new_volume);
                
                if (display_) {
                    char msg[32];
                    snprintf(msg, sizeof(msg), "Volume: %d", new_volume);
                    display_->ShowNotification(msg, 1500);
                }
            }
            break;
            
        case VOLUME_DOWN_PIN:
            // 音量减少
            if (audio_interface_) {
                int current_volume = audio_interface_->output_volume_;
                int new_volume = (current_volume - 10 < 0) ? 0 : current_volume - 10;
                audio_interface_set_output_volume(audio_interface_.get(), new_volume);
                
                if (display_) {
                    char msg[32];
                    snprintf(msg, sizeof(msg), "Volume: %d", new_volume);
                    display_->ShowNotification(msg, 1500);
                }
            }
            break;
    }
}

void CompactWifiBoard::HandleButtonLongPress(int gpio_num) {
    switch (gpio_num) {
        case BOOT_BUTTON_PIN:
            // 重置WiFi配置
            ResetWifiConfig();
            if (display_) {
                display_->ShowNotification("WiFi Reset", 3000);
            }
            break;
            
        case VOLUME_UP_PIN:
            // 开始监听
            if (display_) {
                display_->ShowNotification("Start Listening", 2000);
            }
            break;
            
        case VOLUME_DOWN_PIN:
            // 停止监听
            if (display_) {
                display_->ShowNotification("Stop Listening", 2000);
            }
            break;
    }
}

// C接口工厂函数
extern "C" Board* create_board(void) {
    static CompactWifiBoard* instance = nullptr;
    if (!instance) {
        instance = new CompactWifiBoard();
        if (instance->Initialize() != 0) {
            delete instance;
            instance = nullptr;
        }
    }
    return instance ? instance->GetCInterface() : nullptr;
}
```

### 2. 配置头文件生成

#### 配置生成脚本 (`board/esp32/board/bread-compact-wifi/generate_config.py`)
```python
#!/usr/bin/env python3
import json
import os
import sys

def generate_config_h(config_file, output_file):
    """从JSON配置生成config.h文件"""
    
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    # 获取硬件配置
    hardware = config.get('hardware', {})
    audio = hardware.get('audio', {})
    display = hardware.get('display', {})
    gpio = hardware.get('gpio', {})
    
    # 生成头文件内容
    content = f"""#ifndef CONFIG_H
#define CONFIG_H

// 自动生成的配置文件，请勿手动修改
// Generated from: {os.path.basename(config_file)}

// 板级信息
#define BOARD_NAME "{config['board']['name']}"
#define BOARD_TYPE "{config['board']['type']}"
#define BOARD_VERSION "{config['board']['version']}"

// 音频配置
#define AUDIO_INPUT_SAMPLE_RATE {audio.get('sample_rates', {}).get('input', 16000)}
#define AUDIO_OUTPUT_SAMPLE_RATE {audio.get('sample_rates', {}).get('output', 16000)}
#define AUDIO_I2S_BCK_PIN {audio.get('i2s', {}).get('bck_pin', 4)}
#define AUDIO_I2S_WS_PIN {audio.get('i2s', {}).get('ws_pin', 5)}
#define AUDIO_I2S_DATA_IN_PIN {audio.get('i2s', {}).get('data_in_pin', 6)}
#define AUDIO_I2S_DATA_OUT_PIN {audio.get('i2s', {}).get('data_out_pin', 7)}

// 显示配置
#define DISPLAY_WIDTH {display.get('width', 128)}
#define DISPLAY_HEIGHT {display.get('height', 32)}
#define DISPLAY_SDA_PIN {display.get('i2c', {}).get('sda_pin', 8)}
#define DISPLAY_SCL_PIN {display.get('i2c', {}).get('scl_pin', 9)}
#define DISPLAY_I2C_FREQ {display.get('i2c', {}).get('frequency', 400000)}

// GPIO配置
#define LED_BUILTIN_PIN {gpio.get('led_builtin', 2)}
#define BOOT_BUTTON_PIN {gpio.get('boot_button', 0)}
#define TOUCH_BUTTON_PIN {gpio.get('touch_button', 14)}
#define VOLUME_UP_PIN {gpio.get('volume_up', 12)}
#define VOLUME_DOWN_PIN {gpio.get('volume_down', 13)}

// 构建配置宏
"""
    
    # 添加构建配置宏
    for build_config in config.get('build_configs', []):
        content += f"\n// Build config: {build_config['name']}\n"
        for define, value in build_config.get('defines', {}).items():
            if isinstance(value, bool):
                if value:
                    content += f"#define {define}\n"
            else:
                content += f"#define {define} {value}\n"
    
    content += "\n#endif // CONFIG_H\n"
    
    # 写入文件
    with open(output_file, 'w') as f:
        f.write(content)
    
    print(f"Generated {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: generate_config.py <config.json> <config.h>")
        sys.exit(1)
    
    generate_config_h(sys.argv[1], sys.argv[2])
```

## 平台特定实现

### 1. ESP32音频实现 (`board/esp32/common/esp32_audio.cpp`)
```cpp
#include "esp32_audio.h"

extern "C" {
#include "driver/i2s.h"
#include "esp_log.h"
}

static const char* TAG = "ESP32Audio";

ESP32AudioInterface::ESP32AudioInterface() 
    : i2s_port_(I2S_NUM_0)
    , is_initialized_(false)
{
    // 初始化AudioInterface结构
    memset(&audio_interface_, 0, sizeof(AudioInterface));
    audio_interface_.vtable = &vtable_;
    audio_interface_.impl_data = this;
    
    // 设置默认配置
    audio_interface_.sample_rate = 16000;
    audio_interface_.channels = 1;
    audio_interface_.frame_size = 160;
    audio_interface_.periods = 4;
    audio_interface_.buffer_size = 1024;
    audio_interface_.period_size = 256;
    audio_interface_.output_volume_ = 70;
    audio_interface_.duplex_ = false;
    audio_interface_.input_enabled_ = true;
    audio_interface_.output_enabled_ = true;
}

ESP32AudioInterface::~ESP32AudioInterface() {
    Destroy();
}

int ESP32AudioInterface::Initialize() {
    if (is_initialized_) {
        return 0;
    }
    
    // I2S配置
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = audio_interface_.sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
        .dma_buf_count = audio_interface_.periods,
        .dma_buf_len = audio_interface_.period_size,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S引脚配置
    i2s_pin_config_t pin_config = {
        .bck_io_num = AUDIO_I2S_BCK_PIN,
        .ws_io_num = AUDIO_I2S_WS_PIN,
        .data_out_num = AUDIO_I2S_DATA_OUT_PIN,
        .data_in_num = AUDIO_I2S_DATA_IN_PIN
    };
    
    // 安装I2S驱动
    esp_err_t ret = i2s_driver_install(i2s_port_, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return -1;
    }
    
    // 设置I2S引脚
    ret = i2s_set_pin(i2s_port_, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(i2s_port_);
        return -1;
    }
    
    is_initialized_ = true;
    audio_interface_.is_initialized = true;
    
    ESP_LOGI(TAG, "ESP32 audio interface initialized");
    return 0;
}

int ESP32AudioInterface::Start() {
    if (!is_initialized_) {
        return -1;
    }
    
    esp_err_t ret = i2s_start(i2s_port_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %s", esp_err_to_name(ret));
        return -1;
    }
    
    audio_interface_.is_started = true;
    ESP_LOGI(TAG, "ESP32 audio interface started");
    return 0;
}

void ESP32AudioInterface::Destroy() {
    if (is_initialized_) {
        i2s_stop(i2s_port_);
        i2s_driver_uninstall(i2s_port_);
        is_initialized_ = false;
        audio_interface_.is_initialized = false;
        audio_interface_.is_started = false;
    }
}

int ESP32AudioInterface::InputData(int16_t* data, size_t samples) {
    if (!is_initialized_ || !audio_interface_.input_enabled_) {
        return -1;
    }
    
    size_t bytes_read = 0;
    esp_err_t ret = i2s_read(i2s_port_, data, samples * sizeof(int16_t), 
                            &bytes_read, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return -1;
    }
    
    return bytes_read / sizeof(int16_t);
}

int ESP32AudioInterface::OutputData(const int16_t* data, size_t samples) {
    if (!is_initialized_ || !audio_interface_.output_enabled_) {
        return -1;
    }
    
    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(i2s_port_, data, samples * sizeof(int16_t), 
                             &bytes_written, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(ret));
        return -1;
    }
    
    return bytes_written / sizeof(int16_t);
}

AudioInterface* ESP32AudioInterface::GetCInterface() {
    return &audio_interface_;
}

// C接口实现
int ESP32AudioInterface::c_init(AudioInterface* self) {
    ESP32AudioInterface* impl = static_cast<ESP32AudioInterface*>(self->impl_data);
    return impl->Initialize();
}

int ESP32AudioInterface::c_start(AudioInterface* self) {
    ESP32AudioInterface* impl = static_cast<ESP32AudioInterface*>(self->impl_data);
    return impl->Start();
}

int ESP32AudioInterface::c_destroy(AudioInterface* self) {
    ESP32AudioInterface* impl = static_cast<ESP32AudioInterface*>(self->impl_data);
    impl->Destroy();
    return 0;
}

int ESP32AudioInterface::c_input_data(AudioInterface* self, int16_t* data, size_t samples) {
    ESP32AudioInterface* impl = static_cast<ESP32AudioInterface*>(self->impl_data);
    return impl->InputData(data, samples);
}

int ESP32AudioInterface::c_output_data(AudioInterface* self, const int16_t* data, size_t samples) {
    ESP32AudioInterface* impl = static_cast<ESP32AudioInterface*>(self->impl_data);
    return impl->OutputData(data, samples);
}

// 虚函数表初始化
const AudioInterfaceVTable ESP32AudioInterface::vtable_ = {
    .init = c_init,
    .start = c_start,
    .destroy = c_destroy,
    .set_output_volume = nullptr,  // 实现其他函数...
    .enable_input = nullptr,
    .enable_output = nullptr,
    .output_data = c_output_data,
    .input_data = c_input_data,
    .read = nullptr,
    .write = nullptr,
    .set_config = nullptr
};
```

## 构建系统集成

### 1. 主CMakeLists.txt (`board/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.10)
project(linx_board)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 平台检测
if(DEFINED ESP_PLATFORM)
    set(PLATFORM "esp32")
    add_subdirectory(esp32)
elseif(APPLE)
    set(PLATFORM "mac")
    add_subdirectory(mac)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# 创建board库
add_library(linx_board INTERFACE)

# 链接平台特定实现
if(PLATFORM STREQUAL "esp32")
    target_link_libraries(linx_board INTERFACE linx_board_esp32)
elseif(PLATFORM STREQUAL "mac")
    target_link_libraries(linx_board INTERFACE linx_board_mac)
endif()

# 包含目录
target_include_directories(linx_board INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${PLATFORM}
)
```

### 2. ESP32平台构建 (`board/esp32/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.10)
project(linx_board_esp32)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# 通用ESP32实现
add_subdirectory(common)

# 检测板型
if(DEFINED BOARD_TYPE)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/board/${BOARD_TYPE})
        add_subdirectory(board/${BOARD_TYPE})
    else()
        message(FATAL_ERROR "Board type ${BOARD_TYPE} not found")
    endif()
else()
    # 默认板型
    set(BOARD_TYPE "bread-compact-wifi")
    add_subdirectory(board/${BOARD_TYPE})
endif()

# 创建ESP32 board库
add_library(linx_board_esp32 STATIC)

# 链接子模块
target_link_libraries(linx_board_esp32 PUBLIC
    linx_board_esp32_common
    linx_board_${BOARD_TYPE}
)

# ESP-IDF组件依赖
if(DEFINED ESP_PLATFORM)
    idf_component_register(
        SRCS ""
        INCLUDE_DIRS "."
        REQUIRES driver esp_lcd nvs_flash
    )
endif()
```

## 测试策略

### 1. 单元测试
```cpp
// board/esp32/test/test_compact_wifi_board.cpp
#include "gtest/gtest.h"
#include "compact_wifi_board.h"

class CompactWifiBoardTest : public ::testing::Test {
protected:
    void SetUp() override {
        board_ = std::make_unique<CompactWifiBoard>();
    }
    
    void TearDown() override {
        board_.reset();
    }
    
    std::unique_ptr<CompactWifiBoard> board_;
};

TEST_F(CompactWifiBoardTest, Initialization) {
    EXPECT_EQ(board_->Initialize(), 0);
    EXPECT_STREQ(board_->GetBoardType(), "bread-compact-wifi");
    EXPECT_NE(board_->GetUUID(), nullptr);
}

TEST_F(CompactWifiBoardTest, AudioInterface) {
    ASSERT_EQ(board_->Initialize(), 0);
    
    AudioInterface* audio = board_->GetAudioCodec();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_EQ(audio_interface_init(audio), 0);
    EXPECT_EQ(audio_interface_start(audio), 0);
}

TEST_F(CompactWifiBoardTest, DisplayInterface) {
    ASSERT_EQ(board_->Initialize(), 0);
    
    DisplayInterface* display = board_->GetDisplay();
    ASSERT_NE(display, nullptr);
    
    EXPECT_EQ(display_interface_init(display), 0);
    display_interface_set_status(display, "Test Status");
}
```

### 2. 集成测试
```cpp
// board/esp32/test/test_board_integration.cpp
#include "gtest/gtest.h"
#include "board/board.h"

extern "C" Board* create_board(void);

class BoardIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        board_ = create_board();
        ASSERT_NE(board_, nullptr);
    }
    
    void TearDown() override {
        if (board_) {
            board_destroy(board_);
        }
    }
    
    Board* board_;
};

TEST_F(BoardIntegrationTest, FullInitialization) {
    EXPECT_EQ(board_init(board_), 0);
    
    // 测试所有接口
    EXPECT_NE(board_get_audio_codec(board_), nullptr);
    EXPECT_NE(board_get_display(board_), nullptr);
    EXPECT_NE(board_get_network(board_), nullptr);
    EXPECT_NE(board_get_gpio(board_), nullptr);
    EXPECT_NE(board_get_led(board_), nullptr);
}

TEST_F(BoardIntegrationTest, AudioDisplayInteraction) {
    ASSERT_EQ(board_init(board_), 0);
    
    AudioInterface* audio = board_get_audio_codec(board_);
    DisplayInterface* display = board_get_display(board_);
    
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(display, nullptr);
    
    // 测试音频音量变化时显示更新
    audio_interface_set_output_volume(audio, 80);
    display_interface_show_notification(display, "Volume: 80", 1000);
    
    // 验证状态同步
    EXPECT_EQ(audio_interface_output_volume(audio), 80);
}
```

## 迁移策略

### 1. 阶段性迁移
1. **阶段1**: 创建基础框架和接口定义
2. **阶段2**: 实现ESP32平台的基础功能
3. **阶段3**: 迁移CompactWifiBoard的所有功能
4. **阶段4**: 添加测试和文档
5. **阶段5**: 性能优化和稳定性改进

### 2. 兼容性保证
- 保持现有API的兼容性
- 提供迁移工具和脚本
- 详细的迁移文档和示例

### 3. 验证策略
- 功能对比测试
- 性能基准测试
- 长期稳定性测试

## 总结

这个Board模块实现设计提供了：

1. **统一的C99接口**: 保证跨平台兼容性
2. **模块化设计**: 易于扩展和维护
3. **完整的功能复制**: 保持与bak目录的功能一致性
4. **现代C++实现**: 利用C++的优势进行平台特定实现
5. **灵活的配置系统**: 支持多种硬件配置
6. **完善的测试覆盖**: 保证代码质量和稳定性

通过这个设计，我们可以实现一个既保持向后兼容性，又具有良好扩展性的跨平台Board抽象层。