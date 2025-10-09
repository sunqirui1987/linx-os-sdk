#include "esp32_board.h"
#include "../../src/common/settings.h"
#include "../../src/common/system_info.h"
#include "../../src/common/log/linx_log.h"
#include "../../src/display/display.h"
#include "../../src/camera/camera_interface.h"
#include "../../src/common/cjson/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ESP32Board"

// ESP32Board vtable
static const BoardVTable esp32_board_vtable = {
    .get_board_type = esp32_board_get_board_type,
    .get_audio_codec = esp32_board_get_audio_codec,
    .get_network = esp32_board_get_network,
    .start_network = esp32_board_start_network,
    .get_network_state_icon = esp32_board_get_network_state_icon,
    .set_power_save_mode = esp32_board_set_power_save_mode,
    .get_board_json = esp32_board_get_board_json,
    .get_device_status_json = esp32_board_get_device_status_json,
    .get_uuid = esp32_board_get_uuid,
    .get_backlight = esp32_board_get_backlight,
    .get_led = esp32_board_get_led,
    .get_temperature = esp32_board_get_temperature,
    .get_display = esp32_board_get_display,
    .get_camera = esp32_board_get_camera,
    .get_battery_level = esp32_board_get_battery_level,
    .get_system_info_json = esp32_board_get_system_info_json,
    .destroy = esp32_board_destroy_impl
};

ESP32Board* esp32_board_create(void) {
    ESP32Board* self = (ESP32Board*)malloc(sizeof(ESP32Board));
    if (!self) {
        LINX_LOG_ERROR(TAG, "Failed to allocate memory for ESP32Board");
        return NULL;
    }

    // Initialize base board
    memset(self, 0, sizeof(ESP32Board));
    self->base.vtable = &esp32_board_vtable;
    self->base.uuid_ = board_generate_uuid();
    
    // Initialize ESP32 specific members
    self->wifi_config_mode = false;
    self->power_save_enabled = false;
    self->ssid = NULL;
    self->ip_address = NULL;
    self->mac_address = NULL;
    self->rssi = -100;
    self->channel = 0;
    
    // Initialize hardware components
    esp32_board_init_wifi(self);
    esp32_board_init_display(self);
    esp32_board_init_audio(self);
    esp32_board_init_camera(self);
    esp32_board_init_led(self);
    esp32_board_init_backlight(self);
    
    LINX_LOG_INFO(TAG, "ESP32Board created successfully");
    return self;
}

void esp32_board_destroy(ESP32Board* self) {
    if (!self) return;
    
    // Free allocated strings
    if (self->ssid) {
        free(self->ssid);
    }
    if (self->ip_address) {
        free(self->ip_address);
    }
    if (self->mac_address) {
        free(self->mac_address);
    }
    if (self->base.uuid_) {
        free(self->base.uuid_);
    }
    
    // Cleanup hardware components
    // TODO: Add proper cleanup for display, audio, camera, etc.
    
    free(self);
    LINX_LOG_INFO(TAG, "ESP32Board destroyed");
}

// Hardware initialization functions
bool esp32_board_init_wifi(ESP32Board* self) {
    if (!self) return false;
    
    // Initialize WiFi hardware for bread-compact-wifi board
    self->wifi_config_mode = false;
    self->rssi = -100;
    self->channel = 0;
    
    // Set default MAC address
    if (self->mac_address) {
        free(self->mac_address);
    }
    self->mac_address = strdup("00:00:00:00:00:00");
    
    // Configure boot button for WiFi reset (GPIO_0)
    gpio_config_t boot_btn_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&boot_btn_config);
    
    // Configure touch button for interaction (GPIO_47)
    gpio_config_t touch_btn_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_47),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&touch_btn_config);
    
    // Configure volume buttons
    gpio_config_t vol_up_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_40),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&vol_up_config);
    
    gpio_config_t vol_down_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_39),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&vol_down_config);
    
    LINX_LOG_INFO(TAG, "WiFi and buttons initialized for bread-compact-wifi board");
    return true;
}

