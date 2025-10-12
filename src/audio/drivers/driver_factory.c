/**
 * @file driver_factory.c
 * @brief 音频驱动工厂实现
 * @details 提供跨平台的音频驱动创建和管理功能
 */

#include "audio_driver.h"
#include "../core/types.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 平台特定驱动声明
// ============================================================================

#ifdef __APPLE__
extern linx_audio_driver_t* linx_coreaudio_driver_create(void);
#endif

#ifdef __linux__
extern linx_audio_driver_t* linx_alsa_driver_create(void);
#endif

#ifdef LINX_PLATFORM_ESP32
extern linx_audio_driver_t* linx_esp32_driver_create(void);
#endif

extern linx_audio_driver_t* linx_dummy_driver_create(void);

// ============================================================================
// 驱动类型信息
// ============================================================================

typedef struct {
    linx_audio_driver_type_t type;
    const char* name;
    const char* description;
    bool available;
    linx_audio_driver_t* (*create_func)(void);
} driver_type_info_t;

static const driver_type_info_t driver_types[] = {
#ifdef LINX_PLATFORM_MACOS
    {
        .type = LINX_AUDIO_DRIVER_TYPE_COREAUDIO,
        .name = "coreaudio",
        .description = "CoreAudio driver for macOS",
        .available = true,
        .create_func = linx_coreaudio_driver_create
    },
#endif

#ifdef LINX_PLATFORM_LINUX
    {
        .type = LINX_AUDIO_DRIVER_TYPE_ALSA,
        .name = "alsa",
        .description = "ALSA driver for Linux",
        .available = true,
        .create_func = linx_alsa_driver_create
    },
#endif

#ifdef LINX_PLATFORM_ESP32
    {
        .type = LINX_AUDIO_DRIVER_TYPE_ESP32,
        .name = "esp32",
        .description = "ESP32 I2S audio driver",
        .available = true,
        .create_func = linx_esp32_driver_create
    },
#endif

    {
        .type = LINX_AUDIO_DRIVER_TYPE_DUMMY,
        .name = "dummy",
        .description = "Dummy driver for testing",
        .available = true,
        .create_func = linx_dummy_driver_create
    }
};

static const size_t driver_type_count = sizeof(driver_types) / sizeof(driver_types[0]);

// ============================================================================
// 内部函数
// ============================================================================

static const driver_type_info_t* find_driver_type_info(linx_audio_driver_type_t type) {
    for (size_t i = 0; i < driver_type_count; i++) {
        if (driver_types[i].type == type) {
            return &driver_types[i];
        }
    }
    return NULL;
}

static const driver_type_info_t* find_driver_type_info_by_name(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (size_t i = 0; i < driver_type_count; i++) {
        if (strcmp(driver_types[i].name, name) == 0) {
            return &driver_types[i];
        }
    }
    return NULL;
}

// ============================================================================
// 公共接口实现
// ============================================================================

linx_audio_driver_t* linx_audio_driver_create(linx_audio_driver_type_t type) {
    const driver_type_info_t* info = find_driver_type_info(type);
    if (!info || !info->available || !info->create_func) {
        return NULL;
    }
    
    return info->create_func();
}

void linx_audio_driver_destroy(linx_audio_driver_t* driver) {
    if (!driver) {
        return;
    }
    
    // 反初始化驱动
    if (driver->vtable && driver->vtable->deinitialize) {
        driver->vtable->deinitialize(driver);
    }
    
    // 释放私有数据
    if (driver->private_data) {
        free(driver->private_data);
    }
    
    // 释放驱动结构
    free(driver);
}

const char* linx_audio_driver_type_to_string(linx_audio_driver_type_t type) {
    const driver_type_info_t* info = find_driver_type_info(type);
    return info ? info->name : "unknown";
}

linx_audio_driver_type_t linx_audio_driver_type_from_string(const char* type_str) {
    const driver_type_info_t* info = find_driver_type_info_by_name(type_str);
    return info ? info->type : LINX_AUDIO_DRIVER_TYPE_UNKNOWN;
}

