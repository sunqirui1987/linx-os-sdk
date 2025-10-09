#ifndef ESP32_BOARD_H
#define ESP32_BOARD_H

#include "../../src/board/board.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ESP32 Bread Compact WiFi Board Configuration
#define ESP32_BOARD_NAME "ESP32-BREAD-COMPACT-WIFI"
#define ESP32_BOARD_VERSION "1.0.0"

// Audio Configuration
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Audio I2S Configuration (Simplex Mode)
#define AUDIO_I2S_MIC_GPIO_WS   4
#define AUDIO_I2S_MIC_GPIO_SCK  5
#define AUDIO_I2S_MIC_GPIO_DIN  6
#define AUDIO_I2S_SPK_GPIO_DOUT 7
#define AUDIO_I2S_SPK_GPIO_BCLK 15
#define AUDIO_I2S_SPK_GPIO_LRCK 16

// GPIO Configuration
#define BUILTIN_LED_GPIO        48
#define BOOT_BUTTON_GPIO        0
#define TOUCH_BUTTON_GPIO       47
#define VOLUME_UP_BUTTON_GPIO   40
#define VOLUME_DOWN_BUTTON_GPIO 39

// Display Configuration (OLED)
#define DISPLAY_SDA_PIN 41
#define DISPLAY_SCL_PIN 42
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  64  // Default to 64, can be configured

// WiFi configuration
typedef struct {
    bool config_mode;
    bool connected;
    char ssid[64];
    int8_t rssi;
    uint8_t channel;
    char ip_address[16];
    char mac_address[18];
} ESP32WiFiInfo;

// ESP32 specific board structure
typedef struct ESP32Board {
    Board base;                    // Base board structure
    bool wifi_config_mode;         // WiFi configuration mode flag
    char* ssid;                    // Current WiFi SSID
    int8_t rssi;                   // WiFi signal strength
    uint8_t channel;               // WiFi channel
    char* ip_address;              // Current IP address
    char* mac_address;             // MAC address
    bool power_save_enabled;       // Power save mode status
    void* display_instance;        // Display interface instance
    void* audio_codec_instance;    // Audio codec instance
    void* network_instance;        // Network interface instance
    void* camera_instance;         // Camera instance
    void* led_instance;            // LED instance
    void* backlight_instance;      // Backlight instance
} ESP32Board;

// ESP32Board constructor and destructor
ESP32Board* esp32_board_create(void);
void esp32_board_destroy(ESP32Board* self);

// ESP32 specific methods
void esp32_board_enter_wifi_config_mode(ESP32Board* self);
void esp32_board_reset_wifi_configuration(ESP32Board* self);
bool esp32_board_connect_wifi(ESP32Board* self, const char* ssid, const char* password);
void esp32_board_scan_wifi_networks(ESP32Board* self);

// WiFi management functions
bool esp32_board_is_wifi_connected(ESP32Board* self);
const char* esp32_board_get_wifi_ssid(ESP32Board* self);
int8_t esp32_board_get_wifi_rssi(ESP32Board* self);
const char* esp32_board_get_ip_address(ESP32Board* self);
const char* esp32_board_get_mac_address(ESP32Board* self);

// Hardware initialization functions
bool esp32_board_init_wifi(ESP32Board* self);
bool esp32_board_init_display(ESP32Board* self);
bool esp32_board_init_audio(ESP32Board* self);
bool esp32_board_init_camera(ESP32Board* self);
bool esp32_board_init_led(ESP32Board* self);
bool esp32_board_init_backlight(ESP32Board* self);

// Board vtable implementation functions
const char* esp32_board_get_board_type(Board* self);
void* esp32_board_get_audio_codec(Board* self);
void* esp32_board_get_network(Board* self);
void esp32_board_start_network(Board* self);
const char* esp32_board_get_network_state_icon(Board* self);
void esp32_board_set_power_save_mode(Board* self, bool enabled);
const char* esp32_board_get_board_json(Board* self);
const char* esp32_board_get_device_status_json(Board* self);
const char* esp32_board_get_uuid(Board* self);
void* esp32_board_get_backlight(Board* self);
void* esp32_board_get_led(Board* self);
bool esp32_board_get_temperature(Board* self, float* temperature);
void* esp32_board_get_display(Board* self);
void* esp32_board_get_camera(Board* self);
bool esp32_board_get_battery_level(Board* self, int* level, bool* charging, bool* discharging);
const char* esp32_board_get_system_info_json(Board* self);
void esp32_board_destroy_impl(Board* self);

#ifdef __cplusplus
}
#endif

#endif // ESP32_BOARD_H