bool esp32_board_init_display(ESP32Board* self) {
    if (!self) return false;
    
    // Initialize OLED display for bread-compact-wifi board
    // I2C configuration for SSD1306 OLED display
    // - SDA: GPIO_41
    // - SCL: GPIO_42
    // - Resolution: 128x64
    
    // Configure I2C pins for display
    gpio_config_t i2c_gpio_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_41) | (1ULL << GPIO_NUM_42),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  // Open drain for I2C
        .pull_up_en = GPIO_PULLUP_ENABLE,   // Enable pull-up for I2C
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&i2c_gpio_config);
    
    // Initialize display interface using the display module
    self->display_instance = display_interface_create();
    if (!self->display_instance) {
        LINX_LOG_ERROR(TAG, "Failed to create display interface");
        return false;
    }
    
    LINX_LOG_INFO(TAG, "OLED display initialized for bread-compact-wifi board");
    LINX_LOG_INFO(TAG, "  I2C pins: SDA=%d, SCL=%d", GPIO_NUM_41, GPIO_NUM_42);
    LINX_LOG_INFO(TAG, "  Resolution: 128x64");
    LINX_LOG_INFO(TAG, "  Display type: SSD1306 OLED");
    
    return true;
}

bool esp32_board_init_audio(ESP32Board* self) {
    if (!self) return false;
    
    // Initialize I2S audio for bread-compact-wifi board (Simplex mode)
    // Microphone I2S configuration
    // - WS (Word Select): GPIO_4
    // - SCK (Serial Clock): GPIO_5
    // - DIN (Data In): GPIO_6
    
    // Speaker I2S configuration  
    // - DOUT (Data Out): GPIO_7
    // - BCLK (Bit Clock): GPIO_15
    // - LRCK (Left/Right Clock): GPIO_16
    
    // Configure I2S pins for microphone
    gpio_config_t mic_gpio_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_4) | 
                       (1ULL << GPIO_NUM_5) | 
                       (1ULL << GPIO_NUM_6),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&mic_gpio_config);
    
    // Configure I2S pins for speaker
    gpio_config_t spk_gpio_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_7) | 
                       (1ULL << GPIO_NUM_15) | 
                       (1ULL << GPIO_NUM_16),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&spk_gpio_config);
    
    // TODO: Initialize audio codec
    self->audio_codec_instance = NULL;
    
    LINX_LOG_INFO(TAG, "Audio I2S initialized for bread-compact-wifi board");
    LINX_LOG_INFO(TAG, "  Microphone: WS=%d, SCK=%d, DIN=%d", 
                 GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6);
    LINX_LOG_INFO(TAG, "  Speaker: DOUT=%d, BCLK=%d, LRCK=%d", 
                 GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_16);
    LINX_LOG_INFO(TAG, "  Sample rates: Input=16000Hz, Output=16000Hz");
    
    return true;
}

bool esp32_board_init_camera(ESP32Board* self) {
    if (!self) return false;
    
    // Initialize camera interface
    self->camera_instance = camera_interface_create();
    if (!self->camera_instance) {
        LINX_LOG_ERROR(TAG, "Failed to create camera interface");
        return false;
    }
    
    LINX_LOG_INFO(TAG, "Camera initialized");
    return true;
}

bool esp32_board_init_led(ESP32Board* self) {
    if (!self) return false;
    
    // Initialize built-in LED for bread-compact-wifi board
    // LED GPIO: GPIO_48
    
    // Configure LED GPIO
    gpio_config_t led_gpio_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_48),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_gpio_config);
    
    // Set initial LED state (off)
    gpio_set_level(GPIO_NUM_48, 0);
    
    // Initialize LED interface
    self->led_instance = NULL;  // Will be initialized by LED driver
    
    LINX_LOG_INFO(TAG, "Built-in LED initialized for bread-compact-wifi board");
    LINX_LOG_INFO(TAG, "  LED GPIO: %d", GPIO_NUM_48);
    LINX_LOG_INFO(TAG, "  Initial state: OFF");
    
    return true;
}

