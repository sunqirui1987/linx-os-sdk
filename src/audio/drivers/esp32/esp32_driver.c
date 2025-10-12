/**
 * @file esp32_driver.c
 * @brief ESP32音频驱动实现
 * @details 基于ESP-IDF的ESP32音频驱动，支持I2S接口
 */

#include "../audio_driver.h"
#include "../../core/types.h"
#include <stdlib.h>
#include <string.h>

#ifdef LINX_PLATFORM_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_err.h"
#endif

// ============================================================================
// 内部数据结构
// ============================================================================

typedef struct {
    uint32_t device_id;
    char name[256];
    linx_audio_device_type_t type;
    linx_audio_format_info_t format;
    bool is_default;
#ifdef LINX_PLATFORM_ESP32
    i2s_port_t i2s_port;
#else
    int i2s_port;
#endif
} esp32_device_t;

typedef struct {
    linx_audio_driver_t base;
    
    // ESP32 I2S配置
#ifdef LINX_PLATFORM_ESP32
    i2s_config_t i2s_config;
    i2s_pin_config_t pin_config;
    i2s_port_t i2s_port;
#else
    void* i2s_config;
    void* pin_config;
    int i2s_port;
#endif
    
    // 设备列表
    esp32_device_t* devices;
    size_t device_count;
    
    // 驱动状态
    linx_audio_driver_state_t state;
    linx_audio_driver_config_t config;
    
    // 音频处理任务
#ifdef LINX_PLATFORM_ESP32
    TaskHandle_t audio_task;
    QueueHandle_t audio_queue;
    SemaphoreHandle_t mutex;
#else
    void* audio_task;
    void* audio_queue;
    void* mutex;
#endif
    bool task_running;
    
    // 回调函数
    linx_audio_input_callback_t input_callback;
    linx_audio_output_callback_t output_callback;
    void* callback_user_data;
    
    // 统计信息
    linx_audio_driver_stats_t stats;
    
    // 音频缓冲区
    float* input_buffer;
    float* output_buffer;
    int16_t* i2s_buffer;  // I2S原始数据缓冲区
    size_t buffer_frames;
    
} esp32_driver_private_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t esp32_initialize(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config);
static linx_audio_result_t esp32_deinitialize(linx_audio_driver_t* driver);
static linx_audio_result_t esp32_start(linx_audio_driver_t* driver);
static linx_audio_result_t esp32_stop(linx_audio_driver_t* driver);
static linx_audio_result_t esp32_get_device_count(linx_audio_driver_t* driver, linx_audio_device_type_t type, size_t* count);
static linx_audio_result_t esp32_get_device_info(linx_audio_driver_t* driver, linx_audio_device_type_t type, uint32_t device_id, linx_audio_device_info_t* info);
static linx_audio_result_t esp32_is_format_supported(linx_audio_driver_t* driver, uint32_t device_id, const linx_audio_format_info_t* format, bool* supported);
static linx_audio_result_t esp32_get_state(linx_audio_driver_t* driver, linx_audio_driver_state_t* state);
static linx_audio_result_t esp32_get_stats(linx_audio_driver_t* driver, linx_audio_driver_stats_t* stats);
static linx_audio_result_t esp32_set_input_callback(linx_audio_driver_t* driver, linx_audio_input_callback_t callback, void* user_data);
static linx_audio_result_t esp32_set_output_callback(linx_audio_driver_t* driver, linx_audio_output_callback_t callback, void* user_data);
static linx_audio_result_t esp32_update_config(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config);

#ifdef LINX_PLATFORM_ESP32
static void esp32_audio_task(void* arg);
static esp_err_t esp32_setup_i2s(esp32_driver_private_t* priv);
static void esp32_cleanup_i2s(esp32_driver_private_t* priv);
static void esp32_convert_float_to_i2s(const float* input, int16_t* output, size_t frames, uint32_t channels);
static void esp32_convert_i2s_to_float(const int16_t* input, float* output, size_t frames, uint32_t channels);
#endif

static void esp32_init_devices(esp32_driver_private_t* priv);
static void esp32_cleanup_devices(esp32_driver_private_t* priv);

// ============================================================================
// 常量定义
// ============================================================================

#ifdef LINX_PLATFORM_ESP32
static const char* TAG = "ESP32_AUDIO";

