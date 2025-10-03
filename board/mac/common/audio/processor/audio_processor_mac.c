#include "audio_processor_mac.h"
#include "linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Note: Accelerate framework inclusion removed due to path issues
// Can be re-enabled when proper include paths are configured

/**
 * @file audio_processor_mac.c
 * @brief Mac平台音频处理器实现
 * @details 基于Core Audio和Accelerate框架的高性能音频处理实现
 */

#define MAC_PROCESSOR_TAG "MacAudioProcessor"

// 前向声明
static audio_processor_error_t mac_processor_initialize(AudioProcessor* self,
                                                        const audio_processor_config_t* config,
                                                        audio_codec_t* codec);
static audio_processor_error_t mac_processor_start(AudioProcessor* self);
static audio_processor_error_t mac_processor_stop(AudioProcessor* self);
static audio_processor_error_t mac_processor_feed(AudioProcessor* self, 
                                                  const int16_t* data, size_t size);
static size_t mac_processor_get_feed_size(const AudioProcessor* self);
static audio_processor_error_t mac_processor_enable_device_aec(AudioProcessor* self, bool enable);
static audio_processor_error_t mac_processor_set_output_callback(AudioProcessor* self,
                                                                 audio_processor_output_callback_t callback,
                                                                 void* user_data);
static audio_processor_error_t mac_processor_set_vad_callback(AudioProcessor* self,
                                                             audio_processor_vad_callback_t callback,
                                                             void* user_data);
static bool mac_processor_get_vad_status(const AudioProcessor* self);
static audio_processor_error_t mac_processor_reset(AudioProcessor* self);
static int mac_processor_get_delay_ms(const AudioProcessor* self);
static void mac_processor_destroy(AudioProcessor* self);

// 虚函数表
static const AudioProcessorVTable mac_processor_vtable = {
    .initialize = mac_processor_initialize,
    .start = mac_processor_start,
    .stop = mac_processor_stop,
    .feed = mac_processor_feed,
    .get_feed_size = mac_processor_get_feed_size,
    .enable_device_aec = mac_processor_enable_device_aec,
    .set_output_callback = mac_processor_set_output_callback,
    .set_vad_callback = mac_processor_set_vad_callback,
    .get_vad_status = mac_processor_get_vad_status,
    .reset = mac_processor_reset,
    .get_delay_ms = mac_processor_get_delay_ms,
    .destroy = mac_processor_destroy
};

// =============================================================================
// 公共API实现
// =============================================================================

AudioProcessor* audio_processor_mac_create(void) {
    LINX_LOGI(MAC_PROCESSOR_TAG, "创建Mac音频处理器");
    
    AudioProcessor* processor = (AudioProcessor*)calloc(1, sizeof(AudioProcessor));
    if (!processor) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配AudioProcessor内存失败");
        return NULL;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)calloc(1, sizeof(MacAudioProcessorData));
    if (!data) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配MacAudioProcessorData内存失败");
        free(processor);
        return NULL;
    }
    
    // 初始化基本结构
    processor->vtable = &mac_processor_vtable;
    processor->private_data = data;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&data->processing_mutex, NULL) != 0) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "初始化互斥锁失败");
        free(data);
        free(processor);
        return NULL;
    }
    
    if (pthread_cond_init(&data->processing_cond, NULL) != 0) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "初始化条件变量失败");
        pthread_mutex_destroy(&data->processing_mutex);
        free(data);
        free(processor);
        return NULL;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器创建成功");
    return processor;
}

void audio_processor_mac_destroy(AudioProcessor* processor) {
    if (!processor) {
        return;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "销毁Mac音频处理器");
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)processor->private_data;
    if (data) {
        // 停止处理器
        if (data->started) {
            mac_processor_stop(processor);
        }
        
        // 清理缓冲区
        if (data->input_buffer) {
            free(data->input_buffer);
        }
        if (data->output_buffer) {
            free(data->output_buffer);
        }
        if (data->processing_buffer) {
            free(data->processing_buffer);
        }
        
        // 清理VAD状态
        if (data->vad_state.energy_history) {
            free(data->vad_state.energy_history);
        }
        if (data->vad_state.zcr_history) {
            free(data->vad_state.zcr_history);
        }
        
        // 清理AEC状态
        if (data->aec_state.filter_coeffs) {
            free(data->aec_state.filter_coeffs);
        }
        if (data->aec_state.reference_buffer) {
            free(data->aec_state.reference_buffer);
        }
        if (data->aec_state.error_buffer) {
            free(data->aec_state.error_buffer);
        }
        
        // 清理NS状态
        if (data->ns_state.noise_spectrum) {
            free(data->ns_state.noise_spectrum);
        }
        if (data->ns_state.signal_spectrum) {
            free(data->ns_state.signal_spectrum);
        }
        if (data->ns_state.gain_factors) {
            free(data->ns_state.gain_factors);
        }
        
        // 销毁同步对象
        pthread_cond_destroy(&data->processing_cond);
        pthread_mutex_destroy(&data->processing_mutex);
        
        free(data);
    }
    
    free(processor);
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器销毁完成");
}

