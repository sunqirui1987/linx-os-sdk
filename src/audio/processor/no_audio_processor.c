#include "no_audio_processor.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file no_audio_processor.c
 * @brief 无音频处理实现
 * @details 这是一个不进行任何音频处理的处理器，仅进行通道转换和数据透传
 */

#define NO_AUDIO_PROCESSOR_TAG "NoAudioProcessor"

// 私有数据结构
typedef struct {
    audio_processor_config_t config;
    AudioInterface* audio_interface;
    
    // 状态标志
    bool initialized;
    bool started;
    
    // 回调函数
    audio_processor_output_callback_t output_callback;
    void* output_callback_user_data;
    audio_processor_vad_callback_t vad_callback;
    void* vad_callback_user_data;
    
    // 音频缓冲区 - 使用vector替代固定数组
    vector_int16_t buffer;
} NoAudioProcessorData;

// 前向声明
static audio_processor_error_t no_audio_processor_initialize(AudioProcessor* self,
                                                           const audio_processor_config_t* config,
                                                           AudioInterface* audio_interface);
static audio_processor_error_t no_audio_processor_start(AudioProcessor* self);
static audio_processor_error_t no_audio_processor_stop(AudioProcessor* self);
static audio_processor_error_t no_audio_processor_feed(AudioProcessor* self, 
                                                      const int16_t* data, size_t size);
static size_t no_audio_processor_get_feed_size(const AudioProcessor* self);
static audio_processor_error_t no_audio_processor_enable_device_aec(AudioProcessor* self, bool enable);
static audio_processor_error_t no_audio_processor_set_output_callback(AudioProcessor* self,
                                                                     audio_processor_output_callback_t callback,
                                                                     void* user_data);
static audio_processor_error_t no_audio_processor_set_vad_callback(AudioProcessor* self,
                                                                  audio_processor_vad_callback_t callback,
                                                                  void* user_data);
static bool no_audio_processor_get_vad_status(const AudioProcessor* self);
static audio_processor_error_t no_audio_processor_reset(AudioProcessor* self);
static int no_audio_processor_get_delay_ms(const AudioProcessor* self);

// 虚函数表
static const AudioProcessorVTable no_audio_processor_vtable = {
    .initialize = no_audio_processor_initialize,
    .start = no_audio_processor_start,
    .stop = no_audio_processor_stop,
    .feed = no_audio_processor_feed,
    .get_feed_size = no_audio_processor_get_feed_size,
    .enable_device_aec = no_audio_processor_enable_device_aec,
    .set_output_callback = no_audio_processor_set_output_callback,
    .set_vad_callback = no_audio_processor_set_vad_callback,
    .get_vad_status = no_audio_processor_get_vad_status,
    .reset = no_audio_processor_reset,
    .get_delay_ms = no_audio_processor_get_delay_ms,
    .destroy = no_audio_processor_destroy
};

// =============================================================================
// 公共API实现
// =============================================================================

AudioProcessor* no_audio_processor_create(void) {
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "创建无音频处理器");
    
    AudioProcessor* processor = (AudioProcessor*)calloc(1, sizeof(AudioProcessor));
    if (!processor) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "分配AudioProcessor内存失败");
        return NULL;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)calloc(1, sizeof(NoAudioProcessorData));
    if (!data) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "分配NoAudioProcessorData内存失败");
        free(processor);
        return NULL;
    }
    
    // 初始化vector缓冲区
    if (vector_int16_init(&data->buffer) != 0) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "初始化vector缓冲区失败");
        free(data);
        free(processor);
        return NULL;
    }
    
    // 初始化基本结构
    processor->vtable = &no_audio_processor_vtable;
    processor->private_data = data;
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器创建成功");
    return processor;
}

void no_audio_processor_destroy(AudioProcessor* processor) {
    if (!processor) {
        return;
    }
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "销毁无音频处理器");
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)processor->private_data;
    if (data) {
        // 停止处理器
        if (data->started) {
            no_audio_processor_stop(processor);
        }
        
        // 清理缓冲区
        vector_int16_destroy(&data->buffer);
        
        free(data);
    }
    
    free(processor);
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器销毁完成");
}

