#include "audio_processor_speexdsp.h"
#include "../../../../src/common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/**
 * @file audio_processor_speexdsp.c
 * @brief 基于SpeexDSP的音频处理器实现
 * @details 使用SpeexDSP库实现回声消除、噪声抑制和语音活动检测
 */

#define SPEEX_PROCESSOR_TAG "SpeexDspProcessor"

// 默认配置参数
#define DEFAULT_FRAME_SIZE_MS 20
#define DEFAULT_FILTER_LENGTH_MS 200
#define DEFAULT_NOISE_SUPPRESS -15
#define DEFAULT_AGC_LEVEL 8000

// 前向声明
static audio_processor_error_t speexdsp_initialize(AudioProcessor* self,
                                                   const audio_processor_config_t* config,
                                                   AudioInterface* audio_interface);
static audio_processor_error_t speexdsp_start(AudioProcessor* self);
static audio_processor_error_t speexdsp_stop(AudioProcessor* self);
static audio_processor_error_t speexdsp_feed(AudioProcessor* self, const int16_t* data, size_t size);
static size_t speexdsp_get_feed_size(const AudioProcessor* self);
static audio_processor_error_t speexdsp_enable_device_aec(AudioProcessor* self, bool enable);
static audio_processor_error_t speexdsp_set_output_callback(AudioProcessor* self,
                                                           audio_processor_output_callback_t callback,
                                                           void* user_data);
static audio_processor_error_t speexdsp_set_vad_callback(AudioProcessor* self,
                                                        audio_processor_vad_callback_t callback,
                                                        void* user_data);
static bool speexdsp_get_vad_status(const AudioProcessor* self);
static audio_processor_error_t speexdsp_reset(AudioProcessor* self);
static int speexdsp_get_delay_ms(const AudioProcessor* self);
static void speexdsp_destroy(AudioProcessor* self);

// 内部函数声明
static void* processing_thread_func(void* arg);
static void process_audio_frame(SpeexDspProcessorData* data, const int16_t* input, size_t size);
static bool detect_vad(SpeexDspProcessorData* data, const int16_t* input, size_t size);
static void cleanup_speexdsp_resources(SpeexDspProcessorData* data);

// 虚函数表
static const AudioProcessorVTable speexdsp_vtable = {
    .initialize = speexdsp_initialize,
    .start = speexdsp_start,
    .stop = speexdsp_stop,
    .feed = speexdsp_feed,
    .get_feed_size = speexdsp_get_feed_size,
    .enable_device_aec = speexdsp_enable_device_aec,
    .set_output_callback = speexdsp_set_output_callback,
    .set_vad_callback = speexdsp_set_vad_callback,
    .get_vad_status = speexdsp_get_vad_status,
    .reset = speexdsp_reset,
    .get_delay_ms = speexdsp_get_delay_ms,
    .destroy = speexdsp_destroy
};

// =============================================================================
// 公共API实现
// =============================================================================

AudioProcessor* audio_processor_speexdsp_create(void) {
    AudioProcessor* processor = (AudioProcessor*)malloc(sizeof(AudioProcessor));
    if (!processor) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to allocate AudioProcessor");
        return NULL;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)malloc(sizeof(SpeexDspProcessorData));
    if (!data) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to allocate SpeexDspProcessorData");
        free(processor);
        return NULL;
    }

    // 初始化数据结构
    memset(data, 0, sizeof(SpeexDspProcessorData));
    
    // 初始化互斥锁
    if (pthread_mutex_init(&data->mutex, NULL) != 0) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to initialize mutex");
        free(data);
        free(processor);
        return NULL;
    }

    processor->vtable = &speexdsp_vtable;
    processor->private_data = data;

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP audio processor created");
    return processor;
}

void audio_processor_speexdsp_destroy(AudioProcessor* processor) {
    if (processor && processor->vtable && processor->vtable->destroy) {
        processor->vtable->destroy(processor);
    }
}

// =============================================================================
// 虚函数表实现
// =============================================================================

