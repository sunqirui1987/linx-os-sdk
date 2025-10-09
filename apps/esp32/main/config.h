#ifndef ESP32_APP_CONFIG_H
#define ESP32_APP_CONFIG_H

// Application Configuration
#define APP_NAME "ESP32 AI Assistant"
#define APP_VERSION "1.0.0"
#define APP_AUTHOR "LinX OS SDK"

// Board Configuration
#define BOARD_TYPE "bread-compact-wifi"
#define BOARD_NAME "ESP32 Bread Compact WiFi"

// Audio Configuration
#define ENABLE_AUDIO_SERVICE 1
#define ENABLE_WAKE_WORD_DETECTION 1
#define ENABLE_VAD 1
#define ENABLE_AEC 1

// Network Configuration
#define ENABLE_WIFI 1
#define ENABLE_MQTT 1
#define ENABLE_WEBSOCKET 1
#define ENABLE_HTTP_CLIENT 1

// Display Configuration
#define ENABLE_DISPLAY 1
#define DISPLAY_TYPE_OLED 1
#define OLED_SSD1306 1

// OTA Configuration
#define ENABLE_OTA 1
#define OTA_CHECK_INTERVAL_MS (60 * 60 * 1000)  // 1 hour

// MCP Configuration
#define ENABLE_MCP_SERVER 1
#define MCP_SERVER_PORT 8080

// Logging Configuration
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_ERROR 1

// Task Configuration
#define MAIN_TASK_STACK_SIZE 8192
#define MAIN_TASK_PRIORITY 5
#define AUDIO_TASK_STACK_SIZE 4096
#define AUDIO_TASK_PRIORITY 6
#define NETWORK_TASK_STACK_SIZE 4096
#define NETWORK_TASK_PRIORITY 4

// Memory Configuration
#define MAX_TASK_QUEUE_SIZE 50
#define MAX_AUDIO_BUFFER_SIZE 4096
#define MAX_MESSAGE_SIZE 1024

// Timing Configuration
#define CLOCK_TICK_INTERVAL_MS 1000
#define WATCHDOG_TIMEOUT_MS 30000
#define NETWORK_TIMEOUT_MS 10000
#define AUDIO_TIMEOUT_MS 5000

// Feature Flags
#define FEATURE_POWER_MANAGEMENT 1
#define FEATURE_SLEEP_MODE 1
#define FEATURE_BUTTON_CONTROL 1
#define FEATURE_LED_INDICATION 1
#define FEATURE_TEMPERATURE_MONITORING 1
#define FEATURE_BATTERY_MONITORING 0  // Not available on bread board

// Debug Configuration
#ifdef DEBUG
#define DEBUG_MEMORY_USAGE 1
#define DEBUG_TASK_MONITORING 1
#define DEBUG_AUDIO_PIPELINE 1
#define DEBUG_NETWORK_TRAFFIC 1
#else
#define DEBUG_MEMORY_USAGE 0
#define DEBUG_TASK_MONITORING 0
#define DEBUG_AUDIO_PIPELINE 0
#define DEBUG_NETWORK_TRAFFIC 0
#endif

// Error Handling
#define MAX_ERROR_MESSAGE_LENGTH 256
#define MAX_RETRY_COUNT 3
#define RETRY_DELAY_MS 1000

// Security Configuration
#define ENABLE_SECURE_BOOT 0
#define ENABLE_FLASH_ENCRYPTION 0
#define ENABLE_SECURE_COMMUNICATION 1

#endif // ESP32_APP_CONFIG_H