// 默认I2S引脚配置
#define ESP32_I2S_BCK_PIN    26
#define ESP32_I2S_WS_PIN     25
#define ESP32_I2S_DATA_PIN   22
#define ESP32_I2S_PORT       I2S_NUM_0

// 音频任务配置
#define ESP32_AUDIO_TASK_STACK_SIZE  4096
#define ESP32_AUDIO_TASK_PRIORITY    5
#define ESP32_AUDIO_QUEUE_SIZE       10
#endif

// ============================================================================
// 虚函数表
// ============================================================================

static const linx_audio_driver_vtable_t esp32_vtable = {
    .initialize = esp32_initialize,
    .deinitialize = esp32_deinitialize,
    .start = esp32_start,
    .stop = esp32_stop,
    .get_device_count = esp32_get_device_count,
    .get_device_info = esp32_get_device_info,
    .is_format_supported = esp32_is_format_supported,
    .get_state = esp32_get_state,
    .get_stats = esp32_get_stats,
    .set_input_callback = esp32_set_input_callback,
    .set_output_callback = esp32_set_output_callback,
    .update_config = esp32_update_config
};

// ============================================================================
// 公共接口实现
// ============================================================================

linx_audio_driver_t* linx_esp32_driver_create(void) {
#ifndef LINX_PLATFORM_ESP32
    // 如果不是ESP32平台，返回NULL
    return NULL;
#endif

    esp32_driver_private_t* priv = calloc(1, sizeof(esp32_driver_private_t));
    if (!priv) {
        return NULL;
    }
    
    // 初始化基础结构
    priv->base.type = LINX_AUDIO_DRIVER_TYPE_ESP32;
    priv->base.vtable = &esp32_vtable;
    priv->base.private_data = priv;
    
    // 初始化状态
    priv->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    priv->task_running = false;
    
#ifdef LINX_PLATFORM_ESP32
    // 创建互斥锁
    priv->mutex = xSemaphoreCreateMutex();
    if (!priv->mutex) {
        free(priv);
        return NULL;
    }
    
    // 创建音频队列
    priv->audio_queue = xQueueCreate(ESP32_AUDIO_QUEUE_SIZE, sizeof(uint32_t));
    if (!priv->audio_queue) {
        vSemaphoreDelete(priv->mutex);
        free(priv);
        return NULL;
    }
    
    // 设置I2S端口
    priv->i2s_port = ESP32_I2S_PORT;
#endif
    
    // 初始化设备列表
    esp32_init_devices(priv);
    
    return &priv->base;
}

// ============================================================================
// 驱动接口实现
// ============================================================================