static audio_processor_error_t speexdsp_initialize(AudioProcessor* self,
                                                   const audio_processor_config_t* config,
                                                   AudioInterface* audio_interface) {
    if (!self || !config || !audio_interface) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_ALREADY_STARTED;
    }

    // 保存配置
    data->config = *config;
    data->audio_interface = audio_interface;

    // 计算帧大小
    data->frame_size = config->frame_size;
    if (data->frame_size <= 0) {
        data->frame_size = config->sample_rate * DEFAULT_FRAME_SIZE_MS / 1000;
    }

    data->buffer_size = data->frame_size * config->channels;
    data->sampling_rate = config->sample_rate;

    // 分配缓冲区
    data->input_buffer = (int16_t*)malloc(data->buffer_size * sizeof(int16_t));
    data->output_buffer = (int16_t*)malloc(data->buffer_size * sizeof(int16_t));
    data->reference_buffer = (int16_t*)malloc(data->buffer_size * sizeof(int16_t));

    if (!data->input_buffer || !data->output_buffer || !data->reference_buffer) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to allocate audio buffers");
        cleanup_speexdsp_resources(data);
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC;
    }

    // 初始化SpeexDSP
    if (config->enable_aec) {
        data->filter_length = config->sample_rate * DEFAULT_FILTER_LENGTH_MS / 1000;
        data->echo_state = speex_echo_state_init(data->frame_size, data->filter_length);
        if (!data->echo_state) {
            LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to initialize SpeexDSP echo state");
            cleanup_speexdsp_resources(data);
            pthread_mutex_unlock(&data->mutex);
            return AUDIO_PROCESSOR_ERROR_CODEC_INIT;
        }

        // 设置采样率
        speex_echo_ctl(data->echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &data->sampling_rate);
    }

    if (config->enable_ns || config->enable_vad) {
        data->preprocess_state = speex_preprocess_state_init(data->frame_size, data->sampling_rate);
        if (!data->preprocess_state) {
            LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to initialize SpeexDSP preprocess state");
            cleanup_speexdsp_resources(data);
            pthread_mutex_unlock(&data->mutex);
            return AUDIO_PROCESSOR_ERROR_CODEC_INIT;
        }

        // 配置降噪
        if (config->enable_ns) {
            int denoise = 1;
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
            int noise_suppress = DEFAULT_NOISE_SUPPRESS;
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);
        }

        // 配置AGC
        int agc = 1;
        speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
        int agc_level = DEFAULT_AGC_LEVEL;
        speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);

        // 配置VAD
        if (config->enable_vad) {
            int vad = 1;
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_VAD, &vad);
        }

        // 如果同时启用AEC和预处理，需要关联它们
        if (data->echo_state && data->preprocess_state) {
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, data->echo_state);
        }
    }

    // 分配SpeexDSP处理缓冲区
    data->speex_input_frame = (spx_int16_t*)malloc(data->frame_size * sizeof(spx_int16_t));
    data->speex_output_frame = (spx_int16_t*)malloc(data->frame_size * sizeof(spx_int16_t));
    data->speex_reference_frame = (spx_int16_t*)malloc(data->frame_size * sizeof(spx_int16_t));

    if (!data->speex_input_frame || !data->speex_output_frame || !data->speex_reference_frame) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to allocate SpeexDSP buffers");
        cleanup_speexdsp_resources(data);
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC;
    }

    data->initialized = true;
    pthread_mutex_unlock(&data->mutex);

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP processor initialized: frame_size=%d, sample_rate=%d",
              data->frame_size, data->sampling_rate);
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t speexdsp_start(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    if (!data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_NOT_INITIALIZED;
    }

    if (data->running) {
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_ALREADY_STARTED;
    }

    data->running = true;
    data->buffer_pos = 0;

    // 创建处理线程
    if (pthread_create(&data->processing_thread, NULL, processing_thread_func, self) != 0) {
        LINX_LOGE(SPEEX_PROCESSOR_TAG, "Failed to create processing thread");
        data->running = false;
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_UNKNOWN;
    }

    pthread_mutex_unlock(&data->mutex);

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP processor started");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t speexdsp_stop(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    if (!data->running) {
        pthread_mutex_unlock(&data->mutex);
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }

    data->running = false;
    pthread_mutex_unlock(&data->mutex);

    // 等待处理线程结束
    pthread_join(data->processing_thread, NULL);

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP processor stopped");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t speexdsp_feed(AudioProcessor* self, const int16_t* data, size_t size) {
    if (!self || !data || size == 0) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* proc_data = (SpeexDspProcessorData*)self->private_data;
    if (!proc_data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&proc_data->mutex);

    if (!proc_data->running) {
        pthread_mutex_unlock(&proc_data->mutex);
        return AUDIO_PROCESSOR_ERROR_NOT_STARTED;
    }

    // 将数据添加到输入缓冲区
    size_t remaining = proc_data->buffer_size - proc_data->buffer_pos;
    size_t to_copy = (size < remaining) ? size : remaining;

    memcpy(proc_data->input_buffer + proc_data->buffer_pos, data, to_copy * sizeof(int16_t));
    proc_data->buffer_pos += to_copy;

    // 如果缓冲区满了，处理一帧
    if (proc_data->buffer_pos >= proc_data->buffer_size) {
        process_audio_frame(proc_data, proc_data->input_buffer, proc_data->buffer_size);
        proc_data->buffer_pos = 0;
    }

    pthread_mutex_unlock(&proc_data->mutex);
    return AUDIO_PROCESSOR_SUCCESS;
}

