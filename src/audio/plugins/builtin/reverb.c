/**
 * @file reverb.c
 * @brief 混响效果插件实现
 * @details 基于Freeverb算法的混响效果器
 */

#include "../plugin_interface.h"
#include "../../core/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// 常量定义
// ============================================================================

#define REVERB_NUM_COMBS 8
#define REVERB_NUM_ALLPASSES 4
#define REVERB_STEREO_SPREAD 23
#define REVERB_SCALE_WET 3.0f
#define REVERB_SCALE_DRY 2.0f
#define REVERB_SCALE_DAMP 0.4f
#define REVERB_SCALE_ROOM 0.28f
#define REVERB_OFFSET_ROOM 0.7f
#define REVERB_INITIAL_ROOM 0.5f
#define REVERB_INITIAL_DAMP 0.5f
#define REVERB_INITIAL_WET 1.0f / REVERB_SCALE_WET
#define REVERB_INITIAL_DRY 0.0f
#define REVERB_INITIAL_WIDTH 1.0f
#define REVERB_FREEZE_MODE 0.5f

// Comb滤波器延迟长度（44.1kHz采样率）
static const int comb_tuning[REVERB_NUM_COMBS] = {
    1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
};

// Allpass滤波器延迟长度
static const int allpass_tuning[REVERB_NUM_ALLPASSES] = {
    556, 441, 341, 225
};

// ============================================================================
// 滤波器结构
// ============================================================================

typedef struct {
    float* buffer;
    int buffer_size;
    int buffer_index;
    float feedback;
    float filter_store;
    float damp1;
    float damp2;
} comb_filter_t;

typedef struct {
    float* buffer;
    int buffer_size;
    int buffer_index;
    float feedback;
} allpass_filter_t;

// ============================================================================
// 混响插件私有数据结构
// ============================================================================

typedef struct {
    linx_plugin_base_t base;
    
    // 滤波器组（立体声）
    comb_filter_t comb_L[REVERB_NUM_COMBS];
    comb_filter_t comb_R[REVERB_NUM_COMBS];
    allpass_filter_t allpass_L[REVERB_NUM_ALLPASSES];
    allpass_filter_t allpass_R[REVERB_NUM_ALLPASSES];
    
    // 混响参数
    float room_size;    // 房间大小 (0.0 - 1.0)
    float damping;      // 阻尼 (0.0 - 1.0)
    float wet_level;    // 湿信号电平 (0.0 - 1.0)
    float dry_level;    // 干信号电平 (0.0 - 1.0)
    float width;        // 立体声宽度 (0.0 - 1.0)
    
    // 内部参数
    float gain;
    float room_size1;
    float room_size2;
    float damp1;
    float damp2;
    float wet1;
    float wet2;
    
    // 音频格式
    uint32_t sample_rate;
    uint32_t channels;
    
    // 控制参数
    bool bypass;
    bool freeze_mode;
    
    // 统计信息
    uint64_t samples_processed;
    
    // 内部状态
    bool initialized;
} reverb_plugin_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t comb_filter_init(comb_filter_t* comb, int size);
static void comb_filter_cleanup(comb_filter_t* comb);
static float comb_filter_process(comb_filter_t* comb, float input);
static void comb_filter_set_damp(comb_filter_t* comb, float val);
static void comb_filter_set_feedback(comb_filter_t* comb, float val);

static linx_audio_result_t allpass_filter_init(allpass_filter_t* allpass, int size);
static void allpass_filter_cleanup(allpass_filter_t* allpass);
static float allpass_filter_process(allpass_filter_t* allpass, float input);
static void allpass_filter_set_feedback(allpass_filter_t* allpass, float val);

static void reverb_update_parameters(reverb_plugin_t* reverb);
static void reverb_clear_buffers(reverb_plugin_t* reverb);

static linx_audio_result_t reverb_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
static linx_audio_result_t reverb_deinitialize(linx_plugin_base_t* plugin);
static linx_audio_result_t reverb_start(linx_plugin_base_t* plugin);
static linx_audio_result_t reverb_stop(linx_plugin_base_t* plugin);
static linx_audio_result_t reverb_process(linx_plugin_base_t* plugin,
                                         const linx_audio_buffer_t* input,
                                         linx_audio_buffer_t* output);
static linx_audio_result_t reverb_set_parameter(linx_plugin_base_t* plugin, 
                                               const char* name, const char* value);
static linx_audio_result_t reverb_get_parameter(linx_plugin_base_t* plugin, 
                                               const char* name, char* value, size_t size);
