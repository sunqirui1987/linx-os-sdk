/**
 * @file dummy_driver.c
 * @brief 虚拟音频驱动实现
 * @details 用于测试和开发的虚拟音频驱动
 */

#include "audio_driver.h"
#include "../core/types.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// ============================================================================
// 内部数据结构
// ============================================================================

typedef struct {
    uint32_t device_id;
    char name[256];
    linx_audio_device_type_t type;
    linx_audio_format_info_t format;
    bool is_default;
} dummy_device_t;

typedef struct {
    linx_audio_driver_t base;
    
    // 设备列表
    dummy_device_t* devices;
    size_t device_count;
    
    // 驱动状态
    linx_audio_driver_state_t state;
    linx_audio_driver_config_t config;
    
    // 音频处理线程
    pthread_t audio_thread;
    bool thread_running;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    
    // 回调函数
    linx_audio_input_callback_t input_callback;
    linx_audio_output_callback_t output_callback;
    void* callback_user_data;
    
    // 统计信息
    linx_audio_driver_stats_t stats;
    
    // 音频缓冲区
    float* input_buffer;
    float* output_buffer;
    size_t buffer_frames;
    
} dummy_driver_private_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t dummy_initialize(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config);
static linx_audio_result_t dummy_deinitialize(linx_audio_driver_t* driver);
static linx_audio_result_t dummy_start(linx_audio_driver_t* driver);
static linx_audio_result_t dummy_stop(linx_audio_driver_t* driver);
static linx_audio_result_t dummy_get_device_count(linx_audio_driver_t* driver, linx_audio_device_type_t type, size_t* count);
static linx_audio_result_t dummy_get_device_info(linx_audio_driver_t* driver, linx_audio_device_type_t type, uint32_t device_id, linx_audio_device_info_t* info);
static linx_audio_result_t dummy_is_format_supported(linx_audio_driver_t* driver, uint32_t device_id, const linx_audio_format_info_t* format, bool* supported);
static linx_audio_result_t dummy_get_state(linx_audio_driver_t* driver, linx_audio_driver_state_t* state);
static linx_audio_result_t dummy_get_stats(linx_audio_driver_t* driver, linx_audio_driver_stats_t* stats);
static linx_audio_result_t dummy_set_input_callback(linx_audio_driver_t* driver, linx_audio_input_callback_t callback, void* user_data);
static linx_audio_result_t dummy_set_output_callback(linx_audio_driver_t* driver, linx_audio_output_callback_t callback, void* user_data);
static linx_audio_result_t dummy_update_config(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config);

static void* dummy_audio_thread(void* arg);
static void dummy_init_devices(dummy_driver_private_t* priv);
static void dummy_cleanup_devices(dummy_driver_private_t* priv);

// ============================================================================
// 虚函数表
// ============================================================================

static const linx_audio_driver_vtable_t dummy_vtable = {
    .initialize = dummy_initialize,
    .deinitialize = dummy_deinitialize,
    .start = dummy_start,
    .stop = dummy_stop,
    .get_device_count = dummy_get_device_count,
    .get_device_info = dummy_get_device_info,
    .is_format_supported = dummy_is_format_supported,
    .get_state = dummy_get_state,
    .get_stats = dummy_get_stats,
    .set_input_callback = dummy_set_input_callback,
    .set_output_callback = dummy_set_output_callback,
    .update_config = dummy_update_config
};

// ============================================================================
// 公共接口实现
// ============================================================================

linx_audio_driver_t* linx_dummy_driver_create(void) {
    dummy_driver_private_t* priv = calloc(1, sizeof(dummy_driver_private_t));
    if (!priv) {
        return NULL;
    }
    
    // 初始化基础结构
    priv->base.type = LINX_AUDIO_DRIVER_TYPE_DUMMY;
    priv->base.vtable = &dummy_vtable;
    priv->base.private_data = priv;
    
    // 初始化状态
    priv->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    priv->thread_running = false;
    
    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&priv->mutex, NULL) != 0) {
        free(priv);
        return NULL;
    }
    
    if (pthread_cond_init(&priv->cond, NULL) != 0) {
        pthread_mutex_destroy(&priv->mutex);
        free(priv);
        return NULL;
    }
    
    // 初始化虚拟设备
    dummy_init_devices(priv);
    
    return &priv->base;
}

// ============================================================================
// 驱动接口实现
// ============================================================================