static size_t speexdsp_get_feed_size(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }

    return data->frame_size;
}

static audio_processor_error_t speexdsp_enable_device_aec(AudioProcessor* self, bool enable) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);
    data->config.enable_aec = enable;
    pthread_mutex_unlock(&data->mutex);

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "Device AEC %s", enable ? "enabled" : "disabled");
    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t speexdsp_set_output_callback(AudioProcessor* self,
                                                           audio_processor_output_callback_t callback,
                                                           void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);
    data->output_callback = callback;
    data->output_user_data = user_data;
    pthread_mutex_unlock(&data->mutex);

    return AUDIO_PROCESSOR_SUCCESS;
}

static audio_processor_error_t speexdsp_set_vad_callback(AudioProcessor* self,
                                                        audio_processor_vad_callback_t callback,
                                                        void* user_data) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);
    data->vad_callback = callback;
    data->vad_user_data = user_data;
    pthread_mutex_unlock(&data->mutex);

    return AUDIO_PROCESSOR_SUCCESS;
}

static bool speexdsp_get_vad_status(const AudioProcessor* self) {
    if (!self) {
        return false;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return false;
    }

    return data->vad_state;
}

static audio_processor_error_t speexdsp_reset(AudioProcessor* self) {
    if (!self) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    data->buffer_pos = 0;
    data->vad_state = false;
    data->prev_vad_state = false;

    if (data->echo_state) {
        // SpeexDSP echo state没有reset函数，重新创建来重置状态
        speex_echo_state_destroy(data->echo_state);
        data->echo_state = speex_echo_state_init(data->frame_size, data->filter_length);
        if (data->echo_state) {
            speex_echo_ctl(data->echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &data->sampling_rate);
        }
    }
    if (data->preprocess_state) {
        // SpeexDSP preprocess state没有reset函数，重新创建来重置状态
        speex_preprocess_state_destroy(data->preprocess_state);
        data->preprocess_state = speex_preprocess_state_init(data->frame_size, data->sampling_rate);
        if (data->preprocess_state) {
            // 重新配置参数
            if (data->config.enable_ns) {
                int denoise = 1;
                speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
                int noise_suppress = DEFAULT_NOISE_SUPPRESS;
                speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);
            }
            
            int agc = 1;
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_AGC, &agc);
            int agc_level = DEFAULT_AGC_LEVEL;
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
            
            if (data->config.enable_vad) {
                int vad = 1;
                speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_VAD, &vad);
            }
            
            // 如果echo_state存在，重新关联
            if (data->echo_state) {
                speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, data->echo_state);
            }
        }
    }

    pthread_mutex_unlock(&data->mutex);

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP processor reset");
    return AUDIO_PROCESSOR_SUCCESS;
}

