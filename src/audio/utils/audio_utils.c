/**
 * @file audio_utils.c
 * @brief LinxOS音频工具函数实现
 */

#include "audio_utils.h"
#include "../core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#ifdef __APPLE__
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#include <sched.h>
#endif

// =============================================================================
// 内部常量和宏定义
// =============================================================================

#define AUDIO_SILENCE_THRESHOLD_DEFAULT 0.001f
#define AUDIO_PI 3.14159265358979323846
#define AUDIO_2PI (2.0 * AUDIO_PI)

// 格式信息表
typedef struct {
    audio_format_t format;
    const char* name;
    uint32_t sample_size;
    uint32_t bit_depth;
    bool is_float;
    bool is_signed;
} audio_format_info_table_t;

static const audio_format_info_table_t format_table[] = {
    {AUDIO_FORMAT_PCM_8,      "PCM_8",      1, 8,  false, false},
    {AUDIO_FORMAT_PCM_16,     "PCM_16",     2, 16, false, true},
    {AUDIO_FORMAT_PCM_24,     "PCM_24",     3, 24, false, true},
    {AUDIO_FORMAT_PCM_32,     "PCM_32",     4, 32, false, true},
    {AUDIO_FORMAT_PCM_FLOAT,  "PCM_FLOAT",  4, 32, true,  true},
    {AUDIO_FORMAT_PCM_DOUBLE, "PCM_DOUBLE", 8, 64, true,  true},
    {AUDIO_FORMAT_OPUS,       "OPUS",       0, 0,  false, false},
    {AUDIO_FORMAT_AAC,        "AAC",        0, 0,  false, false},
    {AUDIO_FORMAT_MP3,        "MP3",        0, 0,  false, false},
    {AUDIO_FORMAT_FLAC,       "FLAC",       0, 0,  false, false},
};

static const size_t format_table_size = sizeof(format_table) / sizeof(format_table[0]);

// 通道布局信息表
typedef struct {
    audio_channel_layout_t layout;
    const char* name;
    uint32_t channel_count;
} audio_channel_layout_info_table_t;

static const audio_channel_layout_info_table_t channel_layout_table[] = {
    {AUDIO_CHANNEL_LAYOUT_MONO,     "MONO",     1},
    {AUDIO_CHANNEL_LAYOUT_STEREO,   "STEREO",   2},
    {AUDIO_CHANNEL_LAYOUT_2_1,      "2.1",      3},
    {AUDIO_CHANNEL_LAYOUT_SURROUND, "SURROUND", 3},
    {AUDIO_CHANNEL_LAYOUT_4_0,      "4.0",      4},
    {AUDIO_CHANNEL_LAYOUT_5_1,      "5.1",      6},
    {AUDIO_CHANNEL_LAYOUT_7_1,      "7.1",      8},
};

static const size_t channel_layout_table_size = sizeof(channel_layout_table) / sizeof(channel_layout_table[0]);

// =============================================================================
// 内部辅助函数
// =============================================================================

static const audio_format_info_table_t* find_format_info(audio_format_t format) {
    for (size_t i = 0; i < format_table_size; i++) {
        if (format_table[i].format == format) {
            return &format_table[i];
        }
    }
    return NULL;
}

static const audio_channel_layout_info_table_t* find_channel_layout_info(audio_channel_layout_t layout) {
    for (size_t i = 0; i < channel_layout_table_size; i++) {
        if (channel_layout_table[i].layout == layout) {
            return &channel_layout_table[i];
        }
    }
    return NULL;
}

// =============================================================================
// 格式转换和验证工具实现
// =============================================================================

bool audio_format_is_valid(const audio_format_info_t* format) {
    if (!format) return false;
    
    // 检查格式是否支持
    if (find_format_info(format->format) == NULL) {
        return false;
    }
    
    // 检查采样率范围
    if (format->sample_rate < 8000 || format->sample_rate > 192000) {
        return false;
    }
    
    // 检查通道布局
    if (find_channel_layout_info(format->channel_layout) == NULL) {
        return false;
    }
    
    return true;
}

