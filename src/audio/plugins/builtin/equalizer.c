/**
 * @file equalizer.c
 * @brief 均衡器插件实现
 * @details 提供多频段音频均衡功能
 */

#include "../plugin_interface.h"
#include "../../core/types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// 常量定义
// ============================================================================

#define EQ_MAX_BANDS 10
#define EQ_DEFAULT_BANDS 5

// ============================================================================
// 滤波器类型
// ============================================================================

typedef enum {
    FILTER_TYPE_LOWPASS = 0,
    FILTER_TYPE_HIGHPASS,
    FILTER_TYPE_BANDPASS,
    FILTER_TYPE_NOTCH,
    FILTER_TYPE_PEAKING,
    FILTER_TYPE_LOWSHELF,
    FILTER_TYPE_HIGHSHELF
} filter_type_t;

// ============================================================================
// 双二阶滤波器结构
// ============================================================================

typedef struct {
    // 滤波器系数
    double b0, b1, b2;  // 前馈系数
    double a1, a2;      // 反馈系数
    
    // 延迟线（每个声道）
    double x1[8], x2[8]; // 输入延迟
    double y1[8], y2[8]; // 输出延迟
    
    // 滤波器参数
    filter_type_t type;
    double frequency;   // 中心频率
    double gain_db;     // 增益（分贝）
    double q_factor;    // 品质因子
    double sample_rate; // 采样率
    
    bool enabled;       // 是否启用
} biquad_filter_t;

// ============================================================================
// 均衡器插件私有数据结构
// ============================================================================

typedef struct {
    linx_plugin_base_t base;
    
    // 滤波器组
    biquad_filter_t bands[EQ_MAX_BANDS];
    uint32_t band_count;
    
    // 音频格式
    uint32_t sample_rate;
    uint32_t channels;
    
    // 全局参数
    float master_gain;
    bool bypass;
    
    // 统计信息
    uint64_t samples_processed;
} equalizer_plugin_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static void calculate_biquad_coefficients(biquad_filter_t* filter);
static double process_biquad_sample(biquad_filter_t* filter, double input, uint32_t channel);
static void reset_biquad_filter(biquad_filter_t* filter);

static linx_audio_result_t eq_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
static linx_audio_result_t eq_deinitialize(linx_plugin_base_t* plugin);
static linx_audio_result_t eq_start(linx_plugin_base_t* plugin);
static linx_audio_result_t eq_stop(linx_plugin_base_t* plugin);
static linx_audio_result_t eq_process(linx_plugin_base_t* plugin,
                                     const linx_audio_buffer_t* input,
                                     linx_audio_buffer_t* output);
static linx_audio_result_t eq_set_parameter(linx_plugin_base_t* plugin, 
                                           const char* name, const char* value);
static linx_audio_result_t eq_get_parameter(linx_plugin_base_t* plugin, 
                                           const char* name, char* value, size_t size);
static linx_audio_result_t eq_set_format(linx_plugin_base_t* plugin, 
                                        const linx_audio_format_info_t* format);
static void eq_destroy(linx_plugin_base_t* plugin);

// ============================================================================
// 插件虚函数表
// ============================================================================

static const linx_plugin_vtable_t eq_vtable = {
    .initialize = eq_initialize,
    .deinitialize = eq_deinitialize,
    .start = eq_start,
    .stop = eq_stop,
    .pause = NULL,
    .resume = NULL,
    .reset = NULL,
    .process = eq_process,
    .process_inplace = NULL,
    .set_config = NULL,
    .get_config = NULL,
    .set_parameter = eq_set_parameter,
    .get_parameter = eq_get_parameter,
    .set_format = eq_set_format,
    .get_format = NULL,
    .supports_format = NULL,
    .get_latency = NULL,
    .get_tail_time = NULL,
    .get_state = NULL,
    .get_info = NULL,
    .on_event = NULL,
    .destroy = eq_destroy
};

// ============================================================================
// 双二阶滤波器实现
// ============================================================================