bool esp32_board_init_backlight(ESP32Board* self) {
    if (!self) return false;
    
    // TODO: Initialize backlight
    self->backlight_instance = NULL;
    
    LINX_LOG_INFO(TAG, "Backlight initialized");
    return true;
}

// WiFi management functions
void esp32_board_enter_wifi_config_mode(ESP32Board* self) {
    if (!self) return;
    
    self->wifi_config_mode = true;
    LINX_LOG_INFO(TAG, "Entering WiFi configuration mode");
    
    // TODO: Start WiFi AP mode for configuration
}

void esp32_board_reset_wifi_configuration(ESP32Board* self) {
    if (!self) return;
    
    // Clear WiFi settings
    Settings* settings = settings_create("wifi");
    if (settings) {
        settings_set_int(settings, "force_ap", 1);
        settings_destroy(settings);
    }
    
    LINX_LOG_INFO(TAG, "WiFi configuration reset");
}

bool esp32_board_connect_wifi(ESP32Board* self, const char* ssid, const char* password) {
    if (!self || !ssid) return false;
    
    // TODO: Implement actual WiFi connection
    if (self->ssid) {
        free(self->ssid);
    }
    self->ssid = strdup(ssid);
    self->rssi = -50; // Mock signal strength
    self->channel = 6; // Mock channel
    
    if (self->ip_address) {
        free(self->ip_address);
    }
    self->ip_address = strdup("192.168.1.100"); // Mock IP
    
    LINX_LOG_INFO(TAG, "Connected to WiFi: %s", ssid);
    return true;
}

bool esp32_board_is_wifi_connected(ESP32Board* self) {
    if (!self) return false;
    return self->ssid != NULL && !self->wifi_config_mode;
}

const char* esp32_board_get_wifi_ssid(ESP32Board* self) {
    if (!self) return NULL;
    return self->ssid;
}

int8_t esp32_board_get_wifi_rssi(ESP32Board* self) {
    if (!self) return -100;
    return self->rssi;
}

const char* esp32_board_get_ip_address(ESP32Board* self) {
    if (!self) return NULL;
    return self->ip_address;
}

const char* esp32_board_get_mac_address(ESP32Board* self) {
    if (!self) return NULL;
    return self->mac_address;
}

// Board vtable implementation functions
const char* esp32_board_get_board_type(Board* self) {
    return "esp32";
}

void* esp32_board_get_audio_codec(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->audio_codec_instance;
}

void* esp32_board_get_network(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->network_instance;
}

void esp32_board_start_network(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    
    // Check if we should enter WiFi config mode
    Settings* settings = settings_create("wifi");
    if (settings) {
        int force_ap = settings_get_int(settings, "force_ap");
        if (force_ap == 1) {
            esp32_self->wifi_config_mode = true;
            settings_set_int(settings, "force_ap", 0);
        }
        settings_destroy(settings);
    }
    
    if (esp32_self->wifi_config_mode) {
        esp32_board_enter_wifi_config_mode(esp32_self);
        return;
    }
    
    // TODO: Try to connect to saved WiFi networks
    // For now, just mock a connection
    esp32_board_connect_wifi(esp32_self, "TestNetwork", "password");
}

const char* esp32_board_get_network_state_icon(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    
    if (esp32_self->wifi_config_mode) {
        return "📶"; // WiFi config mode icon
    }
    
    if (!esp32_board_is_wifi_connected(esp32_self)) {
        return "📵"; // No WiFi icon
    }
    
    // Return icon based on signal strength
    if (esp32_self->rssi >= -60) {
        return "📶"; // Strong signal
    } else if (esp32_self->rssi >= -70) {
        return "📶"; // Medium signal
    } else {
        return "📶"; // Weak signal
    }
}