size_t linx_audio_driver_get_available_types(linx_audio_driver_type_t* types, size_t max_count) {
    if (!types || max_count == 0) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < driver_type_count && count < max_count; i++) {
        if (driver_types[i].available) {
            types[count++] = driver_types[i].type;
        }
    }
    
    return count;
}

// ============================================================================
// 平台特定的默认驱动选择
// ============================================================================

linx_audio_driver_t* linx_audio_driver_create_default(void) {
    // 根据平台选择默认驱动
#ifdef __APPLE__
    return linx_audio_driver_create(LINX_AUDIO_DRIVER_TYPE_COREAUDIO);
#elif defined(__linux__)
    return linx_audio_driver_create(LINX_AUDIO_DRIVER_TYPE_ALSA);
#elif defined(LINX_PLATFORM_ESP32)
    return linx_audio_driver_create(LINX_AUDIO_DRIVER_TYPE_ESP32);
#else
    // 其他平台使用虚拟驱动
    return linx_audio_driver_create(LINX_AUDIO_DRIVER_TYPE_DUMMY);
#endif
}

// ============================================================================
// 驱动信息查询
// ============================================================================

const char* linx_audio_driver_get_description(linx_audio_driver_type_t type) {
    const driver_type_info_t* info = find_driver_type_info(type);
    return info ? info->description : "Unknown driver type";
}

bool linx_audio_driver_is_available(linx_audio_driver_type_t type) {
    const driver_type_info_t* info = find_driver_type_info(type);
    return info ? info->available : false;
}

size_t linx_audio_driver_get_all_types(linx_audio_driver_type_t* types, 
                                       const char** names,
                                       const char** descriptions,
                                       bool* available_flags,
                                       size_t max_count) {
    if (max_count == 0) {
        return driver_type_count;
    }
    
    size_t count = driver_type_count < max_count ? driver_type_count : max_count;
    
    for (size_t i = 0; i < count; i++) {
        if (types) {
            types[i] = driver_types[i].type;
        }
        if (names) {
            names[i] = driver_types[i].name;
        }
        if (descriptions) {
            descriptions[i] = driver_types[i].description;
        }
        if (available_flags) {
            available_flags[i] = driver_types[i].available;
        }
    }
    
    return count;
}

// ============================================================================
// 驱动配置辅助函数
// ============================================================================