// =============================================================================
// 虚函数表实现
// =============================================================================

static audio_processor_error_t mac_processor_initialize(AudioProcessor* self,
                                                        const audio_processor_config_t* config,
                                                        audio_codec_t* codec) {
    if (!self || !config) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "初始化参数无效");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "处理器数据为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "初始化Mac音频处理器，采样率: %d, 声道: %d, 帧长: %dms",
              config->sample_rate, config->channels, config->frame_duration_ms);
    
    // 保存配置
    data->config = *config;
    data->codec = codec;
    
    // 计算缓冲区大小
    data->buffer_size = (config->sample_rate * config->frame_duration_ms) / 1000;
    
    // 分配缓冲区
    data->input_buffer = (int16_t*)calloc(data->buffer_size, sizeof(int16_t));
    data->output_buffer = (int16_t*)calloc(data->buffer_size, sizeof(int16_t));
    data->processing_buffer = (float*)calloc(data->buffer_size, sizeof(float));
    
    if (!data->input_buffer || !data->output_buffer || !data->processing_buffer) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配音频缓冲区失败");
        return AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC;
    }
    
    // 初始化各个组件
    if (config->enable_vad) {
        if (mac_vad_init(data) != 0) {
            LINX_LOGE(MAC_PROCESSOR_TAG, "初始化VAD失败");
            return AUDIO_PROCESSOR_ERROR_PROCESSING;
        }
        LINX_LOGI(MAC_PROCESSOR_TAG, "VAD初始化成功");
    }
    
    if (config->enable_aec) {
        if (mac_aec_init(data) != 0) {
            LINX_LOGE(MAC_PROCESSOR_TAG, "初始化AEC失败");
            return AUDIO_PROCESSOR_ERROR_PROCESSING;
        }
        LINX_LOGI(MAC_PROCESSOR_TAG, "AEC初始化成功");
    }
    
    if (config->enable_ns) {
        if (mac_ns_init(data) != 0) {
            LINX_LOGE(MAC_PROCESSOR_TAG, "初始化NS失败");
            return AUDIO_PROCESSOR_ERROR_PROCESSING;
        }
        LINX_LOGI(MAC_PROCESSOR_TAG, "NS初始化成功");
    }
    
    data->initialized = true;
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器初始化完成");
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t mac_processor_start(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "处理器未初始化");
        return AUDIO_PROCESSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (data->started) {
        LINX_LOGW(MAC_PROCESSOR_TAG, "处理器已经启动");
        return AUDIO_PROCESSOR_ERROR_ALREADY_STARTED;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "启动Mac音频处理器");
    
    pthread_mutex_lock(&data->processing_mutex);
    data->started = true;
    data->frames_processed = 0;
    data->vad_speech_frames = 0;
    data->vad_silence_frames = 0;
    pthread_mutex_unlock(&data->processing_mutex);
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器启动成功");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t mac_processor_stop(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!data->started) {
        LINX_LOGW(MAC_PROCESSOR_TAG, "处理器未启动");
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "停止Mac音频处理器");
    
    pthread_mutex_lock(&data->processing_mutex);
    data->started = false;
    pthread_cond_broadcast(&data->processing_cond);
    pthread_mutex_unlock(&data->processing_mutex);
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器停止成功，处理帧数: %llu, 语音帧: %llu, 静音帧: %llu",
              data->frames_processed, data->vad_speech_frames, data->vad_silence_frames);
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t mac_processor_feed(AudioProcessor* self, 
                                                  const int16_t* data_input, size_t size) {
    if (!self || !data_input || size == 0) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data || !data->started) {
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }
    
    pthread_mutex_lock(&data->processing_mutex);
    
    if (!data->started) {
        pthread_mutex_unlock(&data->processing_mutex);
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }
    
    // 复制输入数据到内部缓冲区
    size_t samples_to_process = (size < data->buffer_size) ? size : data->buffer_size;
    memcpy(data->input_buffer, data_input, samples_to_process * sizeof(int16_t));
    memcpy(data->output_buffer, data->input_buffer, samples_to_process * sizeof(int16_t));
    
    // VAD处理
        mac_vad_state_enum_t vad_state = MAC_VAD_STATE_SILENCE;
    if (data->config.enable_vad && data->vad_state.enabled) {
        vad_state = mac_vad_process(data, data->input_buffer, samples_to_process);
        
        // 更新统计
        if (vad_state == MAC_VAD_STATE_SPEECH) {
            data->vad_speech_frames++;
        } else {
            data->vad_silence_frames++;
        }
        
        // 调用VAD回调
        if (data->vad_callback) {
            bool speaking = (vad_state == MAC_VAD_STATE_SPEECH);
            static bool last_speaking = false;
            if (speaking != last_speaking) {
                data->vad_callback(speaking, data->vad_callback_user_data);
                last_speaking = speaking;
            }
        }
    }
    
    // AEC处理
    if (data->config.enable_aec && data->aec_state.enabled) {
        // 这里假设没有参考信号，实际应用中需要提供
        mac_aec_process(data, data->input_buffer, NULL, data->output_buffer, samples_to_process);
    }
    
    // 噪声抑制处理
    if (data->config.enable_ns && data->ns_state.enabled) {
        mac_ns_process(data, data->output_buffer, data->output_buffer, samples_to_process);
    }
    
    // 调用输出回调
    if (data->output_callback) {
        data->output_callback(data->output_buffer, samples_to_process, data->output_callback_user_data);
    }
    
    data->frames_processed++;
    
    pthread_mutex_unlock(&data->processing_mutex);
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static size_t mac_processor_get_feed_size(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }
    
    return data->buffer_size;
}

