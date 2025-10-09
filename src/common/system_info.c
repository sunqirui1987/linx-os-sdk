#include "system_info.h"
#include "log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef ESP_PLATFORM
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_psram.h"
#include "soc/efuse_reg.h"
#include "driver/temp_sensor.h"
#endif

#define TAG "SystemInfo"

// Application information (can be overridden by build system)
#ifndef APP_NAME
#define APP_NAME "LinxOS"
#endif

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

static uint64_t boot_time_ms = 0;

/**
 * Initialize boot time (should be called early in main)
 */
static void init_boot_time(void) {
    if (boot_time_ms == 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        boot_time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
}

uint32_t system_info_get_flash_size(void) {
#ifdef ESP_PLATFORM
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    return flash_size;
#else
    // For non-ESP platforms, return a default value or read from system
    return 4 * 1024 * 1024; // 4MB default
#endif
}

uint32_t system_info_get_minimum_free_heap_size(void) {
#ifdef ESP_PLATFORM
    return esp_get_minimum_free_heap_size();
#else
    // For non-ESP platforms, this is harder to determine
    return system_info_get_free_heap_size();
#endif
}

uint32_t system_info_get_free_heap_size(void) {
#ifdef ESP_PLATFORM
    return esp_get_free_heap_size();
#else
    // For POSIX systems, we can't easily get heap size
    // Return a reasonable estimate
    return 1024 * 1024; // 1MB estimate
#endif
}

bool system_info_get_mac_address(char* mac_str, size_t size) {
    if (!mac_str || size < 18) {
        return false;
    }

#ifdef ESP_PLATFORM
    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret != ESP_OK) {
        LINX_LOGE(TAG, "Failed to read MAC address");
        return false;
    }
    
    snprintf(mac_str, size, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#else
    // For non-ESP platforms, generate a dummy MAC or read from system
    snprintf(mac_str, size, "00:11:22:33:44:55");
#endif
    
    return true;
}

const char* system_info_get_chip_model_name(void) {
#ifdef ESP_PLATFORM
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    switch (chip_info.model) {
        case CHIP_ESP32:
            return "ESP32";
        case CHIP_ESP32S2:
            return "ESP32-S2";
        case CHIP_ESP32S3:
            return "ESP32-S3";
        case CHIP_ESP32C3:
            return "ESP32-C3";
        case CHIP_ESP32C2:
            return "ESP32-C2";
        case CHIP_ESP32C6:
            return "ESP32-C6";
        case CHIP_ESP32H2:
            return "ESP32-H2";
        default:
            return "Unknown ESP32";
    }
#else
    return "Generic";
#endif
}

uint32_t system_info_get_chip_revision(void) {
#ifdef ESP_PLATFORM
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
#else
    return 0;
#endif
}

uint32_t system_info_get_cpu_cores(void) {
#ifdef ESP_PLATFORM
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.cores;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

uint32_t system_info_get_chip_features(void) {
#ifdef ESP_PLATFORM
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.features;
#else
    return 0;
#endif
}

const char* system_info_get_app_name(void) {
    return APP_NAME;
}

const char* system_info_get_app_version(void) {
    return APP_VERSION;
}

const char* system_info_get_app_compile_date(void) {
    return __DATE__;
}

const char* system_info_get_app_compile_time(void) {
    return __TIME__;
}

const char* system_info_get_idf_version(void) {
#ifdef ESP_PLATFORM
    return esp_get_idf_version();
#else
    return "N/A";
#endif
}

bool system_info_get_app_elf_sha256(char* sha256_str, size_t size) {
    if (!sha256_str || size < 65) {
        return false;
    }

#ifdef ESP_PLATFORM
    const esp_app_desc_t* app_desc = esp_app_get_description();
    if (!app_desc) {
        return false;
    }
    
    for (int i = 0; i < 32; i++) {
        sprintf(&sha256_str[i * 2], "%02x", app_desc->app_elf_sha256[i]);
    }
    sha256_str[64] = '\0';
#else
    // For non-ESP platforms, return a dummy hash
    strcpy(sha256_str, "0000000000000000000000000000000000000000000000000000000000000000");
#endif
    
    return true;
}

uint64_t system_info_get_uptime_ms(void) {
    if (boot_time_ms == 0) {
        init_boot_time();
    }
    
#ifdef ESP_PLATFORM
    return esp_timer_get_time() / 1000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t current_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return current_ms - boot_time_ms;
#endif
}

uint32_t system_info_get_cpu_freq_hz(void) {
#ifdef ESP_PLATFORM
    rtc_cpu_freq_config_t freq_config;
    rtc_clk_cpu_freq_get_config(&freq_config);
    return freq_config.freq_mhz * 1000000;
#else
    // For non-ESP platforms, return a default frequency
    return 240000000; // 240MHz default
#endif
}

bool system_info_get_temperature(float* temperature) {
    if (!temperature) {
        return false;
    }

#ifdef ESP_PLATFORM
    // ESP32 temperature sensor implementation
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_install(&temp_sensor);
    temp_sensor_start();
    
    esp_err_t ret = temp_sensor_read_celsius(temperature);
    
    temp_sensor_stop();
    temp_sensor_uninstall();
    
    return (ret == ESP_OK);
#else
    // For non-ESP platforms, temperature might not be available
    *temperature = 25.0f; // Default room temperature
    return false; // Indicate it's not a real reading
#endif
}

uint32_t system_info_get_psram_size(void) {
#ifdef ESP_PLATFORM
    return esp_psram_get_size();
#else
    return 0;
#endif
}

uint32_t system_info_get_free_psram_size(void) {
#ifdef ESP_PLATFORM
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
    return 0;
#endif
}

void system_info_reset(void) {
#ifdef ESP_PLATFORM
    esp_restart();
#else
    // For non-ESP platforms, we can't easily reset
    LINX_LOGW(TAG, "Reset not supported on this platform");
    exit(0);
#endif
}

const char* system_info_get_reset_reason(void) {
#ifdef ESP_PLATFORM
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch (reason) {
        case ESP_RST_POWERON:
            return "Power-on reset";
        case ESP_RST_EXT:
            return "External reset";
        case ESP_RST_SW:
            return "Software reset";
        case ESP_RST_PANIC:
            return "Exception/panic reset";
        case ESP_RST_INT_WDT:
            return "Interrupt watchdog reset";
        case ESP_RST_TASK_WDT:
            return "Task watchdog reset";
        case ESP_RST_WDT:
            return "Other watchdog reset";
        case ESP_RST_DEEPSLEEP:
            return "Deep sleep reset";
        case ESP_RST_BROWNOUT:
            return "Brownout reset";
        case ESP_RST_SDIO:
            return "SDIO reset";
        default:
            return "Unknown reset";
    }
#else
    return "Unknown";
#endif
}