// =============================================================================
// 虚函数表实现
// =============================================================================

static audio_processor_error_t no_audio_processor_initialize(AudioProcessor* self,
                                                           const audio_processor_config_t* config,
                                                           AudioInterface* audio_interface) {
    if (!self || !config) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "初始化参数无效");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "处理器数据为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "初始化无音频处理器，采样率: %d, 声道: %d, 帧长: %dms",
              config->sample_rate, config->channels, config->frame_duration_ms);
    
    // 保存配置
    data->config = *config;
    data->audio_interface = audio_interface;
    
    // 计算缓冲区大小
    size_t buffer_size = (config->sample_rate * config->frame_duration_ms) / 1000;
    
    // 调整缓冲区大小
    if (vector_int16_resize(&data->buffer, buffer_size) != 0) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "调整音频缓冲区大小失败");
        return AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC;
    }
    
    data->initialized = true;
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器初始化完成");
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t no_audio_processor_start(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "处理器未初始化");
        return AUDIO_PROCESSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (data->started) {
        LINX_LOGW(NO_AUDIO_PROCESSOR_TAG, "处理器已经启动");
        return AUDIO_PROCESSOR_ERROR_ALREADY_STARTED;
    }
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "启动无音频处理器");
    data->started = true;
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器启动成功");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t no_audio_processor_stop(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!data->started) {
        LINX_LOGW(NO_AUDIO_PROCESSOR_TAG, "处理器未启动");
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "停止无音频处理器");
    data->started = false;
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器停止成功");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t no_audio_processor_feed(AudioProcessor* self, 
                                                      const int16_t* data_input, size_t size) {
    if (!self || !data_input || size == 0) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data || !data->started) {
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }
    
    if (!data->output_callback) {
        return AUDIO_PROCESSOR_SUCCESS;
    }
    
    // 如果输入通道是2，我们需要提取左声道数据
    if (data->config.channels == 2) {
        size_t mono_size = size / 2;
        size_t buffer_capacity = vector_int16_size(&data->buffer);
        
        if (mono_size > buffer_capacity) {
            mono_size = buffer_capacity;
        }
        
        // 提取左声道数据
        for (size_t i = 0, j = 0; i < mono_size; ++i, j += 2) {
            vector_int16_set(&data->buffer, i, data_input[j]);
        }
        
        data->output_callback(vector_int16_data(&data->buffer), mono_size, data->output_callback_user_data);
    } else {
        data->output_callback(data_input, size, data->output_callback_user_data);
    }
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static size_t no_audio_processor_get_feed_size(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }
    
    return vector_int16_size(&data->buffer);
}

static audio_processor_error_t no_audio_processor_enable_device_aec(AudioProcessor* self, bool enable) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (enable) {
        LINX_LOGE(NO_AUDIO_PROCESSOR_TAG, "设备AEC不支持");
    }
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t no_audio_processor_set_output_callback(AudioProcessor* self,
                                                                     audio_processor_output_callback_t callback,
                                                                     void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    data->output_callback = callback;
    data->output_callback_user_data = user_data;
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "设置输出回调函数");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t no_audio_processor_set_vad_callback(AudioProcessor* self,
                                                                  audio_processor_vad_callback_t callback,
                                                                  void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    data->vad_callback = callback;
    data->vad_callback_user_data = user_data;
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "设置VAD回调函数");
    return AUDIO_PROCESSOR_SUCCESS;
}

static bool no_audio_processor_get_vad_status(const AudioProcessor* self) {
    // 无音频处理器不支持VAD
    return false;
}

static audio_processor_error_t no_audio_processor_reset(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "重置无音频处理器");
    
    // 无音频处理器没有状态需要重置
    LINX_LOGI(NO_AUDIO_PROCESSOR_TAG, "无音频处理器重置完成");
    return AUDIO_PROCESSOR_SUCCESS;
}

static int no_audio_processor_get_delay_ms(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }
    
    NoAudioProcessorData* data = (NoAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }
    
    // 无音频处理器没有处理延迟
    return 0;
}