static linx_audio_result_t esp32_initialize(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config) {
    if (!driver || !config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_UNINITIALIZED) {
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    // 复制配置
    priv->config = *config;
    
    // 计算缓冲区大小
    priv->buffer_frames = config->buffer_size;
    size_t float_buffer_size = priv->buffer_frames * config->format.channels * sizeof(float);
    size_t i2s_buffer_size = priv->buffer_frames * config->format.channels * sizeof(int16_t);
    
    // 分配音频缓冲区
    if (config->enable_input) {
        priv->input_buffer = malloc(float_buffer_size);
        if (!priv->input_buffer) {
#ifdef LINX_PLATFORM_ESP32
            xSemaphoreGive(priv->mutex);
#endif
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        memset(priv->input_buffer, 0, float_buffer_size);
    }
    
    if (config->enable_output) {
        priv->output_buffer = malloc(float_buffer_size);
        if (!priv->output_buffer) {
            free(priv->input_buffer);
            priv->input_buffer = NULL;
#ifdef LINX_PLATFORM_ESP32
            xSemaphoreGive(priv->mutex);
#endif
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        memset(priv->output_buffer, 0, float_buffer_size);
    }
    
    // 分配I2S缓冲区
    priv->i2s_buffer = malloc(i2s_buffer_size);
    if (!priv->i2s_buffer) {
        free(priv->input_buffer);
        free(priv->output_buffer);
        priv->input_buffer = NULL;
        priv->output_buffer = NULL;
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    memset(priv->i2s_buffer, 0, i2s_buffer_size);
    
#ifdef LINX_PLATFORM_ESP32
    // 设置I2S
    esp_err_t err = esp32_setup_i2s(priv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to setup I2S: %s", esp_err_to_name(err));
        free(priv->input_buffer);
        free(priv->output_buffer);
        free(priv->i2s_buffer);
        priv->input_buffer = NULL;
        priv->output_buffer = NULL;
        priv->i2s_buffer = NULL;
        xSemaphoreGive(priv->mutex);
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
#endif
    
    // 初始化统计信息
    memset(&priv->stats, 0, sizeof(linx_audio_driver_stats_t));
    
    priv->state = LINX_AUDIO_DRIVER_STATE_INITIALIZED;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_deinitialize(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    if (priv->state == LINX_AUDIO_DRIVER_STATE_UNINITIALIZED) {
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_SUCCESS;
    }
    
    // 停止音频处理
    if (priv->state == LINX_AUDIO_DRIVER_STATE_RUNNING) {
        esp32_stop(driver);
    }
    
#ifdef LINX_PLATFORM_ESP32
    // 清理I2S
    esp32_cleanup_i2s(priv);
#endif
    
    // 释放缓冲区
    if (priv->input_buffer) {
        free(priv->input_buffer);
        priv->input_buffer = NULL;
    }
    
    if (priv->output_buffer) {
        free(priv->output_buffer);
        priv->output_buffer = NULL;
    }
    
    if (priv->i2s_buffer) {
        free(priv->i2s_buffer);
        priv->i2s_buffer = NULL;
    }
    
    priv->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_start(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_INITIALIZED) {
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
#ifdef LINX_PLATFORM_ESP32
    // 启动I2S
    esp_err_t err = i2s_start(priv->i2s_port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start I2S: %s", esp_err_to_name(err));
        xSemaphoreGive(priv->mutex);
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    // 创建音频处理任务
    priv->task_running = true;
    BaseType_t result = xTaskCreate(
        esp32_audio_task,
        "audio_task",
        ESP32_AUDIO_TASK_STACK_SIZE,
        priv,
        ESP32_AUDIO_TASK_PRIORITY,
        &priv->audio_task
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        priv->task_running = false;
        i2s_stop(priv->i2s_port);
        xSemaphoreGive(priv->mutex);
        return LINX_AUDIO_ERROR_THREAD_ERROR;
    }
#endif
    
    priv->state = LINX_AUDIO_DRIVER_STATE_RUNNING;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_stop(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    if (priv->state != LINX_AUDIO_DRIVER_STATE_RUNNING) {
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_SUCCESS;
    }
    
#ifdef LINX_PLATFORM_ESP32
    // 停止音频处理任务
    priv->task_running = false;
    
    // 发送停止信号
    uint32_t stop_signal = 0;
    xQueueSend(priv->audio_queue, &stop_signal, 0);
    
    xSemaphoreGive(priv->mutex);
    
    // 等待任务结束
    if (priv->audio_task) {
        vTaskDelete(priv->audio_task);
        priv->audio_task = NULL;
    }
    
    // 停止I2S
    i2s_stop(priv->i2s_port);
    
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    priv->state = LINX_AUDIO_DRIVER_STATE_INITIALIZED;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

// 其他接口实现与之前的驱动类似
static linx_audio_result_t esp32_get_device_count(linx_audio_driver_t* driver, linx_audio_device_type_t type, size_t* count) {
    if (!driver || !count) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
    size_t device_count = 0;
    for (size_t i = 0; i < priv->device_count; i++) {
        if (priv->devices[i].type == type || type == LINX_AUDIO_DEVICE_TYPE_UNKNOWN) {
            device_count++;
        }
    }
    
    *count = device_count;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_get_device_info(linx_audio_driver_t* driver, linx_audio_device_type_t type, uint32_t device_id, linx_audio_device_info_t* info) {
    if (!driver || !info) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
    // 查找设备
    esp32_device_t* device = NULL;
    for (size_t i = 0; i < priv->device_count; i++) {
        if (priv->devices[i].device_id == device_id && 
            (priv->devices[i].type == type || type == LINX_AUDIO_DEVICE_TYPE_UNKNOWN)) {
            device = &priv->devices[i];
            break;
        }
    }
    
    if (!device) {
        return LINX_AUDIO_ERROR_DEVICE_NOT_FOUND;
    }
    
    // 填充设备信息
    memset(info, 0, sizeof(linx_audio_device_info_t));
    info->device_id = device->device_id;
    strncpy(info->name, device->name, sizeof(info->name) - 1);
    info->type = device->type;
    info->is_default = device->is_default;
    info->format = device->format;
    
    // ESP32 I2S支持的格式范围
    info->min_sample_rate = 8000;
    info->max_sample_rate = 48000;
    info->min_channels = 1;
    info->max_channels = 2;
    
    // ESP32主要支持16位整数格式
    info->supported_formats = LINX_AUDIO_FORMAT_INT16;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_is_format_supported(linx_audio_driver_t* driver, uint32_t device_id, const linx_audio_format_info_t* format, bool* supported) {
    (void)driver;
    (void)device_id;
    
    if (!format || !supported) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // ESP32 I2S支持的格式限制
    *supported = (format->sample_rate >= 8000 && format->sample_rate <= 48000) &&
                 (format->channels >= 1 && format->channels <= 2) &&
                 (format->format == LINX_AUDIO_FORMAT_INT16);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_get_state(linx_audio_driver_t* driver, linx_audio_driver_state_t* state) {
    if (!driver || !state) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    *state = priv->state;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_get_stats(linx_audio_driver_t* driver, linx_audio_driver_stats_t* stats) {
    if (!driver || !stats) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    *stats = priv->stats;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_set_input_callback(linx_audio_driver_t* driver, linx_audio_input_callback_t callback, void* user_data) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    priv->input_callback = callback;
    priv->callback_user_data = user_data;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_set_output_callback(linx_audio_driver_t* driver, linx_audio_output_callback_t callback, void* user_data) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    priv->output_callback = callback;
    priv->callback_user_data = user_data;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t esp32_update_config(linx_audio_driver_t* driver, const linx_audio_driver_config_t* config) {
    if (!driver || !config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    esp32_driver_private_t* priv = (esp32_driver_private_t*)driver->private_data;
    
#ifdef LINX_PLATFORM_ESP32
    if (xSemaphoreTake(priv->mutex, portMAX_DELAY) != pdTRUE) {
        return LINX_AUDIO_ERROR_TIMEOUT;
    }
#endif
    
    if (priv->state == LINX_AUDIO_DRIVER_STATE_RUNNING) {
#ifdef LINX_PLATFORM_ESP32
        xSemaphoreGive(priv->mutex);
#endif
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    priv->config = *config;
    
#ifdef LINX_PLATFORM_ESP32
    xSemaphoreGive(priv->mutex);
#endif
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// ESP32特定实现
// ============================================================================

#ifdef LINX_PLATFORM_ESP32

static void esp32_audio_task(void* arg) {
    esp32_driver_private_t* priv = (esp32_driver_private_t*)arg;
    uint32_t queue_data;
    size_t bytes_read, bytes_written;
    
    ESP_LOGI(TAG, "Audio task started");
    
    while (priv->task_running) {
        // 检查队列中的消息
        if (xQueueReceive(priv->audio_queue, &queue_data, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (queue_data == 0) {
                // 停止信号
                break;
            }
        }
        
        if (xSemaphoreTake(priv->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
            continue;
        }
        
        if (!priv->task_running) {
            xSemaphoreGive(priv->mutex);
            break;
        }
        
        // 处理音频输出
        if (priv->config.enable_output && priv->output_callback && priv->output_buffer && priv->i2s_buffer) {
            // 清空输出缓冲区
            memset(priv->output_buffer, 0, priv->buffer_frames * priv->config.format.channels * sizeof(float));
            
            // 调用输出回调
            priv->output_callback(priv->output_buffer, priv->buffer_frames, priv->callback_user_data);
            
            // 转换为I2S格式并写入
            esp32_convert_float_to_i2s(priv->output_buffer, priv->i2s_buffer, priv->buffer_frames, priv->config.format.channels);
            
            esp_err_t err = i2s_write(priv->i2s_port, priv->i2s_buffer, 
                                     priv->buffer_frames * priv->config.format.channels * sizeof(int16_t),
                                     &bytes_written, pdMS_TO_TICKS(100));
            
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "I2S write error: %s", esp_err_to_name(err));
                priv->stats.underrun_count++;
            }
        }
        
        // 处理音频输入
        if (priv->config.enable_input && priv->input_callback && priv->input_buffer && priv->i2s_buffer) {
            esp_err_t err = i2s_read(priv->i2s_port, priv->i2s_buffer,
                                    priv->buffer_frames * priv->config.format.channels * sizeof(int16_t),
                                    &bytes_read, pdMS_TO_TICKS(100));
            
            if (err == ESP_OK && bytes_read > 0) {
                // 转换为浮点格式
                size_t frames_read = bytes_read / (priv->config.format.channels * sizeof(int16_t));
                esp32_convert_i2s_to_float(priv->i2s_buffer, priv->input_buffer, frames_read, priv->config.format.channels);
                
                // 调用输入回调
                priv->input_callback(priv->input_buffer, frames_read, priv->callback_user_data);
            } else {
                ESP_LOGW(TAG, "I2S read error: %s", esp_err_to_name(err));
                priv->stats.underrun_count++;
            }
        }
        
        priv->stats.callback_count++;
        
        xSemaphoreGive(priv->mutex);
        
        // 短暂延迟
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    ESP_LOGI(TAG, "Audio task stopped");
    vTaskDelete(NULL);
}

static esp_err_t esp32_setup_i2s(esp32_driver_private_t* priv) {
    // I2S配置
    priv->i2s_config = (i2s_config_t) {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = priv->config.format.sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = (priv->config.format.channels == 1) ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = priv->buffer_frames,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // 引脚配置
    priv->pin_config = (i2s_pin_config_t) {
        .bck_io_num = ESP32_I2S_BCK_PIN,
        .ws_io_num = ESP32_I2S_WS_PIN,
        .data_out_num = ESP32_I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // 安装I2S驱动
    esp_err_t err = i2s_driver_install(priv->i2s_port, &priv->i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(err));
        return err;
    }
    
    // 设置引脚
    err = i2s_set_pin(priv->i2s_port, &priv->pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(err));
        i2s_driver_uninstall(priv->i2s_port);
        return err;
    }
    
    ESP_LOGI(TAG, "I2S setup completed");
    return ESP_OK;
}

static void esp32_cleanup_i2s(esp32_driver_private_t* priv) {
    i2s_driver_uninstall(priv->i2s_port);
    ESP_LOGI(TAG, "I2S cleanup completed");
}

static void esp32_convert_float_to_i2s(const float* input, int16_t* output, size_t frames, uint32_t channels) {
    for (size_t i = 0; i < frames * channels; i++) {
        float sample = input[i];
        
        // 限制范围
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        
        // 转换为16位整数
        output[i] = (int16_t)(sample * 32767.0f);
    }
}

static void esp32_convert_i2s_to_float(const int16_t* input, float* output, size_t frames, uint32_t channels) {
    for (size_t i = 0; i < frames * channels; i++) {
        output[i] = (float)input[i] / 32767.0f;
    }
}

#endif // LINX_PLATFORM_ESP32

// ============================================================================
// 辅助函数
// ============================================================================

static void esp32_init_devices(esp32_driver_private_t* priv) {
    priv->device_count = 2;
    priv->devices = malloc(priv->device_count * sizeof(esp32_device_t));
    
    if (!priv->devices) {
        priv->device_count = 0;
        return;
    }
    
    // I2S输出设备
    priv->devices[0].device_id = 0;
    strcpy(priv->devices[0].name, "ESP32 I2S Output");
    priv->devices[0].type = LINX_AUDIO_DEVICE_TYPE_OUTPUT;
    priv->devices[0].format.sample_rate = 44100;
    priv->devices[0].format.channels = 2;
    priv->devices[0].format.format = LINX_AUDIO_FORMAT_INT16;
    priv->devices[0].format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    priv->devices[0].is_default = true;
#ifdef LINX_PLATFORM_ESP32
    priv->devices[0].i2s_port = I2S_NUM_0;
#else
    priv->devices[0].i2s_port = 0;
#endif
    
    // I2S输入设备
    priv->devices[1].device_id = 1;
    strcpy(priv->devices[1].name, "ESP32 I2S Input");
    priv->devices[1].type = LINX_AUDIO_DEVICE_TYPE_INPUT;
    priv->devices[1].format.sample_rate = 44100;
    priv->devices[1].format.channels = 2;
    priv->devices[1].format.format = LINX_AUDIO_FORMAT_INT16;
    priv->devices[1].format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    priv->devices[1].is_default = true;
#ifdef LINX_PLATFORM_ESP32
    priv->devices[1].i2s_port = I2S_NUM_0;
#else
    priv->devices[1].i2s_port = 0;
#endif
}

static void esp32_cleanup_devices(esp32_driver_private_t* priv) {
    if (priv->devices) {
        free(priv->devices);
        priv->devices = NULL;
        priv->device_count = 0;
    }
}