bool audio_format_is_equal(const audio_format_info_t* format1,
                          const audio_format_info_t* format2) {
    if (!format1 || !format2) return false;
    
    return (format1->format == format2->format &&
            format1->sample_rate == format2->sample_rate &&
            format1->channel_layout == format2->channel_layout);
}

bool audio_format_is_compatible(const audio_format_info_t* src_format,
                               const audio_format_info_t* dst_format) {
    if (!src_format || !dst_format) return false;
    
    // 相同格式直接兼容
    if (audio_format_is_equal(src_format, dst_format)) {
        return true;
    }
    
    // 检查是否可以进行格式转换
    const audio_format_info_table_t* src_info = find_format_info(src_format->format);
    const audio_format_info_table_t* dst_info = find_format_info(dst_format->format);
    
    if (!src_info || !dst_info) return false;
    
    // 浮点和整数格式之间可以转换
    // 不同位深度之间可以转换
    // 不同采样率之间可以重采样
    // 不同通道布局之间可以转换
    return true;
}

uint32_t audio_format_get_sample_size(audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info ? info->sample_size : 0;
}

uint32_t audio_format_get_bit_depth(audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info ? info->bit_depth : 0;
}

bool audio_format_is_float(audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info ? info->is_float : false;
}

bool audio_format_is_signed(audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info ? info->is_signed : false;
}

const char* audio_format_to_string(audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info ? info->name : "UNKNOWN";
}

audio_format_t audio_format_from_string(const char* str) {
    if (!str) return AUDIO_FORMAT_UNKNOWN;
    
    for (size_t i = 0; i < format_table_size; i++) {
        if (strcmp(format_table[i].name, str) == 0) {
            return format_table[i].format;
        }
    }
    return AUDIO_FORMAT_UNKNOWN;
}

// =============================================================================
// 通道布局工具实现
// =============================================================================

uint32_t audio_channel_layout_get_count(audio_channel_layout_t layout) {
    const audio_channel_layout_info_table_t* info = find_channel_layout_info(layout);
    return info ? info->channel_count : 0;
}

bool audio_channel_layout_is_valid(audio_channel_layout_t layout) {
    return find_channel_layout_info(layout) != NULL;
}

const char* audio_channel_layout_to_string(audio_channel_layout_t layout) {
    const audio_channel_layout_info_table_t* info = find_channel_layout_info(layout);
    return info ? info->name : "UNKNOWN";
}

audio_channel_layout_t audio_channel_layout_from_string(const char* str) {
    if (!str) {
        return AUDIO_CHANNEL_LAYOUT_MONO; // 默认返回单声道而不是UNKNOWN
    }
    
    for (size_t i = 0; i < channel_layout_table_size; i++) {
        if (strcmp(str, channel_layout_table[i].name) == 0) {
            return channel_layout_table[i].layout;
        }
    }
    
    return AUDIO_CHANNEL_LAYOUT_MONO; // 默认返回单声道而不是UNKNOWN
}

audio_channel_layout_t audio_channel_layout_from_count(uint32_t count) {
    switch (count) {
        case 1: return AUDIO_CHANNEL_LAYOUT_MONO;
        case 2: return AUDIO_CHANNEL_LAYOUT_STEREO;
        case 3: return AUDIO_CHANNEL_LAYOUT_2_1;
        case 4: return AUDIO_CHANNEL_LAYOUT_4_0;
        case 6: return AUDIO_CHANNEL_LAYOUT_5_1;
        case 8: return AUDIO_CHANNEL_LAYOUT_7_1;
        default: return AUDIO_CHANNEL_LAYOUT_MONO;
    }
}

bool audio_channel_layout_is_compatible(audio_channel_layout_t layout1, 
                                       audio_channel_layout_t layout2) {
    if (layout1 == layout2) {
        return true;
    }
    
    // 检查通道数是否相同
    return audio_channel_layout_get_count(layout1) == 
           audio_channel_layout_get_count(layout2);
}

// =============================================================================
// 时间和延迟计算工具实现
// =============================================================================