static linx_audio_result_t reverb_set_format(linx_plugin_base_t* plugin, 
                                            const linx_audio_format_info_t* format);
static void reverb_destroy(linx_plugin_base_t* plugin);

// ============================================================================
// 插件虚函数表
// ============================================================================

static const linx_plugin_vtable_t reverb_vtable = {
    .initialize = reverb_initialize,
    .deinitialize = reverb_deinitialize,
    .start = reverb_start,
    .stop = reverb_stop,
    .pause = NULL,
    .resume = NULL,
    .reset = NULL,
    .process = reverb_process,
    .process_inplace = NULL,
    .set_config = NULL,
    .get_config = NULL,
    .set_parameter = reverb_set_parameter,
    .get_parameter = reverb_get_parameter,
    .set_format = reverb_set_format,
    .get_format = NULL,
    .supports_format = NULL,
    .get_latency = NULL,
    .get_tail_time = NULL,
    .get_state = NULL,
    .get_info = NULL,
    .on_event = NULL,
    .destroy = reverb_destroy
};

// ============================================================================
// Comb滤波器实现
// ============================================================================

static linx_audio_result_t comb_filter_init(comb_filter_t* comb, int size) {
    if (!comb || size <= 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    comb->buffer = calloc(size, sizeof(float));
    if (!comb->buffer) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    comb->buffer_size = size;
    comb->buffer_index = 0;
    comb->feedback = 0.0f;
    comb->filter_store = 0.0f;
    comb->damp1 = 0.0f;
    comb->damp2 = 0.0f;
    
    return LINX_AUDIO_SUCCESS;
}

static void comb_filter_cleanup(comb_filter_t* comb) {
    if (comb && comb->buffer) {
        free(comb->buffer);
        comb->buffer = NULL;
        comb->buffer_size = 0;
        comb->buffer_index = 0;
    }
}

static float comb_filter_process(comb_filter_t* comb, float input) {
    if (!comb || !comb->buffer) {
        return input;
    }
    
    float output = comb->buffer[comb->buffer_index];
    
    // 低通滤波器
    comb->filter_store = (output * comb->damp2) + (comb->filter_store * comb->damp1);
    
    comb->buffer[comb->buffer_index] = input + (comb->filter_store * comb->feedback);
    
    comb->buffer_index = (comb->buffer_index + 1) % comb->buffer_size;
    
    return output;
}

static void comb_filter_set_damp(comb_filter_t* comb, float val) {
    if (comb) {
        comb->damp1 = val;
        comb->damp2 = 1.0f - val;
    }
}

static void comb_filter_set_feedback(comb_filter_t* comb, float val) {
    if (comb) {
        comb->feedback = val;
    }
}

// ============================================================================
// Allpass滤波器实现
// ============================================================================

static linx_audio_result_t allpass_filter_init(allpass_filter_t* allpass, int size) {
    if (!allpass || size <= 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    allpass->buffer = calloc(size, sizeof(float));
    if (!allpass->buffer) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    allpass->buffer_size = size;
    allpass->buffer_index = 0;
    allpass->feedback = 0.5f;
    
    return LINX_AUDIO_SUCCESS;
}

static void allpass_filter_cleanup(allpass_filter_t* allpass) {
    if (allpass && allpass->buffer) {
        free(allpass->buffer);
        allpass->buffer = NULL;
        allpass->buffer_size = 0;
        allpass->buffer_index = 0;
    }
}

static float allpass_filter_process(allpass_filter_t* allpass, float input) {
    if (!allpass || !allpass->buffer) {
        return input;
    }
    
    float buffer_out = allpass->buffer[allpass->buffer_index];
    float output = -input + buffer_out;
    
    allpass->buffer[allpass->buffer_index] = input + (buffer_out * allpass->feedback);
    
    allpass->buffer_index = (allpass->buffer_index + 1) % allpass->buffer_size;
    
    return output;
}

static void allpass_filter_set_feedback(allpass_filter_t* allpass, float val) {
    if (allpass) {
        allpass->feedback = val;
    }
}

// ============================================================================
// 混响参数更新
// ============================================================================

static void reverb_update_parameters(reverb_plugin_t* reverb) {
    if (!reverb) {
        return;
    }
    
    reverb->wet1 = reverb->wet_level * (reverb->width / 2.0f + 0.5f);
    reverb->wet2 = reverb->wet_level * ((1.0f - reverb->width) / 2.0f);
    
    if (reverb->freeze_mode) {
        reverb->room_size1 = 1.0f;
        reverb->damp1 = 0.0f;
        reverb->gain = 0.0f;
    } else {
        reverb->room_size1 = reverb->room_size;
        reverb->damp1 = reverb->damping;
        reverb->gain = 0.015f;
    }
    
    reverb->room_size2 = reverb->room_size1 * REVERB_SCALE_ROOM + REVERB_OFFSET_ROOM;
    reverb->damp2 = reverb->damp1 * REVERB_SCALE_DAMP;
    
    // 更新所有comb滤波器
    for (int i = 0; i < REVERB_NUM_COMBS; i++) {
        comb_filter_set_feedback(&reverb->comb_L[i], reverb->room_size2);
        comb_filter_set_feedback(&reverb->comb_R[i], reverb->room_size2);
        comb_filter_set_damp(&reverb->comb_L[i], reverb->damp2);
        comb_filter_set_damp(&reverb->comb_R[i], reverb->damp2);
    }
}

static void reverb_clear_buffers(reverb_plugin_t* reverb) {
    if (!reverb) {
        return;
    }
    
    for (int i = 0; i < REVERB_NUM_COMBS; i++) {
        if (reverb->comb_L[i].buffer) {
            memset(reverb->comb_L[i].buffer, 0, reverb->comb_L[i].buffer_size * sizeof(float));
            reverb->comb_L[i].buffer_index = 0;
            reverb->comb_L[i].filter_store = 0.0f;
        }
        if (reverb->comb_R[i].buffer) {
            memset(reverb->comb_R[i].buffer, 0, reverb->comb_R[i].buffer_size * sizeof(float));
            reverb->comb_R[i].buffer_index = 0;
            reverb->comb_R[i].filter_store = 0.0f;
        }
    }
    
    for (int i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        if (reverb->allpass_L[i].buffer) {
            memset(reverb->allpass_L[i].buffer, 0, reverb->allpass_L[i].buffer_size * sizeof(float));
            reverb->allpass_L[i].buffer_index = 0;
        }
        if (reverb->allpass_R[i].buffer) {
            memset(reverb->allpass_R[i].buffer, 0, reverb->allpass_R[i].buffer_size * sizeof(float));
            reverb->allpass_R[i].buffer_index = 0;
        }
    }
}

// ============================================================================
// 插件接口实现
// ============================================================================

static linx_audio_result_t reverb_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    // 设置默认参数
    reverb->sample_rate = 44100;
    reverb->channels = 2;
    reverb->room_size = REVERB_INITIAL_ROOM;
    reverb->damping = REVERB_INITIAL_DAMP;
    reverb->wet_level = REVERB_INITIAL_WET;
    reverb->dry_level = REVERB_INITIAL_DRY;
    reverb->width = REVERB_INITIAL_WIDTH;
    reverb->bypass = false;
    reverb->freeze_mode = false;
    reverb->samples_processed = 0;
    reverb->initialized = false;
    
    // 计算缓冲区大小比例
    float scale = (float)reverb->sample_rate / 44100.0f;
    
    // 初始化comb滤波器
    for (int i = 0; i < REVERB_NUM_COMBS; i++) {
        int size_L = (int)(comb_tuning[i] * scale);
        int size_R = (int)((comb_tuning[i] + REVERB_STEREO_SPREAD) * scale);
        
        linx_audio_result_t result = comb_filter_init(&reverb->comb_L[i], size_L);
        if (result != LINX_AUDIO_SUCCESS) {
            goto cleanup;
        }
        
        result = comb_filter_init(&reverb->comb_R[i], size_R);
        if (result != LINX_AUDIO_SUCCESS) {
            comb_filter_cleanup(&reverb->comb_L[i]);
            goto cleanup;
        }
    }
    
    // 初始化allpass滤波器
    for (int i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        int size_L = (int)(allpass_tuning[i] * scale);
        int size_R = (int)((allpass_tuning[i] + REVERB_STEREO_SPREAD) * scale);
        
        linx_audio_result_t result = allpass_filter_init(&reverb->allpass_L[i], size_L);
        if (result != LINX_AUDIO_SUCCESS) {
            goto cleanup;
        }
        
        result = allpass_filter_init(&reverb->allpass_R[i], size_R);
        if (result != LINX_AUDIO_SUCCESS) {
            allpass_filter_cleanup(&reverb->allpass_L[i]);
            goto cleanup;
        }
        
        // 设置allpass反馈
        allpass_filter_set_feedback(&reverb->allpass_L[i], 0.5f);
        allpass_filter_set_feedback(&reverb->allpass_R[i], 0.5f);
    }
    
    // 更新参数
    reverb_update_parameters(reverb);
    reverb->initialized = true;
    
    return LINX_AUDIO_SUCCESS;
    
cleanup:
    // 清理已初始化的滤波器
    for (int i = 0; i < REVERB_NUM_COMBS; i++) {
        comb_filter_cleanup(&reverb->comb_L[i]);
        comb_filter_cleanup(&reverb->comb_R[i]);
    }
    for (int i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        allpass_filter_cleanup(&reverb->allpass_L[i]);
        allpass_filter_cleanup(&reverb->allpass_R[i]);
    }
    
    return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
}

static linx_audio_result_t reverb_deinitialize(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    if (reverb->initialized) {
        // 清理所有滤波器
        for (int i = 0; i < REVERB_NUM_COMBS; i++) {
            comb_filter_cleanup(&reverb->comb_L[i]);
            comb_filter_cleanup(&reverb->comb_R[i]);
        }
        for (int i = 0; i < REVERB_NUM_ALLPASSES; i++) {
            allpass_filter_cleanup(&reverb->allpass_L[i]);
            allpass_filter_cleanup(&reverb->allpass_R[i]);
        }
        reverb->initialized = false;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t reverb_start(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    // 清空所有缓冲区
    reverb_clear_buffers(reverb);
    reverb->samples_processed = 0;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t reverb_stop(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t reverb_process(linx_plugin_base_t* plugin,
                                         const linx_audio_buffer_t* input,
                                         linx_audio_buffer_t* output) {
    if (!plugin || !input || !output) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    // 检查缓冲区格式
    if (input->frame_count != output->frame_count ||
        input->channels != output->channels) {
        return LINX_AUDIO_ERROR_INVALID_FORMAT;
    }
    
    const float* in_samples = (const float*)input->data;
    float* out_samples = (float*)output->data;
    
    // 旁路模式
    if (reverb->bypass) {
        if (in_samples != out_samples) {
            memcpy(out_samples, in_samples, 
                   input->frame_count * input->channels * sizeof(float));
        }
        return LINX_AUDIO_SUCCESS;
    }
    
    // 处理每个样本帧
    for (uint32_t frame = 0; frame < input->frame_count; frame++) {
        float input_L, input_R;
        float output_L = 0.0f, output_R = 0.0f;
        
        // 获取输入样本
        if (input->channels == 1) {
            input_L = input_R = in_samples[frame];
        } else {
            input_L = in_samples[frame * 2];
            input_R = in_samples[frame * 2 + 1];
        }
        
        // 混合输入为单声道
        float input_mix = (input_L + input_R) * reverb->gain;
        
        // 通过comb滤波器
        for (int i = 0; i < REVERB_NUM_COMBS; i++) {
            output_L += comb_filter_process(&reverb->comb_L[i], input_mix);
            output_R += comb_filter_process(&reverb->comb_R[i], input_mix);
        }
        
        // 通过allpass滤波器
        for (int i = 0; i < REVERB_NUM_ALLPASSES; i++) {
            output_L = allpass_filter_process(&reverb->allpass_L[i], output_L);
            output_R = allpass_filter_process(&reverb->allpass_R[i], output_R);
        }
        
        // 混合干湿信号
        float final_L = output_L * reverb->wet1 + output_R * reverb->wet2 + input_L * reverb->dry_level;
        float final_R = output_R * reverb->wet1 + output_L * reverb->wet2 + input_R * reverb->dry_level;
        
        // 防止削波
        if (final_L > 1.0f) final_L = 1.0f;
        else if (final_L < -1.0f) final_L = -1.0f;
        if (final_R > 1.0f) final_R = 1.0f;
        else if (final_R < -1.0f) final_R = -1.0f;
        
        // 输出样本
        if (output->channels == 1) {
            out_samples[frame] = (final_L + final_R) * 0.5f;
        } else {
            out_samples[frame * 2] = final_L;
            out_samples[frame * 2 + 1] = final_R;
        }
    }
    
    reverb->samples_processed += input->frame_count * input->channels;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t reverb_set_parameter(linx_plugin_base_t* plugin, 
                                               const char* name, const char* value) {
    if (!plugin || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        reverb->bypass = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "room_size") == 0) {
        float room = atof(value);
        if (room >= 0.0f && room <= 1.0f) {
            reverb->room_size = room;
            reverb_update_parameters(reverb);
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "damping") == 0) {
        float damp = atof(value);
        if (damp >= 0.0f && damp <= 1.0f) {
            reverb->damping = damp;
            reverb_update_parameters(reverb);
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "wet_level") == 0) {
        float wet = atof(value);
        if (wet >= 0.0f && wet <= 1.0f) {
            reverb->wet_level = wet;
            reverb_update_parameters(reverb);
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "dry_level") == 0) {
        float dry = atof(value);
        if (dry >= 0.0f && dry <= 1.0f) {
            reverb->dry_level = dry;
            reverb_update_parameters(reverb);
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "width") == 0) {
        float width = atof(value);
        if (width >= 0.0f && width <= 1.0f) {
            reverb->width = width;
            reverb_update_parameters(reverb);
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strcmp(name, "freeze") == 0) {
        reverb->freeze_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        reverb_update_parameters(reverb);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t reverb_get_parameter(linx_plugin_base_t* plugin, 
                                               const char* name, char* value, size_t size) {
    if (!plugin || !name || !value || size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        snprintf(value, size, "%s", reverb->bypass ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "room_size") == 0) {
        snprintf(value, size, "%.3f", reverb->room_size);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "damping") == 0) {
        snprintf(value, size, "%.3f", reverb->damping);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "wet_level") == 0) {
        snprintf(value, size, "%.3f", reverb->wet_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "dry_level") == 0) {
        snprintf(value, size, "%.3f", reverb->dry_level);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "width") == 0) {
        snprintf(value, size, "%.3f", reverb->width);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "freeze") == 0) {
        snprintf(value, size, "%s", reverb->freeze_mode ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "samples_processed") == 0) {
        snprintf(value, size, "%llu", (unsigned long long)reverb->samples_processed);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t reverb_set_format(linx_plugin_base_t* plugin, 
                                            const linx_audio_format_info_t* format) {
    if (!plugin || !format) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    reverb_plugin_t* reverb = (reverb_plugin_t*)plugin;
    
    // 如果采样率改变，需要重新初始化
    if (format->sample_rate != reverb->sample_rate) {
        reverb_deinitialize(plugin);
        reverb->sample_rate = format->sample_rate;
        reverb->channels = format->channels;
        return reverb_initialize(plugin, NULL);
    }
    
    reverb->channels = format->channels;
    
    return LINX_AUDIO_SUCCESS;
}

static void reverb_destroy(linx_plugin_base_t* plugin) {
    if (plugin) {
        reverb_deinitialize(plugin);
        free(plugin);
    }
}

// ============================================================================
// 插件工厂函数
// ============================================================================

linx_plugin_base_t* create_reverb_plugin(const linx_plugin_config_t* config) {
    reverb_plugin_t* reverb = malloc(sizeof(reverb_plugin_t));
    if (!reverb) {
        return NULL;
    }
    
    memset(reverb, 0, sizeof(reverb_plugin_t));
    
    // 初始化基础插件结构
    linx_plugin_metadata_t metadata = {
        .name = "Reverb",
        .version = {1, 0, 0},
        .description = "Freeverb-based reverb effect",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    };
    
    linx_audio_result_t result = linx_plugin_base_init(&reverb->base, &reverb_vtable, &metadata);
    if (result != LINX_AUDIO_SUCCESS) {
        free(reverb);
        return NULL;
    }
    
    // 初始化插件
    if (reverb_initialize(&reverb->base, config) != LINX_AUDIO_SUCCESS) {
        free(reverb);
        return NULL;
    }
    
    return &reverb->base;
}

void destroy_reverb_plugin(linx_plugin_base_t* plugin) {
    if (plugin) {
        reverb_destroy(plugin);
    }
}

linx_audio_result_t get_reverb_plugin_metadata(linx_plugin_metadata_t* metadata) {
    if (!metadata) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    metadata->name = "Reverb";
    metadata->version.major = 1;
    metadata->version.minor = 0;
    metadata->version.patch = 0;
    metadata->description = "Freeverb-based reverb effect";
    metadata->author = "LinxOS Audio Team";
    metadata->license = "MIT";
    metadata->type = LINX_PLUGIN_TYPE_EFFECT;
    
    return LINX_AUDIO_SUCCESS;
}

// 插件描述符
LINX_PLUGIN_IMPLEMENT(reverb, {
    .metadata = {
        .name = "Reverb",
        .version = {1, 0, 0},
        .description = "Freeverb-based reverb effect",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    },
    .create_func = create_reverb_plugin,
    .destroy_func = destroy_reverb_plugin,
    .get_metadata_func = get_reverb_plugin_metadata
});