/**
 * @file gain.c
 * @brief 增益控制插件实现
 * @details 提供音频信号的音量调节功能
 */

#include "../plugin_interface.h"
#include "../../core/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// 增益插件私有数据结构
// ============================================================================

typedef struct {
    linx_plugin_base_t base;
    
    // 增益参数
    float gain_db;          // 增益值（分贝）
    float gain_linear;      // 线性增益值
    bool mute;              // 静音状态
    
    // 平滑处理
    float target_gain;      // 目标增益值
    float current_gain;     // 当前增益值
    float smooth_factor;    // 平滑因子
    
    // 统计信息
    uint64_t samples_processed;
    float peak_level;
    float rms_level;
} gain_plugin_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t gain_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
static linx_audio_result_t gain_deinitialize(linx_plugin_base_t* plugin);
static linx_audio_result_t gain_start(linx_plugin_base_t* plugin);
static linx_audio_result_t gain_stop(linx_plugin_base_t* plugin);
static linx_audio_result_t gain_process(linx_plugin_base_t* plugin,
                                       const linx_audio_buffer_t* input,
                                       linx_audio_buffer_t* output);
static linx_audio_result_t gain_set_parameter(linx_plugin_base_t* plugin, 
                                             const char* name, const char* value);
static linx_audio_result_t gain_get_parameter(linx_plugin_base_t* plugin, 
                                             const char* name, char* value, size_t size);
static void gain_destroy(linx_plugin_base_t* plugin);

// ============================================================================
// 插件虚函数表
// ============================================================================

static linx_plugin_state_t gain_get_state(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return PLUGIN_STATE_ERROR;
    }
    return plugin->state;
}

static const linx_plugin_vtable_t gain_vtable = {
    .initialize = gain_initialize,
    .deinitialize = gain_deinitialize,
    .start = gain_start,
    .stop = gain_stop,
    .process = gain_process,
    .set_parameter = gain_set_parameter,
    .get_parameter = gain_get_parameter,
    .get_state = gain_get_state,
    .destroy = gain_destroy
};

// ============================================================================
// 工具函数
// ============================================================================

static float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}



static void process_audio_samples(gain_plugin_t* gain, 
                                const float* input, 
                                float* output, 
                                uint32_t frame_count,
                                uint32_t channels) {
    float peak = 0.0f;
    float sum_squares = 0.0f;
    
    for (uint32_t i = 0; i < frame_count * channels; i++) {
        // 平滑增益变化
        if (gain->current_gain != gain->target_gain) {
            gain->current_gain += (gain->target_gain - gain->current_gain) * gain->smooth_factor;
        }
        
        // 应用增益
        float sample = input[i] * gain->current_gain;
        
        // 防止削波
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        
        output[i] = sample;
        
        // 统计信息
        float abs_sample = fabsf(sample);
        if (abs_sample > peak) peak = abs_sample;
        sum_squares += sample * sample;
    }
    
    // 更新统计信息
    gain->peak_level = peak;
    gain->rms_level = sqrtf(sum_squares / (frame_count * channels));
    gain->samples_processed += frame_count * channels;
}

// ============================================================================
// 插件接口实现
// ============================================================================