void esp32_board_set_power_save_mode(Board* self, bool enabled) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    esp32_self->power_save_enabled = enabled;
    
    // TODO: Implement actual power save mode
    LINX_LOG_INFO(TAG, "Power save mode %s", enabled ? "enabled" : "disabled");
}

const char* esp32_board_get_board_json(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "esp32");
    cJSON_AddStringToObject(root, "name", "ESP32 Development Board");
    
    if (!esp32_self->wifi_config_mode && esp32_board_is_wifi_connected(esp32_self)) {
        cJSON_AddStringToObject(root, "ssid", esp32_self->ssid ? esp32_self->ssid : "");
        cJSON_AddNumberToObject(root, "rssi", esp32_self->rssi);
        cJSON_AddNumberToObject(root, "channel", esp32_self->channel);
        cJSON_AddStringToObject(root, "ip", esp32_self->ip_address ? esp32_self->ip_address : "");
    }
    
    cJSON_AddStringToObject(root, "mac", esp32_self->mac_address ? esp32_self->mac_address : "");
    
    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    // Note: This creates a memory leak. In a real implementation,
    // you should manage this memory properly
    return json_string;
}

const char* esp32_board_get_device_status_json(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    
    cJSON* root = cJSON_CreateObject();
    
    // Audio speaker
    cJSON* audio_speaker = cJSON_CreateObject();
    cJSON_AddNumberToObject(audio_speaker, "volume", 70); // Mock volume
    cJSON_AddItemToObject(root, "audio_speaker", audio_speaker);
    
    // Screen
    cJSON* screen = cJSON_CreateObject();
    cJSON_AddNumberToObject(screen, "brightness", 100); // Mock brightness
    cJSON_AddStringToObject(screen, "theme", "light");
    cJSON_AddItemToObject(root, "screen", screen);
    
    // Network
    cJSON* network = cJSON_CreateObject();
    cJSON_AddStringToObject(network, "type", "wifi");
    if (esp32_board_is_wifi_connected(esp32_self)) {
        cJSON_AddStringToObject(network, "ssid", esp32_self->ssid ? esp32_self->ssid : "");
        if (esp32_self->rssi >= -60) {
            cJSON_AddStringToObject(network, "signal", "strong");
        } else if (esp32_self->rssi >= -70) {
            cJSON_AddStringToObject(network, "signal", "medium");
        } else {
            cJSON_AddStringToObject(network, "signal", "weak");
        }
    }
    cJSON_AddItemToObject(root, "network", network);
    
    // Chip temperature
    float temperature = 25.0f; // Mock temperature
    if (esp32_board_get_temperature(self, &temperature)) {
        cJSON* chip = cJSON_CreateObject();
        cJSON_AddNumberToObject(chip, "temperature", temperature);
        cJSON_AddItemToObject(root, "chip", chip);
    }
    
    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return json_string;
}

const char* esp32_board_get_uuid(Board* self) {
    return self->uuid_;
}

void* esp32_board_get_backlight(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->backlight_instance;
}

void* esp32_board_get_led(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->led_instance;
}

bool esp32_board_get_temperature(Board* self, float* temperature) {
    if (!temperature) return false;
    
    // TODO: Read actual chip temperature
    *temperature = 25.0f; // Mock temperature
    return true;
}

void* esp32_board_get_display(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->display_instance;
}

void* esp32_board_get_camera(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    return esp32_self->camera_instance;
}

bool esp32_board_get_battery_level(Board* self, int* level, bool* charging, bool* discharging) {
    // ESP32 development boards typically don't have battery
    return false;
}

const char* esp32_board_get_system_info_json(Board* self) {
    // Use the default implementation from the base board
    // TODO: Add ESP32-specific system information
    return NULL; // This will trigger the default implementation
}

void esp32_board_destroy_impl(Board* self) {
    ESP32Board* esp32_self = (ESP32Board*)self;
    esp32_board_destroy(esp32_self);
}

// Board factory function for the DECLARE_BOARD macro
Board* create_board(void) {
    return (Board*)esp32_board_create();
}

DECLARE_BOARD(esp32_board_create)