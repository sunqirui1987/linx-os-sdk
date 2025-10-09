#include "system_info.h"
#include "log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define TAG "SystemInfo"

// Application information (can be overridden by build system)
#ifndef APP_NAME
#define APP_NAME "LinxOS"
#endif

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

// Forward declaration for external create_system_info function
extern SystemInfo* create_system_info(void);

// Extended system info data structure
typedef struct {
    uint64_t boot_time_ms;
} SystemInfoData;

// Singleton instance
static SystemInfo* g_system_info_instance = NULL;

/**
 * Initialize boot time (should be called early in main)
 */
static void init_boot_time(SystemInfoData* data) {
    if (data->boot_time_ms == 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        data->boot_time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
}

// Default implementations for virtual functions

static uint32_t system_info_default_get_flash_size(SystemInfo* self) {
    // For non-ESP platforms, return a default value or read from system
    return 4 * 1024 * 1024; // 4MB default
}

static uint32_t system_info_default_get_minimum_free_heap_size(SystemInfo* self) {
    // For non-ESP platforms, this is harder to determine
    return self->vtable->get_free_heap_size(self);
}

static uint32_t system_info_default_get_free_heap_size(SystemInfo* self) {
    // For POSIX systems, we can't easily get heap size
    // Return a reasonable estimate
    return 1024 * 1024; // 1MB estimate
}

static bool system_info_default_get_mac_address(SystemInfo* self, char* mac_str, size_t size) {
    if (!mac_str || size < 18) {
        return false;
    }
    
    // For non-ESP platforms, generate a dummy MAC or read from system
    snprintf(mac_str, size, "00:11:22:33:44:55");
    return true;
}

static const char* system_info_default_get_chip_model_name(SystemInfo* self) {
    return "Generic";
}

static uint32_t system_info_default_get_chip_revision(SystemInfo* self) {
    return 0;
}

static uint32_t system_info_default_get_cpu_cores(SystemInfo* self) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

static uint32_t system_info_default_get_chip_features(SystemInfo* self) {
    return 0;
}

static uint32_t system_info_default_get_cpu_freq_hz(SystemInfo* self) {
    // For non-ESP platforms, return a default frequency
    return 240000000; // 240MHz default
}

static bool system_info_default_get_temperature(SystemInfo* self, float* temperature) {
    if (!temperature) {
        return false;
    }
    
    // For non-ESP platforms, temperature might not be available
    *temperature = 25.0f; // Default room temperature
    return false; // Indicate it's not a real reading
}

static uint32_t system_info_default_get_psram_size(SystemInfo* self) {
    return 0;
}

static uint32_t system_info_default_get_free_psram_size(SystemInfo* self) {
    return 0;
}

const char* system_info_default_get_app_name(SystemInfo* self) {
    return APP_NAME;
}

const char* system_info_default_get_app_version(SystemInfo* self) {
    return APP_VERSION;
}

const char* system_info_default_get_app_compile_date(SystemInfo* self) {
    return __DATE__;
}

const char* system_info_default_get_app_compile_time(SystemInfo* self) {
    return __TIME__;
}

static const char* system_info_default_get_idf_version(SystemInfo* self) {
    return "N/A";
}

static bool system_info_default_get_app_elf_sha256(SystemInfo* self, char* sha256_str, size_t size) {
    if (!sha256_str || size < 65) {
        return false;
    }
    
    // For non-ESP platforms, return a dummy hash
    strcpy(sha256_str, "0000000000000000000000000000000000000000000000000000000000000000");
    return true;
}

static uint64_t system_info_default_get_uptime_ms(SystemInfo* self) {
    SystemInfoData* data = (SystemInfoData*)self->data;
    if (!data) {
        return 0;
    }
    
    init_boot_time(data);
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t current_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return current_ms - data->boot_time_ms;
}

static const char* system_info_default_get_reset_reason(SystemInfo* self) {
    return "Unknown";
}

static void system_info_default_reset(SystemInfo* self) {
    // For non-ESP platforms, we can't easily reset
    LINX_LOGW(TAG, "Reset not supported on this platform");
    exit(0);
}

static void system_info_default_destroy(SystemInfo* self) {
    if (!self) {
        return;
    }
    
    LINX_LOGI(TAG, "销毁系统信息实例");
    
    SystemInfoData* data = (SystemInfoData*)self->data;
    if (data) {
        free(data);
    }
    
    free(self);
    LINX_LOGI(TAG, "系统信息实例销毁完成");
}

// Default vtable
static const SystemInfoVTable system_info_default_vtable = {
    .get_flash_size = system_info_default_get_flash_size,
    .get_minimum_free_heap_size = system_info_default_get_minimum_free_heap_size,
    .get_free_heap_size = system_info_default_get_free_heap_size,
    .get_mac_address = system_info_default_get_mac_address,
    .get_chip_model_name = system_info_default_get_chip_model_name,
    .get_chip_revision = system_info_default_get_chip_revision,
    .get_cpu_cores = system_info_default_get_cpu_cores,
    .get_chip_features = system_info_default_get_chip_features,
    .get_cpu_freq_hz = system_info_default_get_cpu_freq_hz,
    .get_temperature = system_info_default_get_temperature,
    .get_psram_size = system_info_default_get_psram_size,
    .get_free_psram_size = system_info_default_get_free_psram_size,
    .get_app_name = system_info_default_get_app_name,
    .get_app_version = system_info_default_get_app_version,
    .get_app_compile_date = system_info_default_get_app_compile_date,
    .get_app_compile_time = system_info_default_get_app_compile_time,
    .get_idf_version = system_info_default_get_idf_version,
    .get_app_elf_sha256 = system_info_default_get_app_elf_sha256,
    .get_uptime_ms = system_info_default_get_uptime_ms,
    .get_reset_reason = system_info_default_get_reset_reason,
    .reset = system_info_default_reset,
    .destroy = system_info_default_destroy
};

// =============================================================================
// Public interface implementation
// =============================================================================

SystemInfo* system_info_create(void) {
    SystemInfo* self = (SystemInfo*)malloc(sizeof(SystemInfo));
    if (!self) {
        LINX_LOGE(TAG, "分配SystemInfo内存失败");
        return NULL;
    }
    
    SystemInfoData* data = (SystemInfoData*)calloc(1, sizeof(SystemInfoData));
    if (!data) {
        LINX_LOGE(TAG, "分配SystemInfoData内存失败");
        free(self);
        return NULL;
    }
    
    // Initialize base structure
    self->vtable = &system_info_default_vtable;
    self->data = data;
    
    LINX_LOGI(TAG, "系统信息实例创建成功");
    return self;
}

void system_info_destroy(SystemInfo* self) {
    if (!self || !self->vtable || !self->vtable->destroy) {
        return;
    }
    
    self->vtable->destroy(self);
}

SystemInfo* system_info_get_instance(void) {
    if (!g_system_info_instance) {
        // Try to create platform-specific instance first
        g_system_info_instance = create_system_info();
        
        // If platform-specific creation fails, create default instance
        if (!g_system_info_instance) {
            g_system_info_instance = system_info_create();
        }
    }
    
    return g_system_info_instance;
}

// Public methods - these call the vtable functions
uint32_t system_info_get_flash_size(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_flash_size) {
        return 0;
    }
    return instance->vtable->get_flash_size(instance);
}