static linx_audio_result_t gain_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config) {
    (void)config; // 避免未使用参数警告
    
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    gain_plugin_t* gain = (gain_plugin_t*)plugin;
    
    // 设置默认参数
    gain->gain_db = 0.0f;
    gain->gain_linear = 1.0f;
    gain->mute = false;
    gain->target_gain = 1.0f;
    gain->current_gain = 1.0f;
    gain->smooth_factor = 0.01f; // 平滑因子
    
    // 重置统计信息
    gain->samples_processed = 0;
    gain->peak_level = 0.0f;
    gain->rms_level = 0.0f;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t gain_deinitialize(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 清理资源（如果有的话）
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t gain_start(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    gain_plugin_t* gain = (gain_plugin_t*)plugin;
    
    // 重置统计信息
    gain->samples_processed = 0;
    gain->peak_level = 0.0f;
    gain->rms_level = 0.0f;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t gain_stop(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t gain_process(linx_plugin_base_t* plugin,
                                       const linx_audio_buffer_t* input,
                                       linx_audio_buffer_t* output) {
    if (!plugin || !input || !output) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    gain_plugin_t* gain = (gain_plugin_t*)plugin;
    
    // 检查缓冲区格式
    if (input->frames != output->frames ||
        input->params.channels != output->params.channels) {
        return LINX_AUDIO_ERROR_UNSUPPORTED_FORMAT;
    }
    
    // 静音处理
    if (gain->mute) {
        memset(output->data, 0, output->frames * output->params.channels * sizeof(float));
        return LINX_AUDIO_SUCCESS;
    }
    
    // 处理音频数据
    process_audio_samples(gain, 
                         (const float*)input->data,
                         (float*)output->data,
                         input->frames,
                         input->params.channels);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t gain_set_parameter(linx_plugin_base_t* plugin, 
                                             const char* name, const char* value) {
    if (!plugin || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    gain_plugin_t* gain = (gain_plugin_t*)plugin;
    
    if (strcmp(name, "gain_db") == 0) {
        float db = atof(value);
        if (db >= -60.0f && db <= 20.0f) {
            gain->gain_db = db;
            gain->gain_linear = db_to_linear(db);
            gain->target_gain = gain->mute ? 0.0f : gain->gain_linear;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "mute") == 0) {
        gain->mute = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        gain->target_gain = gain->mute ? 0.0f : gain->gain_linear;
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "smooth_factor") == 0) {
        float factor = atof(value);
        if (factor > 0.0f && factor <= 1.0f) {
            gain->smooth_factor = factor;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t gain_get_parameter(linx_plugin_base_t* plugin, 
                                             const char* name, char* value, size_t size) {
    if (!plugin || !name || !value || size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    gain_plugin_t* gain = (gain_plugin_t*)plugin;
    
    if (strcmp(name, "gain_db") == 0) {
        snprintf(value, size, "%.2f", gain->gain_db);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "mute") == 0) {
        snprintf(value, size, "%s", gain->mute ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "peak_level") == 0) {
        snprintf(value, size, "%.6f", gain->peak_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "rms_level") == 0) {
        snprintf(value, size, "%.6f", gain->rms_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "samples_processed") == 0) {
        snprintf(value, size, "%llu", (unsigned long long)gain->samples_processed);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static void gain_destroy(linx_plugin_base_t* plugin) {
    if (plugin) {
        free(plugin);
    }
}

// ============================================================================
// 插件工厂函数
// ============================================================================

linx_plugin_base_t* create_gain_plugin(const linx_plugin_config_t* config) {
    gain_plugin_t* gain = malloc(sizeof(gain_plugin_t));
    if (!gain) {
        return NULL;
    }
    
    memset(gain, 0, sizeof(gain_plugin_t));
    
    // 初始化基础插件结构
    linx_plugin_metadata_t metadata = {
        .name = "Gain",
        .version = {1, 0, 0, NULL},
        .description = "Audio gain control plugin",
        .author = "LinxOS Audio Team",
        .type = LINX_AUDIO_PLUGIN_TYPE_EFFECT,
        .capabilities = PLUGIN_CAP_REALTIME | PLUGIN_CAP_MULTI_CHANNEL | PLUGIN_CAP_CONFIGURABLE
    };
    
    linx_audio_result_t result = linx_plugin_base_init(&gain->base, &gain_vtable, &metadata);
    if (result != LINX_AUDIO_SUCCESS) {
        free(gain);
        return NULL;
    }
    
    // 初始化插件
    if (gain_initialize(&gain->base, config) != LINX_AUDIO_SUCCESS) {
        free(gain);
        return NULL;
    }
    
    return &gain->base;
}

void destroy_gain_plugin(linx_plugin_base_t* plugin) {
    if (plugin) {
        gain_deinitialize(plugin);
        gain_destroy(plugin);
    }
}

linx_audio_result_t get_gain_plugin_metadata(linx_plugin_metadata_t* metadata) {
    if (!metadata) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }

    metadata->name = "Gain";
    metadata->version.major = 1;
    metadata->version.minor = 0;
    metadata->version.patch = 0;
    metadata->version.build = "stable";
    metadata->description = "Audio gain control plugin";
    metadata->author = "LinxOS Audio Team";
    metadata->type = LINX_AUDIO_PLUGIN_TYPE_EFFECT;
    metadata->capabilities = PLUGIN_CAP_REALTIME | PLUGIN_CAP_MULTI_CHANNEL | PLUGIN_CAP_CONFIGURABLE;

    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 插件描述符
// ============================================================================

/**
 * @brief 获取gain插件描述符
 * @return 插件描述符指针
 */
const linx_plugin_descriptor_t* linx_gain_plugin_get_descriptor(void) {
    static const linx_plugin_descriptor_t descriptor = {
        .metadata = {
            .name = "Gain",
            .description = "Audio gain control plugin",
            .author = "LinxOS Audio Team",
            .version = {1, 0, 0, "stable"},
            .type = LINX_AUDIO_PLUGIN_TYPE_EFFECT,
            .capabilities = PLUGIN_CAP_REALTIME | PLUGIN_CAP_INPLACE | PLUGIN_CAP_MULTI_CHANNEL | PLUGIN_CAP_CONFIGURABLE
        },
        .create = create_gain_plugin,
        .destroy = destroy_gain_plugin,
        .get_metadata = get_gain_plugin_metadata
    };
    
    return &descriptor;
}