static linx_audio_result_t dummy_initialize(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config) {
    if (!driver || !config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    // 复制配置
    priv->config = *config;
    
    // 计算缓冲区大小
    priv->buffer_frames = config->buffer_size;
    size_t buffer_size = priv->buffer_frames * config->format.channels * sizeof(float);
    
    // 分配音频缓冲区
    if (config->enable_input) {
        priv->input_buffer = malloc(buffer_size);
        if (!priv->input_buffer) {
            pthread_mutex_unlock(&priv->mutex);
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        memset(priv->input_buffer, 0, buffer_size);
    }
    
    if (config->enable_output) {
        priv->output_buffer = malloc(buffer_size);
        if (!priv->output_buffer) {
            free(priv->input_buffer);
            priv->input_buffer = NULL;
            pthread_mutex_unlock(&priv->mutex);
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        memset(priv->output_buffer, 0, buffer_size);
    }
    
    // 初始化统计信息
    memset(&priv->stats, 0, sizeof(linx_audio_driver_stats_t));
    
    priv->state = LINX_AUDIO_DRIVER_STATE_INITIALIZED;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_deinitialize(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    if (priv->state == LINX_AUDIO_DRIVER_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    // 停止音频处理
    if (priv->state == LINX_AUDIO_DRIVER_STATE_RUNNING) {
        dummy_stop(driver);
    }
    
    // 释放缓冲区
    if (priv->input_buffer) {
        free(priv->input_buffer);
        priv->input_buffer = NULL;
    }
    
    if (priv->output_buffer) {
        free(priv->output_buffer);
        priv->output_buffer = NULL;
    }
    
    priv->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_start(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_INITIALIZED) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    // 启动音频处理线程
    priv->thread_running = true;
    if (pthread_create(&priv->audio_thread, NULL, dummy_audio_thread, priv) != 0) {
        priv->thread_running = false;
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_ERROR_THREAD_ERROR;
    }
    
    priv->state = LINX_AUDIO_DRIVER_STATE_RUNNING;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_stop(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_RUNNING) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    // 停止音频处理线程
    priv->thread_running = false;
    pthread_cond_signal(&priv->cond);
    
    pthread_mutex_unlock(&priv->mutex);
    
    // 等待线程结束
    pthread_join(priv->audio_thread, NULL);
    
    pthread_mutex_lock(&priv->mutex);
    priv->state = LINX_AUDIO_DRIVER_STATE_INITIALIZED;
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_get_device_count(linx_audio_driver_t* driver, linx_audio_device_type_t type, size_t* count) {
    if (!driver || !count) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    size_t device_count = 0;
    for (size_t i = 0; i < priv->device_count; i++) {
        if (priv->devices[i].type == type || type == LINX_AUDIO_DEVICE_TYPE_UNKNOWN) {
            device_count++;
        }
    }
    
    *count = device_count;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_get_device_info(linx_audio_driver_t* driver, linx_audio_device_type_t type, uint32_t device_id, linx_audio_device_info_t* info) {
    if (!driver || !info) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    // 查找设备
    dummy_device_t* device = NULL;
    for (size_t i = 0; i < priv->device_count; i++) {
        if (priv->devices[i].device_id == device_id && 
            (priv->devices[i].type == type || type == LINX_AUDIO_DEVICE_TYPE_UNKNOWN)) {
            device = &priv->devices[i];
            break;
        }
    }
    
    if (!device) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_ERROR_DEVICE_NOT_FOUND;
    }
    
    // 填充设备信息
    memset(info, 0, sizeof(linx_audio_device_info_t));
    info->device_id = device->device_id;
    strncpy(info->name, device->name, sizeof(info->name) - 1);
    info->type = device->type;
    info->is_default = device->is_default;
    info->format = device->format;
    
    // 设置支持的格式范围
    info->min_sample_rate = 8000;
    info->max_sample_rate = 192000;
    info->min_channels = 1;
    info->max_channels = 8;
    
    // 设置支持的格式
    info->supported_formats = LINX_AUDIO_FORMAT_INT16 | 
                             LINX_AUDIO_FORMAT_INT24 | 
                             LINX_AUDIO_FORMAT_INT32 | 
                             LINX_AUDIO_FORMAT_FLOAT32;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_is_format_supported(linx_audio_driver_t* driver, uint32_t device_id, const linx_audio_format_info_t* format, bool* supported) {
    (void)driver;
    (void)device_id;
    
    if (!format || !supported) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 虚拟驱动支持所有常见格式
    *supported = (format->sample_rate >= 8000 && format->sample_rate <= 192000) &&
                 (format->channels >= 1 && format->channels <= 8) &&
                 (format->format == LINX_AUDIO_FORMAT_INT16 ||
                  format->format == LINX_AUDIO_FORMAT_INT24 ||
                  format->format == LINX_AUDIO_FORMAT_INT32 ||
                  format->format == LINX_AUDIO_FORMAT_FLOAT32);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_get_state(linx_audio_driver_t* driver, linx_audio_driver_state_t* state) {
    if (!driver || !state) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    *state = priv->state;
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_get_stats(linx_audio_driver_t* driver, linx_audio_driver_stats_t* stats) {
    if (!driver || !stats) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    *stats = priv->stats;
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_set_input_callback(linx_audio_driver_t* driver, linx_audio_input_callback_t callback, void* user_data) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    priv->input_callback = callback;
    priv->callback_user_data = user_data;
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_set_output_callback(linx_audio_driver_t* driver, linx_audio_output_callback_t callback, void* user_data) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    priv->output_callback = callback;
    priv->callback_user_data = user_data;
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t dummy_update_config(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config) {
    if (!driver || !config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    dummy_driver_private_t* priv = (dummy_driver_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->mutex);
    
    if (priv->state == LINX_AUDIO_DRIVER_STATE_RUNNING) {
        pthread_mutex_unlock(&priv->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    priv->config = *config;
    
    pthread_mutex_unlock(&priv->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 内部实现
// ============================================================================

static void* dummy_audio_thread(void* arg) {
    dummy_driver_private_t* priv = (dummy_driver_private_t*)arg;
    
    while (priv->thread_running) {
        pthread_mutex_lock(&priv->mutex);
        
        if (!priv->thread_running) {
            pthread_mutex_unlock(&priv->mutex);
            break;
        }
        
        // 模拟音频输入
        if (priv->config.enable_input && priv->input_callback && priv->input_buffer) {
            // 生成静音或测试信号
            memset(priv->input_buffer, 0, priv->buffer_frames * priv->config.format.channels * sizeof(float));
            priv->input_callback(priv->input_buffer, priv->buffer_frames, priv->callback_user_data);
        }
        
        // 模拟音频输出
        if (priv->config.enable_output && priv->output_callback && priv->output_buffer) {
            memset(priv->output_buffer, 0, priv->buffer_frames * priv->config.format.channels * sizeof(float));
            priv->output_callback(priv->output_buffer, priv->buffer_frames, priv->callback_user_data);
            // 虚拟驱动不需要实际输出音频
        }
        
        priv->stats.callback_count++;
        
        pthread_mutex_unlock(&priv->mutex);
        
        // 模拟音频缓冲区延迟
        usleep((priv->buffer_frames * 1000000) / priv->config.format.sample_rate);
    }
    
    return NULL;
}

static void dummy_init_devices(dummy_driver_private_t* priv) {
    priv->device_count = 2;
    priv->devices = malloc(priv->device_count * sizeof(dummy_device_t));
    
    if (!priv->devices) {
        priv->device_count = 0;
        return;
    }
    
    // 虚拟输出设备
    priv->devices[0].device_id = 0;
    strcpy(priv->devices[0].name, "Dummy Output Device");
    priv->devices[0].type = LINX_AUDIO_DEVICE_TYPE_OUTPUT;
    priv->devices[0].format.sample_rate = 44100;
    priv->devices[0].format.channels = 2;
    priv->devices[0].format.format = LINX_AUDIO_FORMAT_FLOAT32;
    priv->devices[0].format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    priv->devices[0].is_default = true;
    
    // 虚拟输入设备
    priv->devices[1].device_id = 1;
    strcpy(priv->devices[1].name, "Dummy Input Device");
    priv->devices[1].type = LINX_AUDIO_DEVICE_TYPE_INPUT;
    priv->devices[1].format.sample_rate = 44100;
    priv->devices[1].format.channels = 2;
    priv->devices[1].format.format = LINX_AUDIO_FORMAT_FLOAT32;
    priv->devices[1].format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    priv->devices[1].is_default = true;
}

static void dummy_cleanup_devices(dummy_driver_private_t* priv) {
    if (priv->devices) {
        free(priv->devices);
        priv->devices = NULL;
        priv->device_count = 0;
    }
}