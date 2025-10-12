/**
 * @file delay.c
 * @brief 延迟效果插件实现
 * @details 提供音频延迟和回声效果
 */

#include "../plugin_interface.h"
#include "../../core/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// 常量定义
// ============================================================================

#define DELAY_MAX_TIME_MS 5000.0f    // 最大延迟时间（毫秒）
#define DELAY_MIN_TIME_MS 1.0f       // 最小延迟时间（毫秒）
#define DELAY_MAX_FEEDBACK 0.95f     // 最大反馈量
#define DELAY_MAX_CHANNELS 8         // 最大支持声道数

// ============================================================================
// 延迟线结构
// ============================================================================

typedef struct {
    float* buffer;          // 延迟缓冲区
    uint32_t buffer_size;   // 缓冲区大小（样本数）
    uint32_t write_pos;     // 写入位置
    uint32_t read_pos;      // 读取位置
    uint32_t delay_samples; // 延迟样本数
} delay_line_t;

// ============================================================================
// 延迟插件私有数据结构
// ============================================================================

typedef struct {
    linx_plugin_base_t base;
    
    // 延迟线（每个声道一个）
    delay_line_t delay_lines[DELAY_MAX_CHANNELS];
    
    // 音频格式
    uint32_t sample_rate;
    uint32_t channels;
    
    // 延迟参数
    float delay_time_ms;    // 延迟时间（毫秒）
    float feedback;         // 反馈量 (0.0 - 0.95)
    float wet_level;        // 湿信号电平 (0.0 - 1.0)
    float dry_level;        // 干信号电平 (0.0 - 1.0)
    
    // 控制参数
    bool bypass;
    bool stereo_link;       // 立体声链接
    
    // 统计信息
    uint64_t samples_processed;
    
    // 内部状态
    bool initialized;
} delay_plugin_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t delay_line_init(delay_line_t* line, uint32_t max_samples);
static void delay_line_cleanup(delay_line_t* line);
static void delay_line_set_delay(delay_line_t* line, uint32_t delay_samples);
static float delay_line_process(delay_line_t* line, float input);
static void delay_line_clear(delay_line_t* line);

static linx_audio_result_t delay_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
static linx_audio_result_t delay_deinitialize(linx_plugin_base_t* plugin);
static linx_audio_result_t delay_start(linx_plugin_base_t* plugin);
static linx_audio_result_t delay_stop(linx_plugin_base_t* plugin);
static linx_audio_result_t delay_process(linx_plugin_base_t* plugin,
                                        const linx_audio_buffer_t* input,
                                        linx_audio_buffer_t* output);
static linx_audio_result_t delay_set_parameter(linx_plugin_base_t* plugin, 
                                              const char* name, const char* value);
static linx_audio_result_t delay_get_parameter(linx_plugin_base_t* plugin, 
                                              const char* name, char* value, size_t size);
static linx_audio_result_t delay_set_format(linx_plugin_base_t* plugin, 
                                           const linx_audio_format_info_t* format);
static void delay_destroy(linx_plugin_base_t* plugin);

// ============================================================================
// 插件虚函数表
// ============================================================================

static const linx_plugin_vtable_t delay_vtable = {
    .initialize = delay_initialize,
    .deinitialize = delay_deinitialize,
    .start = delay_start,
    .stop = delay_stop,
    .pause = NULL,
    .resume = NULL,
    .reset = NULL,
    .process = delay_process,
    .process_inplace = NULL,
    .set_config = NULL,
    .get_config = NULL,
    .set_parameter = delay_set_parameter,
    .get_parameter = delay_get_parameter,
    .set_format = delay_set_format,
    .get_format = NULL,
    .supports_format = NULL,
    .get_latency = NULL,
    .get_tail_time = NULL,
    .get_state = NULL,
    .get_info = NULL,
    .on_event = NULL,
    .destroy = delay_destroy
};

// ============================================================================
// 延迟线实现
// ============================================================================