static audio_processor_error_t mac_processor_enable_device_aec(AudioProcessor* self, bool enable) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "%s设备AEC", enable ? "启用" : "禁用");
    
    pthread_mutex_lock(&data->processing_mutex);
    data->aec_state.enabled = enable;
    pthread_mutex_unlock(&data->processing_mutex);
    
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t mac_processor_set_output_callback(AudioProcessor* self,
                                                                 audio_processor_output_callback_t callback,
                                                                 void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&data->processing_mutex);
    data->output_callback = callback;
    data->output_callback_user_data = user_data;
    pthread_mutex_unlock(&data->processing_mutex);
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "设置输出回调函数");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t mac_processor_set_vad_callback(AudioProcessor* self,
                                                             audio_processor_vad_callback_t callback,
                                                             void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&data->processing_mutex);
    data->vad_callback = callback;
    data->vad_callback_user_data = user_data;
    pthread_mutex_unlock(&data->processing_mutex);
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "设置VAD回调函数");
    return AUDIO_PROCESSOR_SUCCESS;
}

static bool mac_processor_get_vad_status(const AudioProcessor* self) {
    if (!self) {
        return false;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data || !data->config.enable_vad) {
        return false;
    }
    
    return (data->vad_state.current_state == MAC_VAD_STATE_SPEECH);
}

static audio_processor_error_t mac_processor_reset(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "重置Mac音频处理器");
    
    pthread_mutex_lock(&data->processing_mutex);
    
    // 重置统计信息
    data->frames_processed = 0;
    data->vad_speech_frames = 0;
    data->vad_silence_frames = 0;
    
    // 重置VAD状态
    if (data->vad_state.enabled) {
        data->vad_state.current_state = MAC_VAD_STATE_SILENCE;
        data->vad_state.hangover_counter = 0;
        data->vad_state.history_pos = 0;
        if (data->vad_state.energy_history) {
            memset(data->vad_state.energy_history, 0, 
                   data->vad_state.history_size * sizeof(float));
        }
        if (data->vad_state.zcr_history) {
            memset(data->vad_state.zcr_history, 0, 
                   data->vad_state.history_size * sizeof(float));
        }
    }
    
    // 重置AEC状态
    if (data->aec_state.enabled) {
        data->aec_state.buffer_pos = 0;
        if (data->aec_state.filter_coeffs) {
            memset(data->aec_state.filter_coeffs, 0, 
                   data->aec_state.filter_length * sizeof(float));
        }
        if (data->aec_state.reference_buffer) {
            memset(data->aec_state.reference_buffer, 0, 
                   data->aec_state.filter_length * sizeof(float));
        }
        if (data->aec_state.error_buffer) {
            memset(data->aec_state.error_buffer, 0, 
                   data->aec_state.filter_length * sizeof(float));
        }
    }
    
    pthread_mutex_unlock(&data->processing_mutex);
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "Mac音频处理器重置完成");
    return AUDIO_PROCESSOR_SUCCESS;
}