static void calculate_biquad_coefficients(biquad_filter_t* filter) {
    if (!filter || filter->sample_rate <= 0) {
        return;
    }
    
    double omega = 2.0 * M_PI * filter->frequency / filter->sample_rate;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    double alpha = sin_omega / (2.0 * filter->q_factor);
    double A = pow(10.0, filter->gain_db / 40.0);
    double beta = sqrt(A) / filter->q_factor;
    
    switch (filter->type) {
        case FILTER_TYPE_LOWPASS:
            filter->b0 = (1.0 - cos_omega) / 2.0;
            filter->b1 = 1.0 - cos_omega;
            filter->b2 = (1.0 - cos_omega) / 2.0;
            filter->a1 = -2.0 * cos_omega;
            filter->a2 = 1.0 - alpha;
            break;
            
        case FILTER_TYPE_HIGHPASS:
            filter->b0 = (1.0 + cos_omega) / 2.0;
            filter->b1 = -(1.0 + cos_omega);
            filter->b2 = (1.0 + cos_omega) / 2.0;
            filter->a1 = -2.0 * cos_omega;
            filter->a2 = 1.0 - alpha;
            break;
            
        case FILTER_TYPE_PEAKING:
            filter->b0 = 1.0 + alpha * A;
            filter->b1 = -2.0 * cos_omega;
            filter->b2 = 1.0 - alpha * A;
            filter->a1 = -2.0 * cos_omega;
            filter->a2 = 1.0 - alpha / A;
            break;
            
        case FILTER_TYPE_LOWSHELF:
            filter->b0 = A * ((A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega);
            filter->b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_omega);
            filter->b2 = A * ((A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega);
            filter->a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_omega);
            filter->a2 = (A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega;
            break;
            
        case FILTER_TYPE_HIGHSHELF:
            filter->b0 = A * ((A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega);
            filter->b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_omega);
            filter->b2 = A * ((A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega);
            filter->a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_omega);
            filter->a2 = (A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega;
            break;
            
        default:
            // 直通滤波器
            filter->b0 = 1.0;
            filter->b1 = 0.0;
            filter->b2 = 0.0;
            filter->a1 = 0.0;
            filter->a2 = 0.0;
            break;
    }
    
    // 归一化系数
    double a0 = 1.0 + alpha;
    filter->b0 /= a0;
    filter->b1 /= a0;
    filter->b2 /= a0;
    filter->a1 /= a0;
    filter->a2 /= a0;
}

static double process_biquad_sample(biquad_filter_t* filter, double input, uint32_t channel) {
    if (!filter || !filter->enabled || channel >= 8) {
        return input;
    }
    
    // 双二阶滤波器方程：y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    double output = filter->b0 * input + 
                   filter->b1 * filter->x1[channel] + 
                   filter->b2 * filter->x2[channel] -
                   filter->a1 * filter->y1[channel] - 
                   filter->a2 * filter->y2[channel];
    
    // 更新延迟线
    filter->x2[channel] = filter->x1[channel];
    filter->x1[channel] = input;
    filter->y2[channel] = filter->y1[channel];
    filter->y1[channel] = output;
    
    return output;
}

static void reset_biquad_filter(biquad_filter_t* filter) {
    if (!filter) {
        return;
    }
    
    memset(filter->x1, 0, sizeof(filter->x1));
    memset(filter->x2, 0, sizeof(filter->x2));
    memset(filter->y1, 0, sizeof(filter->y1));
    memset(filter->y2, 0, sizeof(filter->y2));
}

// ============================================================================
// 插件接口实现
// ============================================================================

static linx_audio_result_t eq_initialize(linx_plugin_base_t* plugin, const linx_plugin_config_t* config) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    // 设置默认参数
    eq->band_count = EQ_DEFAULT_BANDS;
    eq->sample_rate = 44100;
    eq->channels = 2;
    eq->master_gain = 1.0f;
    eq->bypass = false;
    eq->samples_processed = 0;
    
    // 初始化默认频段
    double frequencies[] = {100.0, 300.0, 1000.0, 3000.0, 10000.0};
    filter_type_t types[] = {
        FILTER_TYPE_LOWSHELF,
        FILTER_TYPE_PEAKING,
        FILTER_TYPE_PEAKING,
        FILTER_TYPE_PEAKING,
        FILTER_TYPE_HIGHSHELF
    };
    
    for (uint32_t i = 0; i < eq->band_count; i++) {
        biquad_filter_t* band = &eq->bands[i];
        memset(band, 0, sizeof(biquad_filter_t));
        
        band->type = types[i];
        band->frequency = frequencies[i];
        band->gain_db = 0.0;
        band->q_factor = 1.0;
        band->sample_rate = eq->sample_rate;
        band->enabled = true;
        
        calculate_biquad_coefficients(band);
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t eq_deinitialize(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t eq_start(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    // 重置所有滤波器
    for (uint32_t i = 0; i < eq->band_count; i++) {
        reset_biquad_filter(&eq->bands[i]);
    }
    
    eq->samples_processed = 0;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t eq_stop(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t eq_process(linx_plugin_base_t* plugin,
                                     const linx_audio_buffer_t* input,
                                     linx_audio_buffer_t* output) {
    if (!plugin || !input || !output) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    // 检查缓冲区格式
    if (input->frame_count != output->frame_count ||
        input->channels != output->channels) {
        return LINX_AUDIO_ERROR_INVALID_FORMAT;
    }
    
    const float* in_samples = (const float*)input->data;
    float* out_samples = (float*)output->data;
    uint32_t total_samples = input->frame_count * input->channels;
    
    // 旁路模式
    if (eq->bypass) {
        if (in_samples != out_samples) {
            memcpy(out_samples, in_samples, total_samples * sizeof(float));
        }
        return LINX_AUDIO_SUCCESS;
    }
    
    // 处理每个样本
    for (uint32_t frame = 0; frame < input->frame_count; frame++) {
        for (uint32_t ch = 0; ch < input->channels; ch++) {
            uint32_t sample_idx = frame * input->channels + ch;
            double sample = in_samples[sample_idx];
            
            // 通过所有频段滤波器
            for (uint32_t band = 0; band < eq->band_count; band++) {
                sample = process_biquad_sample(&eq->bands[band], sample, ch);
            }
            
            // 应用主增益
            sample *= eq->master_gain;
            
            // 防止削波
            if (sample > 1.0) sample = 1.0;
            else if (sample < -1.0) sample = -1.0;
            
            out_samples[sample_idx] = (float)sample;
        }
    }
    
    eq->samples_processed += total_samples;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t eq_set_parameter(linx_plugin_base_t* plugin, 
                                           const char* name, const char* value) {
    if (!plugin || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        eq->bypass = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "master_gain") == 0) {
        float gain = atof(value);
        if (gain >= 0.0f && gain <= 10.0f) {
            eq->master_gain = gain;
            return LINX_AUDIO_SUCCESS;
        }
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    else if (strncmp(name, "band", 4) == 0) {
        // 解析频段参数：band0_gain, band0_freq, band0_q
        int band_num = atoi(name + 4);
        if (band_num < 0 || band_num >= (int)eq->band_count) {
            return LINX_AUDIO_ERROR_INVALID_PARAM;
        }
        
        biquad_filter_t* band = &eq->bands[band_num];
        const char* param = strchr(name + 4, '_');
        if (!param) {
            return LINX_AUDIO_ERROR_INVALID_PARAM;
        }
        param++; // 跳过下划线
        
        if (strcmp(param, "gain") == 0) {
            double gain = atof(value);
            if (gain >= -20.0 && gain <= 20.0) {
                band->gain_db = gain;
                calculate_biquad_coefficients(band);
                return LINX_AUDIO_SUCCESS;
            }
        }
        else if (strcmp(param, "freq") == 0) {
            double freq = atof(value);
            if (freq >= 20.0 && freq <= eq->sample_rate / 2.0) {
                band->frequency = freq;
                calculate_biquad_coefficients(band);
                return LINX_AUDIO_SUCCESS;
            }
        }
        else if (strcmp(param, "q") == 0) {
            double q = atof(value);
            if (q >= 0.1 && q <= 10.0) {
                band->q_factor = q;
                calculate_biquad_coefficients(band);
                return LINX_AUDIO_SUCCESS;
            }
        }
        else if (strcmp(param, "enabled") == 0) {
            band->enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            return LINX_AUDIO_SUCCESS;
        }
        
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t eq_get_parameter(linx_plugin_base_t* plugin, 
                                           const char* name, char* value, size_t size) {
    if (!plugin || !name || !value || size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    if (strcmp(name, "bypass") == 0) {
        snprintf(value, size, "%s", eq->bypass ? "true" : "false");
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "master_gain") == 0) {
        snprintf(value, size, "%.3f", eq->master_gain);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "band_count") == 0) {
        snprintf(value, size, "%u", eq->band_count);
        return LINX_AUDIO_SUCCESS;
    }
    else if (strcmp(name, "samples_processed") == 0) {
        snprintf(value, size, "%llu", (unsigned long long)eq->samples_processed);
        return LINX_AUDIO_SUCCESS;
    }
    
    return LINX_AUDIO_ERROR_NOT_SUPPORTED;
}

static linx_audio_result_t eq_set_format(linx_plugin_base_t* plugin, 
                                        const linx_audio_format_info_t* format) {
    if (!plugin || !format) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    equalizer_plugin_t* eq = (equalizer_plugin_t*)plugin;
    
    // 更新采样率
    if (format->sample_rate != eq->sample_rate) {
        eq->sample_rate = format->sample_rate;
        eq->channels = format->channels;
        
        // 重新计算所有滤波器系数
        for (uint32_t i = 0; i < eq->band_count; i++) {
            eq->bands[i].sample_rate = eq->sample_rate;
            calculate_biquad_coefficients(&eq->bands[i]);
            reset_biquad_filter(&eq->bands[i]);
        }
    }
    
    return LINX_AUDIO_SUCCESS;
}

static void eq_destroy(linx_plugin_base_t* plugin) {
    if (plugin) {
        free(plugin);
    }
}

// ============================================================================
// 插件工厂函数
// ============================================================================

linx_plugin_base_t* create_equalizer_plugin(const linx_plugin_config_t* config) {
    equalizer_plugin_t* eq = malloc(sizeof(equalizer_plugin_t));
    if (!eq) {
        return NULL;
    }
    
    memset(eq, 0, sizeof(equalizer_plugin_t));
    
    // 初始化基础插件结构
    linx_plugin_metadata_t metadata = {
        .name = "Equalizer",
        .version = {1, 0, 0},
        .description = "Multi-band audio equalizer",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    };
    
    linx_audio_result_t result = linx_plugin_base_init(&eq->base, &eq_vtable, &metadata);
    if (result != LINX_AUDIO_SUCCESS) {
        free(eq);
        return NULL;
    }
    
    // 初始化插件
    if (eq_initialize(&eq->base, config) != LINX_AUDIO_SUCCESS) {
        free(eq);
        return NULL;
    }
    
    return &eq->base;
}

void destroy_equalizer_plugin(linx_plugin_base_t* plugin) {
    if (plugin) {
        eq_deinitialize(plugin);
        eq_destroy(plugin);
    }
}

linx_audio_result_t get_equalizer_plugin_metadata(linx_plugin_metadata_t* metadata) {
    if (!metadata) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    metadata->name = "Equalizer";
    metadata->version.major = 1;
    metadata->version.minor = 0;
    metadata->version.patch = 0;
    metadata->description = "Multi-band audio equalizer";
    metadata->author = "LinxOS Audio Team";
    metadata->license = "MIT";
    metadata->type = LINX_PLUGIN_TYPE_EFFECT;
    
    return LINX_AUDIO_SUCCESS;
}

// 插件描述符
LINX_PLUGIN_IMPLEMENT(equalizer, {
    .metadata = {
        .name = "Equalizer",
        .version = {1, 0, 0},
        .description = "Multi-band audio equalizer",
        .author = "LinxOS Audio Team",
        .license = "MIT",
        .type = LINX_PLUGIN_TYPE_EFFECT
    },
    .create_func = create_equalizer_plugin,
    .destroy_func = destroy_equalizer_plugin,
    .get_metadata_func = get_equalizer_plugin_metadata
});