static linx_audio_result_t delay_line_init(delay_line_t* line, uint32_t max_samples) {
    if (!line || max_samples == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    line->buffer = calloc(max_samples, sizeof(float));
    if (!line->buffer) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    line->buffer_size = max_samples;
    line->write_pos = 0;
    line->read_pos = 0;
    line->delay_samples = 0;
    
    return LINX_AUDIO_SUCCESS;
}

static void delay_line_cleanup(delay_line_t* line) {
    if (line && line->buffer) {
        free(line->buffer);
        line->buffer = NULL;
        line->buffer_size = 0;
        line->write_pos = 0;
        line->read_pos = 0;
        line->delay_samples = 0;
    }
}

static void delay_line_set_delay(delay_line_t* line, uint32_t delay_samples) {
    if (!line || delay_samples >= line->buffer_size) {
        return;
    }
    
    line->delay_samples = delay_samples;
    
    // 更新读取位置
    if (line->write_pos >= delay_samples) {
        line->read_pos = line->write_pos - delay_samples;
    } else {
        line->read_pos = line->buffer_size - (delay_samples - line->write_pos);
    }
}

static float delay_line_process(delay_line_t* line, float input) {
    if (!line || !line->buffer) {
        return input;
    }
    
    // 读取延迟样本
    float delayed_sample = line->buffer[line->read_pos];
    
    // 写入新样本
    line->buffer[line->write_pos] = input;
    
    // 更新指针
    line->write_pos = (line->write_pos + 1) % line->buffer_size;
    line->read_pos = (line->read_pos + 1) % line->buffer_size;
    
    return delayed_sample;
}

static void delay_line_clear(delay_line_t* line) {
    if (line && line->buffer) {
        memset(line->buffer, 0, line->buffer_size * sizeof(float));
        line->write_pos = 0;
        line->read_pos = 0;
    }
}

// ============================================================================
// 插件接口实现
// ============================================================================

static linx_audio_result_t delay_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    // 设置默认参数
    delay->sample_rate = 44100;
    delay->channels = 2;
    delay->delay_time_ms = 250.0f;
    delay->feedback = 0.3f;
    delay->wet_level = 0.3f;
    delay->dry_level = 0.7f;
    delay->bypass = false;
    delay->stereo_link = true;
    delay->samples_processed = 0;
    delay->initialized = false;
    
    // 初始化延迟线
    uint32_t max_samples = (uint32_t)((DELAY_MAX_TIME_MS / 1000.0f) * delay->sample_rate);
    
    for (uint32_t i = 0; i < DELAY_MAX_CHANNELS; i++) {
        linx_audio_result_t result = delay_line_init(&delay->delay_lines[i], max_samples);
        if (result != LINX_AUDIO_SUCCESS) {
            // 清理已初始化的延迟线
            for (uint32_t j = 0; j < i; j++) {
                delay_line_cleanup(&delay->delay_lines[j]);
            }
            return result;
        }
    }
    
    // 设置初始延迟时间
    uint32_t delay_samples = (uint32_t)((delay->delay_time_ms / 1000.0f) * delay->sample_rate);
    for (uint32_t i = 0; i < DELAY_MAX_CHANNELS; i++) {
        delay_line_set_delay(&delay->delay_lines[i], delay_samples);
    }
    
    delay->initialized = true;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t delay_deinitialize(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    if (delay->initialized) {
        // 清理所有延迟线
        for (uint32_t i = 0; i < DELAY_MAX_CHANNELS; i++) {
            delay_line_cleanup(&delay->delay_lines[i]);
        }
        delay->initialized = false;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t delay_start(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    // 清空所有延迟线
    for (uint32_t i = 0; i < delay->channels && i < DELAY_MAX_CHANNELS; i++) {
        delay_line_clear(&delay->delay_lines[i]);
    }
    
    delay->samples_processed = 0;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t delay_stop(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t delay_process(linx_plugin_base_t* plugin,
                                        const linx_audio_buffer_t* input,
                                        linx_audio_buffer_t* output) {
    if (!plugin || !input || !output) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    // 检查缓冲区格式
    if (input->frame_count != output->frame_count ||
        input->channels != output->channels) {
        return LINX_AUDIO_ERROR_INVALID_FORMAT;
    }
    
    const float* in_samples = (const float*)input->data;
    float* out_samples = (float*)output->data;
    uint32_t total_samples = input->frame_count * input->channels;
    
    // 旁路模式
    if (delay->bypass) {
        if (in_samples != out_samples) {
            memcpy(out_samples, in_samples, total_samples * sizeof(float));
        }
        return LINX_AUDIO_SUCCESS;
    }
    
    // 处理每个样本帧
    for (uint32_t frame = 0; frame < input->frame_count; frame++) {
        for (uint32_t ch = 0; ch < input->channels && ch < DELAY_MAX_CHANNELS; ch++) {
            uint32_t sample_idx = frame * input->channels + ch;
            float input_sample = in_samples[sample_idx];
            
            // 获取延迟样本
            float delayed_sample = delay_line_process(&delay->delay_lines[ch], 
                                                     input_sample + delayed_sample * delay->feedback);
            
            // 混合干湿信号
            float output_sample = input_sample * delay->dry_level + 
                                 delayed_sample * delay->wet_level;
            
            // 防止削波
            if (output_sample > 1.0f) output_sample = 1.0f;
            else if (output_sample < -1.0f) output_sample = -1.0f;
            
            out_samples[sample_idx] = output_sample;
        }
    }
    
    delay->samples_processed += total_samples;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t delay_set_parameter(linx_plugin_base_t* plugin, 
                                              const char* name, const char* value) {
    if (!plugin || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        delay->bypass = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "delay_time") == 0) {
        float time_ms = atof(value);
        if (time_ms >= DELAY_MIN_TIME_MS && time_ms <= DELAY_MAX_TIME_MS) {
            delay->delay_time_ms = time_ms;
            
            // 更新延迟线
            uint32_t delay_samples = (uint32_t)((time_ms / 1000.0f) * delay->sample_rate);
            for (uint32_t i = 0; i < delay->channels && i < DELAY_MAX_CHANNELS; i++) {
                delay_line_set_delay(&delay->delay_lines[i], delay_samples);
            }
            
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "feedback") == 0) {
        float feedback = atof(value);
        if (feedback >= 0.0f && feedback <= DELAY_MAX_FEEDBACK) {
            delay->feedback = feedback;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "wet_level") == 0) {
        float wet = atof(value);
        if (wet >= 0.0f && wet <= 1.0f) {
            delay->wet_level = wet;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "dry_level") == 0) {
        float dry = atof(value);
        if (dry >= 0.0f && dry <= 1.0f) {
            delay->dry_level = dry;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "stereo_link") == 0) {
        delay->stereo_link = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t delay_get_parameter(linx_plugin_base_t* plugin, 
                                              const char* name, char* value, size_t size) {
    if (!plugin || !name || !value || size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        snprintf(value, size, "%s", delay->bypass ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "delay_time") == 0) {
        snprintf(value, size, "%.3f", delay->delay_time_ms);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "feedback") == 0) {
        snprintf(value, size, "%.3f", delay->feedback);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "wet_level") == 0) {
        snprintf(value, size, "%.3f", delay->wet_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "dry_level") == 0) {
        snprintf(value, size, "%.3f", delay->dry_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "stereo_link") == 0) {
        snprintf(value, size, "%s", delay->stereo_link ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "samples_processed") == 0) {
        snprintf(value, size, "%llu", (unsigned long long)delay->samples_processed);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t delay_set_format(linx_plugin_base_t* plugin, 
                                           const linx_audio_format_info_t* format) {
    if (!plugin || !format) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    delay_plugin_t* delay = (delay_plugin_t*)plugin;
    
    // 更新采样率和声道数
    if (format->sample_rate != delay->sample_rate || format->channels != delay->channels) {
        delay->sample_rate = format->sample_rate;
        delay->channels = format->channels;
        
        // 重新设置延迟时间
        uint32_t delay_samples = (uint32_t)((delay->delay_time_ms / 1000.0f) * delay->sample_rate);
        for (uint32_t i = 0; i < delay->channels && i < DELAY_MAX_CHANNELS; i++) {
            delay_line_set_delay(&delay->delay_lines[i], delay_samples);
            delay_line_clear(&delay->delay_lines[i]);
        }
    }
    
    return LINX_AUDIO_SUCCESS;
}

static void delay_destroy(linx_plugin_base_t* plugin) {
    if (plugin) {
        delay_deinitialize(plugin);
        free(plugin);
    }
}

// ============================================================================
// 插件工厂函数
// ============================================================================

linx_plugin_base_t* create_delay_plugin(const linx_plugin_config_t* config) {
    delay_plugin_t* delay = malloc(sizeof(delay_plugin_t));
    if (!delay) {
        return NULL;
    }
    
    memset(delay, 0, sizeof(delay_plugin_t));
    
    // 初始化基础插件结构
    linx_plugin_metadata_t metadata = {
        .name = "Delay",
        .version = {1, 0, 0},
        .description = "Audio delay and echo effect",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    };
    
    linx_audio_result_t result = linx_plugin_base_init(&delay->base, &delay_vtable, &metadata);
    if (result != LINX_AUDIO_SUCCESS) {
        free(delay);
        return NULL;
    }
    
    // 初始化插件
    if (delay_initialize(&delay->base, config) != LINX_AUDIO_SUCCESS) {
        free(delay);
        return NULL;
    }
    
    return &delay->base;
}

void destroy_delay_plugin(linx_plugin_base_t* plugin) {
    if (plugin) {
        delay_destroy(plugin);
    }
}

linx_audio_result_t get_delay_plugin_metadata(linx_plugin_metadata_t* metadata) {
    if (!metadata) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    metadata->name = "Delay";
    metadata->version.major = 1;
    metadata->version.minor = 0;
    metadata->version.patch = 0;
    metadata->description = "Audio delay and echo effect";
    metadata->author = "LinxOS Audio Team";
    metadata->license = "MIT";
    metadata->type = LINX_PLUGIN_TYPE_EFFECT;
    
    return LINX_AUDIO_SUCCESS;
}

// 插件描述符
LINX_PLUGIN_IMPLEMENT(delay, {
    .metadata = {
        .name = "Delay",
        .version = {1, 0, 0},
        .description = "Audio delay and echo effect",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    },
    .create_func = create_delay_plugin,
    .destroy_func = destroy_delay_plugin,
    .get_metadata_func = get_delay_plugin_metadata
});