static int mac_processor_get_delay_ms(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }
    
    MacAudioProcessorData* data = (MacAudioProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }
    
    // 计算处理延迟（基于帧大小）
    return data->config.frame_duration_ms;
}

static void mac_processor_destroy(AudioProcessor* self) {
    audio_processor_mac_destroy(self);
}

// =============================================================================
// VAD实现
// =============================================================================

int mac_vad_init(MacAudioProcessorData* data) {
    if (!data) {
        return -1;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "初始化VAD组件");
    
    data->vad_state.history_size = MAC_PROCESSOR_VAD_HISTORY_SIZE;
    data->vad_state.energy_history = (float*)calloc(data->vad_state.history_size, sizeof(float));
    data->vad_state.zcr_history = (float*)calloc(data->vad_state.history_size, sizeof(float));
    
    if (!data->vad_state.energy_history || !data->vad_state.zcr_history) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配VAD历史缓冲区失败");
        return -1;
    }
    
    // 设置默认阈值
    data->vad_state.energy_threshold = data->config.vad_threshold > 0 ? 
                                      data->config.vad_threshold : 0.01f;
    data->vad_state.zcr_threshold = 0.3f;
    data->vad_state.hangover_time = 0.5f; // 500ms
    data->vad_state.hangover_counter = 0;
    data->vad_state.history_pos = 0;
    data->vad_state.current_state = MAC_VAD_STATE_SILENCE;
    data->vad_state.enabled = true;
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "VAD初始化完成，能量阈值: %.3f, 过零率阈值: %.3f",
              data->vad_state.energy_threshold, data->vad_state.zcr_threshold);
    
    return 0;
}

mac_vad_state_enum_t mac_vad_process(MacAudioProcessorData* data, const int16_t* samples, size_t sample_count) {
    if (!data || !samples || sample_count == 0 || !data->vad_state.enabled) {
        return MAC_VAD_STATE_SILENCE;
    }
    
    // 计算当前帧的特征
    float energy = mac_calculate_energy(samples, sample_count);
    float zcr = mac_calculate_zero_crossing_rate(samples, sample_count);
    
    // 更新历史
    data->vad_state.energy_history[data->vad_state.history_pos] = energy;
    data->vad_state.zcr_history[data->vad_state.history_pos] = zcr;
    data->vad_state.history_pos = (data->vad_state.history_pos + 1) % data->vad_state.history_size;
    
    // 计算历史平均值
    float avg_energy = 0.0f;
    float avg_zcr = 0.0f;
    for (size_t i = 0; i < data->vad_state.history_size; i++) {
        avg_energy += data->vad_state.energy_history[i];
        avg_zcr += data->vad_state.zcr_history[i];
    }
    avg_energy /= data->vad_state.history_size;
    avg_zcr /= data->vad_state.history_size;
    
    // 自适应阈值调整
    float adaptive_energy_threshold = data->vad_state.energy_threshold + avg_energy * 0.1f;
    
    // VAD决策
    bool is_speech = (energy > adaptive_energy_threshold) && (zcr < data->vad_state.zcr_threshold);
    
    // 状态机处理
    mac_vad_state_enum_t new_state = data->vad_state.current_state;
    
    if (is_speech) {
        new_state = MAC_VAD_STATE_SPEECH;
        data->vad_state.hangover_counter = data->vad_state.hangover_time * 
                                          (data->config.sample_rate / data->buffer_size);
    } else {
        if (data->vad_state.hangover_counter > 0) {
            data->vad_state.hangover_counter--;
            new_state = MAC_VAD_STATE_SPEECH; // 保持语音状态
        } else {
            new_state = MAC_VAD_STATE_SILENCE;
        }
    }
    
    data->vad_state.current_state = new_state;
    
    return new_state;
}