linx_audio_result_t linx_audio_driver_get_default_config(linx_audio_driver_config_t* config) {
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    memset(config, 0, sizeof(linx_audio_driver_config_t));
    
    // 设置默认值
    config->type = LINX_AUDIO_DRIVER_TYPE_UNKNOWN;
    config->input_device_id = 0;  // 默认设备
    config->output_device_id = 0; // 默认设备
    
    // 默认音频格式
    config->format.sample_rate = 44100;
    config->format.channels = 2;
    config->format.format = LINX_AUDIO_FORMAT_FLOAT32;
    config->format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    
    // 默认缓冲区配置
    config->buffer_size = 512;
    config->buffer_count = 4;
    
    // 默认性能配置
    config->thread_priority = LINX_AUDIO_THREAD_PRIORITY_HIGH;
    config->use_exclusive_mode = false;
    config->enable_input = false;
    config->enable_output = true;
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_driver_validate_config(const linx_audio_driver_config_t* config) {
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 检查驱动类型
    if (!linx_audio_driver_is_available(config->type)) {
        return LINX_AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    // 检查音频格式
    if (config->format.sample_rate < 8000 || config->format.sample_rate > 192000) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (config->format.channels == 0 || config->format.channels > 32) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 检查缓冲区配置
    if (config->buffer_size == 0 || config->buffer_size > 8192) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (config->buffer_count == 0 || config->buffer_count > 16) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 检查至少启用了输入或输出
    if (!config->enable_input && !config->enable_output) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 驱动管理器
// ============================================================================

typedef struct {
    linx_audio_driver_t** drivers;
    size_t driver_count;
    size_t driver_capacity;
    pthread_mutex_t mutex;
} driver_manager_t;

static driver_manager_t g_driver_manager = {0};
static bool g_driver_manager_initialized = false;

linx_audio_result_t linx_audio_driver_manager_initialize(void) {
    if (g_driver_manager_initialized) {
        return LINX_AUDIO_SUCCESS;
    }
    
    memset(&g_driver_manager, 0, sizeof(driver_manager_t));
    
    if (pthread_mutex_init(&g_driver_manager.mutex, NULL) != 0) {
        return LINX_AUDIO_ERROR_THREAD_ERROR;
    }
    
    g_driver_manager.driver_capacity = 16;
    g_driver_manager.drivers = malloc(g_driver_manager.driver_capacity * sizeof(linx_audio_driver_t*));
    if (!g_driver_manager.drivers) {
        pthread_mutex_destroy(&g_driver_manager.mutex);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    g_driver_manager_initialized = true;
    return LINX_AUDIO_SUCCESS;
}

void linx_audio_driver_manager_deinitialize(void) {
    if (!g_driver_manager_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_driver_manager.mutex);
    
    // 销毁所有驱动
    for (size_t i = 0; i < g_driver_manager.driver_count; i++) {
        if (g_driver_manager.drivers[i]) {
            linx_audio_driver_destroy(g_driver_manager.drivers[i]);
        }
    }
    
    free(g_driver_manager.drivers);
    g_driver_manager.drivers = NULL;
    g_driver_manager.driver_count = 0;
    g_driver_manager.driver_capacity = 0;
    
    pthread_mutex_unlock(&g_driver_manager.mutex);
    pthread_mutex_destroy(&g_driver_manager.mutex);
    
    g_driver_manager_initialized = false;
}

linx_audio_result_t linx_audio_driver_manager_register(linx_audio_driver_t* driver) {
    if (!driver || !g_driver_manager_initialized) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_driver_manager.mutex);
    
    // 检查是否需要扩展数组
    if (g_driver_manager.driver_count >= g_driver_manager.driver_capacity) {
        size_t new_capacity = g_driver_manager.driver_capacity * 2;
        linx_audio_driver_t** new_drivers = realloc(g_driver_manager.drivers,
                                                    new_capacity * sizeof(linx_audio_driver_t*));
        if (!new_drivers) {
            pthread_mutex_unlock(&g_driver_manager.mutex);
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        
        g_driver_manager.drivers = new_drivers;
        g_driver_manager.driver_capacity = new_capacity;
    }
    
    g_driver_manager.drivers[g_driver_manager.driver_count++] = driver;
    
    pthread_mutex_unlock(&g_driver_manager.mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_driver_manager_unregister(linx_audio_driver_t* driver) {
    if (!driver || !g_driver_manager_initialized) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_driver_manager.mutex);
    
    // 查找并移除驱动
    for (size_t i = 0; i < g_driver_manager.driver_count; i++) {
        if (g_driver_manager.drivers[i] == driver) {
            // 移动后续元素
            for (size_t j = i; j < g_driver_manager.driver_count - 1; j++) {
                g_driver_manager.drivers[j] = g_driver_manager.drivers[j + 1];
            }
            g_driver_manager.driver_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_driver_manager.mutex);
    
    return LINX_AUDIO_SUCCESS;
}

size_t linx_audio_driver_manager_get_count(void) {
    if (!g_driver_manager_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_driver_manager.mutex);
    size_t count = g_driver_manager.driver_count;
    pthread_mutex_unlock(&g_driver_manager.mutex);
    
    return count;
}

linx_audio_driver_t* linx_audio_driver_manager_get_driver(size_t index) {
    if (!g_driver_manager_initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_driver_manager.mutex);
    
    linx_audio_driver_t* driver = NULL;
    if (index < g_driver_manager.driver_count) {
        driver = g_driver_manager.drivers[index];
    }
    
    pthread_mutex_unlock(&g_driver_manager.mutex);
    
    return driver;
}