static int speexdsp_get_delay_ms(const AudioProcessor* self) {
    if (!self) {
        return 0;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (!data || !data->initialized) {
        return 0;
    }

    // 返回一帧的延迟时间
    return (data->frame_size * 1000) / data->sampling_rate;
}

static void speexdsp_destroy(AudioProcessor* self) {
    if (!self) {
        return;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;
    if (data) {
        // 停止处理器
        if (data->running) {
            speexdsp_stop(self);
        }

        // 清理资源
        cleanup_speexdsp_resources(data);

        // 销毁互斥锁
        pthread_mutex_destroy(&data->mutex);

        free(data);
    }

    free(self);
    LINX_LOGI(SPEEX_PROCESSOR_TAG, "SpeexDSP processor destroyed");
}

// =============================================================================
// 内部函数实现
// =============================================================================

static void* processing_thread_func(void* arg) {
    AudioProcessor* self = (AudioProcessor*)arg;
    SpeexDspProcessorData* data = (SpeexDspProcessorData*)self->private_data;

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "Processing thread started");

    while (data->running) {
        usleep(1000); // 1ms
        // 处理线程主要用于状态管理，实际处理在feed函数中进行
    }

    LINX_LOGI(SPEEX_PROCESSOR_TAG, "Processing thread stopped");
    return NULL;
}

static void process_audio_frame(SpeexDspProcessorData* data, const int16_t* input, size_t size) {
    if (!data || !input || size == 0) {
        return;
    }

    // 复制输入到输出（默认直通）
    memcpy(data->output_buffer, input, size * sizeof(int16_t));

    // 如果启用了SpeexDSP处理
    if (data->echo_state || data->preprocess_state) {
        // 转换为SpeexDSP格式
        for (size_t i = 0; i < data->frame_size && i < size; i++) {
            data->speex_input_frame[i] = input[i];
        }

        // AEC处理
        if (data->echo_state && data->config.enable_aec) {
            // 这里需要参考信号，暂时使用零信号
            memset(data->speex_reference_frame, 0, data->frame_size * sizeof(spx_int16_t));
            speex_echo_cancellation(data->echo_state, data->speex_input_frame,
                                   data->speex_reference_frame, data->speex_output_frame);
            memcpy(data->speex_input_frame, data->speex_output_frame, data->frame_size * sizeof(spx_int16_t));
        }

        // 预处理（降噪、AGC、VAD）
        if (data->preprocess_state) {
            int vad_result = speex_preprocess_run(data->preprocess_state, data->speex_input_frame);
            
            // 更新VAD状态
            bool new_vad_state = (vad_result != 0);
            if (new_vad_state != data->vad_state) {
                data->vad_state = new_vad_state;
                if (data->vad_callback) {
                    data->vad_callback(new_vad_state, data->vad_user_data);
                }
            }

            // 更新统计信息
            if (new_vad_state) {
                data->vad_speech_frames++;
            } else {
                data->vad_silence_frames++;
            }
        }

        // 转换回int16_t格式
        for (size_t i = 0; i < data->frame_size && i < size; i++) {
            data->output_buffer[i] = data->speex_input_frame[i];
        }
    } else {
        // 如果没有启用SpeexDSP处理，使用简单的VAD检测
        if (data->config.enable_vad) {
            bool new_vad_state = detect_vad(data, input, size);
            if (new_vad_state != data->vad_state) {
                data->vad_state = new_vad_state;
                if (data->vad_callback) {
                    data->vad_callback(new_vad_state, data->vad_user_data);
                }
            }
        }
    }

    // 更新统计信息
    data->frames_processed++;

    // 调用输出回调
    if (data->output_callback) {
        data->output_callback(data->output_buffer, size, data->output_user_data);
    }
}

static bool detect_vad(SpeexDspProcessorData* data, const int16_t* input, size_t size) {
    if (!data || !input || size == 0) {
        return false;
    }

    // 简单的能量检测VAD
    long long energy = 0;
    for (size_t i = 0; i < size; i++) {
        energy += (long long)input[i] * input[i];
    }
    energy /= size;

    // 简单的阈值检测
    const long long threshold = 1000000; // 可调整的阈值
    return energy > threshold;
}

static void cleanup_speexdsp_resources(SpeexDspProcessorData* data) {
    if (!data) {
        return;
    }

    // 清理音频缓冲区
    if (data->input_buffer) {
        free(data->input_buffer);
        data->input_buffer = NULL;
    }
    if (data->output_buffer) {
        free(data->output_buffer);
        data->output_buffer = NULL;
    }
    if (data->reference_buffer) {
        free(data->reference_buffer);
        data->reference_buffer = NULL;
    }

    // 清理SpeexDSP资源
    if (data->echo_state) {
        speex_echo_state_destroy(data->echo_state);
        data->echo_state = NULL;
    }
    if (data->preprocess_state) {
        speex_preprocess_state_destroy(data->preprocess_state);
        data->preprocess_state = NULL;
    }

    // 清理SpeexDSP缓冲区
    if (data->speex_input_frame) {
        free(data->speex_input_frame);
        data->speex_input_frame = NULL;
    }
    if (data->speex_output_frame) {
        free(data->speex_output_frame);
        data->speex_output_frame = NULL;
    }
    if (data->speex_reference_frame) {
        free(data->speex_reference_frame);
        data->speex_reference_frame = NULL;
    }

    data->initialized = false;
}

// =============================================================================
// 扩展API实现
// =============================================================================

audio_processor_error_t audio_processor_speexdsp_get_stats(const AudioProcessor* processor,
                                                          unsigned long* frames_processed,
                                                          unsigned long* vad_speech_frames,
                                                          unsigned long* vad_silence_frames) {
    if (!processor) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)processor->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock((pthread_mutex_t*)&data->mutex);

    if (frames_processed) {
        *frames_processed = data->frames_processed;
    }
    if (vad_speech_frames) {
        *vad_speech_frames = data->vad_speech_frames;
    }
    if (vad_silence_frames) {
        *vad_silence_frames = data->vad_silence_frames;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&data->mutex);
    return AUDIO_PROCESSOR_SUCCESS;
}