uint64_t audio_frames_to_microseconds(uint32_t frames, uint32_t sample_rate) {
    if (sample_rate == 0) return 0;
    return (uint64_t)frames * 1000000 / sample_rate;
}

uint32_t audio_microseconds_to_frames(uint64_t microseconds, uint32_t sample_rate) {
    if (sample_rate == 0) return 0;
    return (uint32_t)(microseconds * sample_rate / 1000000);
}

uint32_t audio_frames_to_milliseconds(uint32_t frames, uint32_t sample_rate) {
    if (sample_rate == 0) return 0;
    return frames * 1000 / sample_rate;
}

uint32_t audio_milliseconds_to_frames(uint32_t milliseconds, uint32_t sample_rate) {
    if (sample_rate == 0) return 0;
    return milliseconds * sample_rate / 1000;
}

uint32_t audio_bytes_to_frames(uint32_t bytes, const audio_format_info_t* format) {
    if (!format) return 0;
    
    uint32_t sample_size = audio_format_get_sample_size(format->format);
    uint32_t channel_count = audio_channel_layout_get_count(format->channel_layout);
    
    if (sample_size == 0 || channel_count == 0) return 0;
    
    uint32_t frame_size = sample_size * channel_count;
    return bytes / frame_size;
}

uint32_t audio_frames_to_bytes(uint32_t frames, const audio_format_info_t* format) {
    if (!format) return 0;
    
    uint32_t sample_size = audio_format_get_sample_size(format->format);
    uint32_t channel_count = audio_channel_layout_get_count(format->channel_layout);
    
    if (sample_size == 0 || channel_count == 0) return 0;
    
    uint32_t frame_size = sample_size * channel_count;
    return frames * frame_size;
}

// =============================================================================
// 音频数据处理工具实现
// =============================================================================

bool audio_detect_silence(const audio_buffer_t* buffer, float threshold) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return true;
    }
    
    // 简化实现，假设为32位浮点格式
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return false; // 需要格式转换
    }
    
    const float* samples = (const float*)buffer->data;
    size_t sample_count = buffer->size / sizeof(float);
    
    for (size_t i = 0; i < sample_count; i++) {
        if (fabsf(samples[i]) > threshold) {
            return false;
        }
    }
    
    return true;
}

float audio_calculate_rms(const audio_buffer_t* buffer) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return 0.0f;
    }
    
    // 简化实现，假设为32位浮点格式
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return 0.0f; // 需要格式转换
    }
    
    const float* samples = (const float*)buffer->data;
    size_t sample_count = buffer->size / sizeof(float);
    
    double sum = 0.0;
    for (size_t i = 0; i < sample_count; i++) {
        sum += samples[i] * samples[i];
    }
    
    return sqrtf(sum / sample_count);
}

float audio_calculate_peak(const audio_buffer_t* buffer) {
    if (!buffer || !buffer->data || buffer->size == 0) {
        return 0.0f;
    }
    
    // 简化实现，假设为32位浮点格式
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return 0.0f; // 返回0而不是错误码
    }
    
    const float* samples = (const float*)buffer->data;
    size_t sample_count = buffer->size / sizeof(float);
    
    float peak = 0.0f;
    for (size_t i = 0; i < sample_count; i++) {
        float abs_sample = fabsf(samples[i]);
        if (abs_sample > peak) {
            peak = abs_sample;
        }
    }
    
    return peak;
}

