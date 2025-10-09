#include "esp32_system_info.h"
#include "common/log.h"

#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <esp_app_desc.h>
#include <esp_timer.h>
#include <driver/temp_sensor.h>
#include <soc/rtc.h>
#include <esp_psram.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* TAG = "ESP32SystemInfo";

// =============================================================================
// ESP32特定的虚函数实现
// =============================================================================

static uint32_t esp32_system_info_get_flash_size(SystemInfo* self) {
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    return flash_size;
}

static uint32_t esp32_system_info_get_minimum_free_heap_size(SystemInfo* self) {
    return esp_get_minimum_free_heap_size();
}

static uint32_t esp32_system_info_get_free_heap_size(SystemInfo* self) {
    return esp_get_free_heap_size();
}

static bool esp32_system_info_get_mac_address(SystemInfo* self, char* mac_str, size_t size) {
    if (!mac_str || size < 18) {
        return false;
    }

    uint8_t mac[6];
    esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (ret != ESP_OK) {
        LINX_LOGE(TAG, "Failed to read MAC address");
        return false;
    }
    
    snprintf(mac_str, size, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return true;
}

static const char* esp32_system_info_get_chip_model_name(SystemInfo* self) {
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
}

static uint32_t esp32_system_info_get_chip_revision(SystemInfo* self) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
}

static uint32_t esp32_system_info_get_cpu_cores(SystemInfo* self) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.cores;
}

static uint32_t esp32_system_info_get_chip_features(SystemInfo* self) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.features;
}

static uint32_t esp32_system_info_get_cpu_freq_hz(SystemInfo* self) {
    rtc_cpu_freq_config_t freq_config;
    rtc_clk_cpu_freq_get_config(&freq_config);
    return freq_config.freq_mhz * 1000000;
}

static bool esp32_system_info_get_temperature(SystemInfo* self, float* temperature) {
    if (!temperature) {
        return false;
    }

    // ESP32 temperature sensor implementation
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_install(&temp_sensor);
    temp_sensor_start();
    
    esp_err_t ret = temp_sensor_read_celsius(temperature);
    
    temp_sensor_stop();
    temp_sensor_uninstall();
    
    return (ret == ESP_OK);
}

static uint32_t esp32_system_info_get_psram_size(SystemInfo* self) {
    return esp_psram_get_size();
}

static uint32_t esp32_system_info_get_free_psram_size(SystemInfo* self) {
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

static const char* esp32_system_info_get_idf_version(SystemInfo* self) {
    return esp_get_idf_version();
}

static bool esp32_system_info_get_app_elf_sha256(SystemInfo* self, char* sha256_str, size_t size) {
    if (!sha256_str || size < 65) {
        return false;
    }

    const esp_app_desc_t* app_desc = esp_app_get_description();
    if (!app_desc) {
        return false;
    }
    
    for (int i = 0; i < 32; i++) {
        sprintf(&sha256_str[i * 2], "%02x", app_desc->app_elf_sha256[i]);
    }
    sha256_str[64] = '\0';
    
    return true;
}

static uint64_t esp32_system_info_get_uptime_ms(SystemInfo* self) {
    return esp_timer_get_time() / 1000;
}

static const char* esp32_system_info_get_reset_reason(SystemInfo* self) {
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
}

static void esp32_system_info_reset(SystemInfo* self) {
    esp_restart();
}

static void esp32_system_info_destroy(SystemInfo* self) {
    if (!self) {
        return;
    }
    
    LINX_LOGI(TAG, "销毁ESP32系统信息实例");
    
    SystemInfoData* data = (SystemInfoData*)self->data;
    if (data) {
        free(data);
    }
    
    free(self);
    LINX_LOGI(TAG, "ESP32系统信息实例销毁完成");
}

// ESP32特定的虚函数表
static const SystemInfoVTable esp32_system_info_vtable = {
    .get_flash_size = esp32_system_info_get_flash_size,
    .get_minimum_free_heap_size = esp32_system_info_get_minimum_free_heap_size,
    .get_free_heap_size = esp32_system_info_get_free_heap_size,
    .get_mac_address = esp32_system_info_get_mac_address,
    .get_chip_model_name = esp32_system_info_get_chip_model_name,
    .get_chip_revision = esp32_system_info_get_chip_revision,
    .get_cpu_cores = esp32_system_info_get_cpu_cores,
    .get_chip_features = esp32_system_info_get_chip_features,
    .get_cpu_freq_hz = esp32_system_info_get_cpu_freq_hz,
    .get_temperature = esp32_system_info_get_temperature,
    .get_psram_size = esp32_system_info_get_psram_size,
    .get_free_psram_size = esp32_system_info_get_free_psram_size,
    .get_app_name = system_info_default_get_app_name,  // 使用默认实现
    .get_app_version = system_info_default_get_app_version,  // 使用默认实现
    .get_app_compile_date = system_info_default_get_app_compile_date,  // 使用默认实现
    .get_app_compile_time = system_info_default_get_app_compile_time,  // 使用默认实现
    .get_idf_version = esp32_system_info_get_idf_version,
    .get_app_elf_sha256 = esp32_system_info_get_app_elf_sha256,
    .get_uptime_ms = esp32_system_info_get_uptime_ms,
    .get_reset_reason = esp32_system_info_get_reset_reason,
    .reset = esp32_system_info_reset,
    .destroy = esp32_system_info_destroy
};

// =============================================================================
// 公共接口实现
// =============================================================================

ESP32SystemInfo* esp32_system_info_create(void) {
    ESP32SystemInfo* self = (ESP32SystemInfo*)malloc(sizeof(ESP32SystemInfo));
    if (!self) {
        LINX_LOGE(TAG, "分配ESP32SystemInfo内存失败");
        return NULL;
    }
    
    SystemInfoData* data = (SystemInfoData*)calloc(1, sizeof(SystemInfoData));
    if (!data) {
        LINX_LOGE(TAG, "分配SystemInfoData内存失败");
        free(self);
        return NULL;
    }
    
    // 初始化基类
    self->base.vtable = &esp32_system_info_vtable;
    self->base.data = data;
    
    LINX_LOGI(TAG, "ESP32系统信息实例创建成功");
    return self;
}

void esp32_system_info_destroy(ESP32SystemInfo* self) {
    if (!self) {
        return;
    }
    
    system_info_destroy((SystemInfo*)self);
}

// =============================================================================
// 平台特定的创建函数 - 供system_info.c调用
// =============================================================================

SystemInfo* create_system_info(void) {
    ESP32SystemInfo* esp32_instance = esp32_system_info_create();
    if (!esp32_instance) {
        return NULL;
    }
    
    return (SystemInfo*)esp32_instance;
}