audio_processor_error_t audio_processor_speexdsp_set_aec_params(AudioProcessor* processor,
                                                               int filter_length) {
    if (!processor || filter_length <= 0) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)processor->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    if (data->echo_state) {
        // 需要重新创建echo state来改变filter length
        speex_echo_state_destroy(data->echo_state);
        data->filter_length = filter_length;
        data->echo_state = speex_echo_state_init(data->frame_size, data->filter_length);
        if (data->echo_state) {
            speex_echo_ctl(data->echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &data->sampling_rate);
            LINX_LOGI(SPEEX_PROCESSOR_TAG, "AEC filter length updated to %d", filter_length);
        }
    }

    pthread_mutex_unlock(&data->mutex);
    return AUDIO_PROCESSOR_SUCCESS;
}

audio_processor_error_t audio_processor_speexdsp_set_denoise_params(AudioProcessor* processor,
                                                                   int noise_suppress,
                                                                   int agc_level) {
    if (!processor) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    SpeexDspProcessorData* data = (SpeexDspProcessorData*)processor->private_data;
    if (!data) {
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&data->mutex);

    if (data->preprocess_state) {
        if (noise_suppress >= -15 && noise_suppress <= 0) {
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);
        }
        if (agc_level >= 1 && agc_level <= 32768) {
            speex_preprocess_ctl(data->preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agc_level);
        }
        LINX_LOGI(SPEEX_PROCESSOR_TAG, "Denoise params updated: noise_suppress=%d, agc_level=%d",
                  noise_suppress, agc_level);
    }

    pthread_mutex_unlock(&data->mutex);
    return AUDIO_PROCESSOR_SUCCESS;
}