// =============================================================================
// AEC实现
// =============================================================================

int mac_aec_init(MacAudioProcessorData* data) {
    if (!data) {
        return -1;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "初始化AEC组件");
    
    data->aec_state.filter_length = MAC_PROCESSOR_AEC_FILTER_LENGTH;
    data->aec_state.filter_coeffs = (float*)calloc(data->aec_state.filter_length, sizeof(float));
    data->aec_state.reference_buffer = (float*)calloc(data->aec_state.filter_length, sizeof(float));
    data->aec_state.error_buffer = (float*)calloc(data->aec_state.filter_length, sizeof(float));
    
    if (!data->aec_state.filter_coeffs || !data->aec_state.reference_buffer || 
        !data->aec_state.error_buffer) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配AEC缓冲区失败");
        return -1;
    }
    
    data->aec_state.buffer_pos = 0;
    data->aec_state.step_size = 0.01f; // LMS步长
    data->aec_state.enabled = true;
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "AEC初始化完成，滤波器长度: %zu, 步长: %.3f",
              data->aec_state.filter_length, data->aec_state.step_size);
    
    return 0;
}

void mac_aec_process(MacAudioProcessorData* data, const int16_t* input, 
                     const int16_t* reference, int16_t* output, size_t sample_count) {
    if (!data || !input || !output || !data->aec_state.enabled) {
        if (input && output && sample_count > 0) {
            memcpy(output, input, sample_count * sizeof(int16_t));
        }
        return;
    }
    
    // 如果没有参考信号，直接复制输入到输出
    if (!reference) {
        memcpy(output, input, sample_count * sizeof(int16_t));
        return;
    }
    
    // 简化的NLMS自适应滤波器实现
    for (size_t i = 0; i < sample_count; i++) {
        // 转换为浮点数
        float input_sample = (float)input[i] / 32768.0f;
        float ref_sample = (float)reference[i] / 32768.0f;
        
        // 更新参考信号缓冲区
        data->aec_state.reference_buffer[data->aec_state.buffer_pos] = ref_sample;
        
        // 计算滤波器输出（回声估计）
        float echo_estimate = 0.0f;
        for (size_t j = 0; j < data->aec_state.filter_length; j++) {
            size_t idx = (data->aec_state.buffer_pos + data->aec_state.filter_length - j) % 
                        data->aec_state.filter_length;
            echo_estimate += data->aec_state.filter_coeffs[j] * data->aec_state.reference_buffer[idx];
        }
        
        // 计算误差信号
        float error = input_sample - echo_estimate;
        
        // 计算参考信号功率
        float ref_power = 0.0f;
        for (size_t j = 0; j < data->aec_state.filter_length; j++) {
            float ref_val = data->aec_state.reference_buffer[j];
            ref_power += ref_val * ref_val;
        }
        ref_power = ref_power / data->aec_state.filter_length + 1e-6f; // 避免除零
        
        // NLMS更新滤波器系数
        float step = data->aec_state.step_size / ref_power;
        for (size_t j = 0; j < data->aec_state.filter_length; j++) {
            size_t idx = (data->aec_state.buffer_pos + data->aec_state.filter_length - j) % 
                        data->aec_state.filter_length;
            data->aec_state.filter_coeffs[j] += step * error * data->aec_state.reference_buffer[idx];
        }
        
        // 更新缓冲区位置
        data->aec_state.buffer_pos = (data->aec_state.buffer_pos + 1) % data->aec_state.filter_length;
        
        // 输出误差信号（回声消除后的信号）
        output[i] = (int16_t)(error * 32768.0f);
        
        // 限制输出范围
        if (output[i] > 32767) output[i] = 32767;
        if (output[i] < -32768) output[i] = -32768;
    }
}

// =============================================================================
// 噪声抑制实现
// =============================================================================