audio_result_t audio_apply_gain(audio_buffer_t* buffer, float gain) {
    if (!buffer || !buffer->data) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    float* samples = (float*)buffer->data;
    size_t sample_count = buffer->size / sizeof(float);
    
    for (size_t i = 0; i < sample_count; i++) {
        samples[i] *= gain;
    }
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_mix_buffers(audio_buffer_t* dst,
                                const audio_buffer_t* src,
                                float mix_ratio) {
    if (!dst || !src || !dst->data || !src->data) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (dst->format.format != src->format.format) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    size_t mix_size = (dst->size < src->size) ? dst->size : src->size;
    
    // 简化实现，假设为32位浮点格式
    if (dst->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    float* dst_samples = (float*)dst->data;
    const float* src_samples = (const float*)src->data;
    
    size_t sample_count = mix_size / sizeof(float);
    for (size_t i = 0; i < sample_count; i++) {
        dst_samples[i] = dst_samples[i] * (1.0f - mix_ratio) + src_samples[i] * mix_ratio;
    }
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_copy_buffer(audio_buffer_t* dst, const audio_buffer_t* src) {
    if (!dst || !src) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (dst->capacity < src->size) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    memcpy(dst->data, src->data, src->size);
    dst->size = src->size;
    dst->frame_count = src->frame_count;
    dst->format = src->format;
    dst->timestamp = src->timestamp;
    dst->is_silence = src->is_silence;
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_clear_buffer(audio_buffer_t* buffer) {
    if (!buffer) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (buffer->data && buffer->capacity > 0) {
        memset(buffer->data, 0, buffer->capacity);
    }
    buffer->size = 0;
    buffer->frame_count = 0;
    buffer->is_silence = true;
    
    return AUDIO_SUCCESS;
}

// =============================================================================
// 缓冲区管理工具实现
// =============================================================================

audio_buffer_t* audio_buffer_create(const audio_format_info_t* format,
                                   uint32_t frame_count) {
    if (!format || frame_count == 0) {
        return NULL;
    }
    
    audio_buffer_t* buffer = malloc(sizeof(audio_buffer_t));
    if (!buffer) {
        return NULL;
    }
    
    size_t buffer_size = frame_count * format->frame_size;
    buffer->data = malloc(buffer_size);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }
    
    buffer->size = 0;
    buffer->capacity = buffer_size;
    buffer->frame_count = 0;
    buffer->format = *format;
    buffer->timestamp = 0;
    buffer->is_silence = true;
    buffer->metadata = NULL;
    
    return buffer;
}

void audio_buffer_destroy(audio_buffer_t* buffer) {
    if (!buffer) return;
    
    if (buffer->data) {
        free(buffer->data);
    }
    free(buffer);
}

audio_result_t audio_buffer_resize(audio_buffer_t* buffer, uint32_t new_frame_count) {
    if (!buffer) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (new_frame_count == 0) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    size_t new_buffer_size = new_frame_count * buffer->format.frame_size;
    void* new_data = realloc(buffer->data, new_buffer_size);
    if (!new_data) {
        return AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    buffer->data = new_data;
    buffer->capacity = new_buffer_size;
    
    // 如果缩小了，调整当前大小
    if (buffer->size > new_buffer_size) {
        buffer->size = new_buffer_size;
        buffer->frame_count = new_frame_count;
    }
    
    return AUDIO_SUCCESS;
}

uint32_t audio_buffer_get_available_frames(const audio_buffer_t* buffer) {
    if (!buffer) return 0;
    return buffer->capacity - buffer->frame_count;
}

uint32_t audio_buffer_get_used_frames(const audio_buffer_t* buffer) {
    if (!buffer) return 0;
    return buffer->frame_count;
}

bool audio_buffer_is_empty(const audio_buffer_t* buffer) {
    return !buffer || buffer->frame_count == 0;
}

bool audio_buffer_is_full(const audio_buffer_t* buffer) {
    return buffer && buffer->frame_count >= buffer->capacity;
}

// =============================================================================
// 事件工具实现
// =============================================================================

audio_event_t* audio_event_create(audio_event_type_t type,
                                 uint32_t source_id,
                                 const void* data,
                                 uint32_t data_size) {
    audio_event_t* event = malloc(sizeof(audio_event_t));
    if (!event) {
        return NULL;
    }
    
    event->type = type;
    event->timestamp = audio_get_timestamp_us();
    event->source = (void*)(uintptr_t)source_id; // 将source_id转换为指针
    event->data_size = data_size;
    
    if (data && data_size > 0) {
        event->data = malloc(data_size);
        if (!event->data) {
            free(event);
            return NULL;
        }
        memcpy(event->data, data, data_size);
    } else {
        event->data = NULL;
    }
    
    event->user_data = NULL;
    
    return event;
}

void audio_event_destroy(audio_event_t* event) {
    if (!event) return;
    
    if (event->data) {
        free(event->data);
    }
    free(event);
}

audio_event_t* audio_event_clone(const audio_event_t* event) {
    if (!event) {
        return NULL;
    }
    
    uint32_t source_id = (uint32_t)(uintptr_t)event->source; // 从指针转换回source_id
    return audio_event_create(event->type, source_id, event->data, event->data_size);
}

const char* audio_event_type_to_string(audio_event_type_t type) {
    switch (type) {
        case AUDIO_EVENT_TYPE_STREAM_STARTED: return "Stream Started";
        case AUDIO_EVENT_TYPE_STREAM_STOPPED: return "Stream Stopped";
        case AUDIO_EVENT_TYPE_STREAM_PAUSED: return "Stream Paused";
        case AUDIO_EVENT_TYPE_STREAM_RESUMED: return "Stream Resumed";
        case AUDIO_EVENT_TYPE_STREAM_ERROR: return "Stream Error";
        case AUDIO_EVENT_TYPE_BUFFER_UNDERRUN: return "Buffer Underrun";
        case AUDIO_EVENT_TYPE_BUFFER_OVERRUN: return "Buffer Overrun";
        case AUDIO_EVENT_TYPE_DEVICE_CONNECTED: return "Device Connected";
        case AUDIO_EVENT_TYPE_DEVICE_DISCONNECTED: return "Device Disconnected";
        case AUDIO_EVENT_TYPE_PLUGIN_LOADED: return "Plugin Loaded";
        case AUDIO_EVENT_TYPE_PLUGIN_UNLOADED: return "Plugin Unloaded";
        case AUDIO_EVENT_TYPE_VOLUME_CHANGED: return "Volume Changed";
        case AUDIO_EVENT_TYPE_FORMAT_CHANGED: return "Format Changed";
        default: return "Unknown";
    }
}

// =============================================================================
// 调试和诊断工具实现
// =============================================================================

void audio_format_print(const audio_format_info_t* format, const char* prefix) {
    if (!format) return;
    
    const char* pfx = prefix ? prefix : "";
    printf("%sAudio Format:\n", pfx);
    printf("%s  Format: %s\n", pfx, audio_format_to_string(format->format));
    printf("%s  Sample Rate: %u Hz\n", pfx, format->sample_rate);
    printf("%s  Channel Layout: %s (%u channels)\n", pfx, 
           audio_channel_layout_to_string(format->channel_layout),
           audio_channel_layout_get_count(format->channel_layout));
}

void audio_buffer_print(const audio_buffer_t* buffer, const char* prefix) {
    if (!buffer) return;
    
    const char* pfx = prefix ? prefix : "";
    printf("%sAudio Buffer:\n", pfx);
    printf("%s  Frame Count: %u\n", pfx, buffer->frame_count);
    printf("%s  Capacity: %zu\n", pfx, buffer->capacity);
    printf("%s  Data Size: %u bytes\n", pfx, 
           audio_frames_to_bytes(buffer->frame_count, &buffer->format));
    audio_format_print(&buffer->format, prefix);
}

void audio_event_print(const audio_event_t* event, const char* prefix) {
    if (!event) return;
    
    const char* pfx = prefix ? prefix : "";
    printf("%sAudio Event:\n", pfx);
    printf("%s  Type: %s\n", pfx, audio_event_type_to_string(event->type));
    printf("%s  Timestamp: %llu\n", pfx, (unsigned long long)event->timestamp);
    printf("%s  Source: %p\n", pfx, event->source);
    printf("%s  Data Size: %zu\n", pfx, event->data_size);
}

bool audio_buffer_validate(const audio_buffer_t* buffer) {
    if (!buffer) return false;
    if (!buffer->data && buffer->frame_count > 0) return false;
    if (buffer->frame_count > buffer->capacity) return false;
    if (!audio_format_is_valid(&buffer->format)) return false;
    
    return true;
}

audio_result_t audio_generate_sine_wave(audio_buffer_t* buffer,
                                       float frequency,
                                       float amplitude,
                                       float phase) {
    if (!buffer || !buffer->data) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 简化实现，假设为32位浮点格式
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    float* samples = (float*)buffer->data;
    size_t sample_count = buffer->capacity / sizeof(float);
    uint32_t sample_rate = buffer->format.sample_rate;
    
    for (size_t i = 0; i < sample_count; i++) {
        float t = (float)i / sample_rate;
        samples[i] = amplitude * sinf(2.0f * AUDIO_PI * frequency * t + phase);
    }
    
    buffer->size = sample_count * sizeof(float);
    buffer->frame_count = sample_count / buffer->format.channels;
    buffer->is_silence = false;
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_generate_white_noise(audio_buffer_t* buffer, float amplitude) {
    if (!buffer || !buffer->data) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 简化实现，假设为32位浮点格式
    if (buffer->format.format != AUDIO_FORMAT_PCM_FLOAT) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    float* samples = (float*)buffer->data;
    size_t sample_count = buffer->capacity / sizeof(float);
    
    for (size_t i = 0; i < sample_count; i++) {
        samples[i] = amplitude * (2.0f * (float)rand() / RAND_MAX - 1.0f);
    }
    
    buffer->size = sample_count * sizeof(float);
    buffer->frame_count = sample_count / buffer->format.channels;
    buffer->is_silence = false;
    
    return AUDIO_SUCCESS;
}

// =============================================================================
// 性能测量工具实现
// =============================================================================

audio_perf_timer_t* audio_perf_timer_create(void) {
    audio_perf_timer_t* timer = malloc(sizeof(audio_perf_timer_t));
    if (!timer) return NULL;
    
    memset(timer, 0, sizeof(audio_perf_timer_t));
    return timer;
}

void audio_perf_timer_destroy(audio_perf_timer_t* timer) {
    if (timer) {
        free(timer);
    }
}

void audio_perf_timer_start(audio_perf_timer_t* timer) {
    if (!timer) return;
    clock_gettime(CLOCK_MONOTONIC, &timer->start_time);
}

uint64_t audio_perf_timer_stop(audio_perf_timer_t* timer) {
    if (!timer) return 0;
    
    clock_gettime(CLOCK_MONOTONIC, &timer->end_time);
    
    uint64_t elapsed_ns = (timer->end_time.tv_sec - timer->start_time.tv_sec) * 1000000000ULL +
                         (timer->end_time.tv_nsec - timer->start_time.tv_nsec);
    
    timer->total_time_ns += elapsed_ns;
    timer->measurement_count++;
    
    return elapsed_ns;
}

uint64_t audio_perf_timer_get_average(const audio_perf_timer_t* timer) {
    if (!timer || timer->measurement_count == 0) return 0;
    return timer->total_time_ns / timer->measurement_count;
}

void audio_perf_timer_reset(audio_perf_timer_t* timer) {
    if (!timer) return;
    timer->total_time_ns = 0;
    timer->measurement_count = 0;
}

// =============================================================================
// 错误处理工具实现
// =============================================================================

const char* audio_result_to_string(audio_result_t result) {
    switch (result) {
        case AUDIO_SUCCESS: return "SUCCESS";
        case AUDIO_ERROR_INVALID_PARAM: return "INVALID_PARAMETER";
        case AUDIO_ERROR_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case AUDIO_ERROR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case AUDIO_ERROR_ALREADY_INITIALIZED: return "ALREADY_INITIALIZED";
        case AUDIO_ERROR_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case AUDIO_ERROR_DEVICE_BUSY: return "DEVICE_BUSY";
        case AUDIO_ERROR_TIMEOUT: return "TIMEOUT";
        case AUDIO_ERROR_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case AUDIO_ERROR_BUFFER_UNDERFLOW: return "BUFFER_UNDERFLOW";
        case AUDIO_ERROR_PLUGIN_NOT_FOUND: return "PLUGIN_NOT_FOUND";
        case AUDIO_ERROR_PLUGIN_LOAD_FAILED: return "PLUGIN_LOAD_FAILED";
        case AUDIO_ERROR_STREAM_NOT_FOUND: return "STREAM_NOT_FOUND";
        case AUDIO_ERROR_INVALID_STATE: return "INVALID_STATE";
        case AUDIO_ERROR_HARDWARE_FAILURE: return "HARDWARE_FAILURE";
        case AUDIO_ERROR_UNKNOWN: return "UNKNOWN_ERROR";
        default: return "UNKNOWN";
    }
}

bool audio_is_success(audio_result_t result) {
    return result == AUDIO_SUCCESS;
}

bool audio_is_error(audio_result_t result) {
    return result != AUDIO_SUCCESS;
}

// =============================================================================
// 内存管理工具实现
// =============================================================================

void* audio_aligned_malloc(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) {
        return NULL;
    }
    
    void* ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    
    return ptr;
}

void audio_aligned_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

size_t audio_get_page_size(void) {
    return getpagesize();
}

size_t audio_get_cache_line_size(void) {
    // 大多数现代处理器的缓存行大小
    return 64;
}

// =============================================================================
// 线程和同步工具实现
// =============================================================================

audio_result_t audio_set_thread_priority(pthread_t thread,
                                        audio_thread_priority_t priority) {
    if (thread == 0) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    int policy = SCHED_OTHER;
    struct sched_param param;
    memset(&param, 0, sizeof(param));
    
    switch (priority) {
        case AUDIO_THREAD_PRIORITY_LOW:
            param.sched_priority = 10;
            break;
        case AUDIO_THREAD_PRIORITY_NORMAL:
            param.sched_priority = 20;
            break;
        case AUDIO_THREAD_PRIORITY_HIGH:
            param.sched_priority = 30;
            break;
        case AUDIO_THREAD_PRIORITY_REALTIME:
            policy = SCHED_FIFO;
            param.sched_priority = 50;
            break;
        default:
            return AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (pthread_setschedparam(thread, policy, &param) != 0) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_get_thread_priority(pthread_t thread,
                                        audio_thread_priority_t* priority) {
    if (thread == 0 || priority == NULL) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
    int policy;
    struct sched_param param;
    
    if (pthread_getschedparam(thread, &policy, &param) != 0) {
        *priority = AUDIO_THREAD_PRIORITY_NORMAL;
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    if (policy == SCHED_FIFO && param.sched_priority >= 50) {
        *priority = AUDIO_THREAD_PRIORITY_REALTIME;
    } else if (param.sched_priority >= 30) {
        *priority = AUDIO_THREAD_PRIORITY_HIGH;
    } else if (param.sched_priority >= 20) {
        *priority = AUDIO_THREAD_PRIORITY_NORMAL;
    } else {
        *priority = AUDIO_THREAD_PRIORITY_LOW;
    }
    
    return AUDIO_SUCCESS;
}

audio_result_t audio_set_thread_name(pthread_t thread, const char* name) {
    if (thread == 0 || !name) {
        return AUDIO_ERROR_INVALID_PARAM;
    }
    
#ifdef __APPLE__
    // macOS 使用不同的API
    if (pthread_equal(thread, pthread_self())) {
        if (pthread_setname_np(name) != 0) {
            return AUDIO_ERROR_NOT_SUPPORTED;
        }
    } else {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
#else
    if (pthread_setname_np(thread, name) != 0) {
        return AUDIO_ERROR_NOT_SUPPORTED;
    }
#endif
    
    return AUDIO_SUCCESS;
}

const char* audio_thread_priority_to_string(audio_thread_priority_t priority) {
    switch (priority) {
        case AUDIO_THREAD_PRIORITY_LOW: return "LOW";
        case AUDIO_THREAD_PRIORITY_NORMAL: return "NORMAL";
        case AUDIO_THREAD_PRIORITY_HIGH: return "HIGH";
        case AUDIO_THREAD_PRIORITY_REALTIME: return "REALTIME";
        default: return "UNKNOWN";
    }
}

uint64_t audio_get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}