uint32_t system_info_get_minimum_free_heap_size(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_minimum_free_heap_size) {
        return 0;
    }
    return instance->vtable->get_minimum_free_heap_size(instance);
}

uint32_t system_info_get_free_heap_size(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_free_heap_size) {
        return 0;
    }
    return instance->vtable->get_free_heap_size(instance);
}

bool system_info_get_mac_address(char* mac_str, size_t size) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_mac_address) {
        return false;
    }
    return instance->vtable->get_mac_address(instance, mac_str, size);
}

const char* system_info_get_chip_model_name(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_chip_model_name) {
        return "Unknown";
    }
    return instance->vtable->get_chip_model_name(instance);
}

uint32_t system_info_get_chip_revision(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_chip_revision) {
        return 0;
    }
    return instance->vtable->get_chip_revision(instance);
}

uint32_t system_info_get_cpu_cores(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_cpu_cores) {
        return 1;
    }
    return instance->vtable->get_cpu_cores(instance);
}

uint32_t system_info_get_chip_features(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_chip_features) {
        return 0;
    }
    return instance->vtable->get_chip_features(instance);
}

uint32_t system_info_get_cpu_freq_hz(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_cpu_freq_hz) {
        return 0;
    }
    return instance->vtable->get_cpu_freq_hz(instance);
}

bool system_info_get_temperature(float* temperature) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_temperature) {
        return false;
    }
    return instance->vtable->get_temperature(instance, temperature);
}

uint32_t system_info_get_psram_size(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_psram_size) {
        return 0;
    }
    return instance->vtable->get_psram_size(instance);
}

uint32_t system_info_get_free_psram_size(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_free_psram_size) {
        return 0;
    }
    return instance->vtable->get_free_psram_size(instance);
}

const char* system_info_get_app_name(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_app_name) {
        return "Unknown";
    }
    return instance->vtable->get_app_name(instance);
}

const char* system_info_get_app_version(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_app_version) {
        return "Unknown";
    }
    return instance->vtable->get_app_version(instance);
}

const char* system_info_get_app_compile_date(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_app_compile_date) {
        return "Unknown";
    }
    return instance->vtable->get_app_compile_date(instance);
}

const char* system_info_get_app_compile_time(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_app_compile_time) {
        return "Unknown";
    }
    return instance->vtable->get_app_compile_time(instance);
}

const char* system_info_get_idf_version(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_idf_version) {
        return "Unknown";
    }
    return instance->vtable->get_idf_version(instance);
}

bool system_info_get_app_elf_sha256(char* sha256_str, size_t size) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_app_elf_sha256) {
        return false;
    }
    return instance->vtable->get_app_elf_sha256(instance, sha256_str, size);
}

uint64_t system_info_get_uptime_ms(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_uptime_ms) {
        return 0;
    }
    return instance->vtable->get_uptime_ms(instance);
}

const char* system_info_get_reset_reason(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->get_reset_reason) {
        return "Unknown";
    }
    return instance->vtable->get_reset_reason(instance);
}

void system_info_reset(void) {
    SystemInfo* instance = system_info_get_instance();
    if (!instance || !instance->vtable || !instance->vtable->reset) {
        return;
    }
    instance->vtable->reset(instance);
}