int mac_ns_init(MacAudioProcessorData* data) {
    if (!data) {
        return -1;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "初始化噪声抑制组件");
    
    // 使用较小的FFT大小以减少延迟
    data->ns_state.fft_size = 256;
    
    data->ns_state.noise_spectrum = (float*)calloc(data->ns_state.fft_size / 2 + 1, sizeof(float));
    data->ns_state.signal_spectrum = (float*)calloc(data->ns_state.fft_size / 2 + 1, sizeof(float));
    data->ns_state.gain_factors = (float*)calloc(data->ns_state.fft_size / 2 + 1, sizeof(float));
    
    if (!data->ns_state.noise_spectrum || !data->ns_state.signal_spectrum || 
        !data->ns_state.gain_factors) {
        LINX_LOGE(MAC_PROCESSOR_TAG, "分配噪声抑制缓冲区失败");
        return -1;
    }
    
    // 初始化参数
    data->ns_state.noise_floor = 0.001f;
    data->ns_state.alpha = 0.95f; // 噪声谱平滑因子
    data->ns_state.enabled = true;
    
    // 初始化增益因子为1
    for (size_t i = 0; i < data->ns_state.fft_size / 2 + 1; i++) {
        data->ns_state.gain_factors[i] = 1.0f;
    }
    
    LINX_LOGI(MAC_PROCESSOR_TAG, "噪声抑制初始化完成，FFT大小: %zu",
              data->ns_state.fft_size);
    
    return 0;
}

void mac_ns_process(MacAudioProcessorData* data, const int16_t* input, 
                    int16_t* output, size_t sample_count) {
    if (!data || !input || !output || !data->ns_state.enabled) {
        if (input && output && sample_count > 0) {
            memcpy(output, input, sample_count * sizeof(int16_t));
        }
        return;
    }
    
    // 简化的频域噪声抑制实现
    // 在实际应用中，这里应该使用FFT进行频域处理
    // 这里使用简化的时域方法
    
    // 计算当前帧的能量
    float frame_energy = mac_calculate_energy(input, sample_count);
    
    // 简单的能量门限噪声抑制
    float noise_threshold = data->ns_state.noise_floor * 10.0f;
    float gain = 1.0f;
    
    if (frame_energy < noise_threshold) {
        // 低能量帧，可能是噪声
        gain = 0.1f; // 大幅衰减
    } else if (frame_energy < noise_threshold * 3.0f) {
        // 中等能量帧，部分抑制
        gain = 0.5f;
    }
    
    // 应用增益
    for (size_t i = 0; i < sample_count; i++) {
        float sample = (float)input[i] * gain;
        output[i] = (int16_t)sample;
        
        // 限制输出范围
        if (output[i] > 32767) output[i] = 32767;
        if (output[i] < -32768) output[i] = -32768;
    }
    
    // 更新噪声底噪估计
    if (frame_energy < data->ns_state.noise_floor * 2.0f) {
        data->ns_state.noise_floor = data->ns_state.alpha * data->ns_state.noise_floor + 
                                    (1.0f - data->ns_state.alpha) * frame_energy;
    }
}

// =============================================================================
// 辅助函数实现
// =============================================================================

float mac_calculate_energy(const int16_t* samples, size_t sample_count) {
    if (!samples || sample_count == 0) {
        return 0.0f;
    }
    
    float energy = 0.0f;
    for (size_t i = 0; i < sample_count; i++) {
        float sample = (float)samples[i] / 32768.0f;
        energy += sample * sample;
    }
    
    return energy / sample_count;
}

float mac_calculate_zero_crossing_rate(const int16_t* samples, size_t sample_count) {
    if (!samples || sample_count < 2) {
        return 0.0f;
    }
    
    size_t zero_crossings = 0;
    for (size_t i = 1; i < sample_count; i++) {
        if ((samples[i] >= 0 && samples[i-1] < 0) || 
            (samples[i] < 0 && samples[i-1] >= 0)) {
            zero_crossings++;
        }
    }
    
    return (float)zero_crossings / (sample_count - 1);
}

float mac_calculate_spectral_centroid(const int16_t* samples, size_t sample_count, int sample_rate) {
    if (!samples || sample_count == 0) {
        return 0.0f;
    }
    
    // 简化的频谱重心计算
    // 在实际应用中应该使用FFT
    float weighted_sum = 0.0f;
    float magnitude_sum = 0.0f;
    
    for (size_t i = 0; i < sample_count; i++) {
        float magnitude = fabsf((float)samples[i]);
        float frequency = (float)i * sample_rate / (2.0f * sample_count);
        
        weighted_sum += frequency * magnitude;
        magnitude_sum += magnitude;
    }
    
    return (magnitude_sum > 0) ? (weighted_sum / magnitude_sum) : 0.0f;
}