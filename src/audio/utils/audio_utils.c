/**
 * @file audio_utils.c
 * @brief LinxOS音频工具函数实现
 */

#include "audio_utils.h"
#include <stdlib.h>
#include <string.h>

// =============================================================================
// 格式信息表
// =============================================================================

typedef struct {
    linx_audio_format_t format;
    uint32_t sample_size;
    uint32_t bit_depth;
    bool is_float;
    bool is_signed;
    const char* name;
} audio_format_info_table_t;

static const audio_format_info_table_t format_table[] = {
    {LINX_AUDIO_FORMAT_U8,      1, 8,  false, false, "U8"},
    {LINX_AUDIO_FORMAT_S16LE,   2, 16, false, true, "S16LE"},
    {LINX_AUDIO_FORMAT_S16BE,   2, 16, false, true, "S16BE"},
    {LINX_AUDIO_FORMAT_S24LE,   3, 24, false, true, "S24LE"},
    {LINX_AUDIO_FORMAT_S24BE,   3, 24, false, true, "S24BE"},
    {LINX_AUDIO_FORMAT_S32LE,   4, 32, false, true, "S32LE"},
    {LINX_AUDIO_FORMAT_S32BE,   4, 32, false, true, "S32BE"},
    {LINX_AUDIO_FORMAT_F32LE,   4, 32, true,  true, "F32LE"},
    {LINX_AUDIO_FORMAT_F32BE,   4, 32, true,  true, "F32BE"},
    {LINX_AUDIO_FORMAT_F64LE,   8, 64, true,  true, "F64LE"},
    {LINX_AUDIO_FORMAT_F64BE,   8, 64, true,  true, "F64BE"},
    {LINX_AUDIO_FORMAT_UNKNOWN, 0, 0,  false, false, "UNKNOWN"}
};

typedef struct {
    linx_audio_channel_layout_t layout;
    uint32_t channel_count;
    const char* name;
} audio_channel_layout_info_table_t;

static const audio_channel_layout_info_table_t channel_layout_table[] = {
    {LINX_AUDIO_CHANNEL_LAYOUT_MONO,   1, "MONO"},
    {LINX_AUDIO_CHANNEL_LAYOUT_STEREO, 2, "STEREO"},
    {LINX_AUDIO_CHANNEL_LAYOUT_2_1,    3, "2.1"},
    {LINX_AUDIO_CHANNEL_LAYOUT_5_1,    6, "5.1"},
    {LINX_AUDIO_CHANNEL_LAYOUT_7_1,    8, "7.1"},
    {LINX_AUDIO_CHANNEL_LAYOUT_UNKNOWN, 0, "UNKNOWN"}
};

// =============================================================================
// 内部辅助函数
// =============================================================================

static const audio_format_info_table_t* find_format_info(linx_audio_format_t format) {
    for (size_t i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++) {
        if (format_table[i].format == format) {
            return &format_table[i];
        }
    }
    return &format_table[sizeof(format_table) / sizeof(format_table[0]) - 1]; // UNKNOWN
}

static const audio_channel_layout_info_table_t* find_channel_layout_info(linx_audio_channel_layout_t layout) {
    for (size_t i = 0; i < sizeof(channel_layout_table) / sizeof(channel_layout_table[0]); i++) {
        if (channel_layout_table[i].layout == layout) {
            return &channel_layout_table[i];
        }
    }
    return &channel_layout_table[sizeof(channel_layout_table) / sizeof(channel_layout_table[0]) - 1]; // UNKNOWN
}

// =============================================================================
// 格式转换和验证工具实现
// =============================================================================

uint32_t audio_format_get_sample_size(linx_audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info->sample_size;
}

const char* audio_format_to_string(linx_audio_format_t format) {
    const audio_format_info_table_t* info = find_format_info(format);
    return info->name;
}

uint32_t audio_channel_layout_get_count(linx_audio_channel_layout_t layout) {
    const audio_channel_layout_info_table_t* info = find_channel_layout_info(layout);
    return info->channel_count;
}

const char* audio_channel_layout_to_string(linx_audio_channel_layout_t layout) {
    const audio_channel_layout_info_table_t* info = find_channel_layout_info(layout);
    return info->name;
}

// =============================================================================
// 错误处理工具实现
// =============================================================================

const char* audio_result_to_string(linx_audio_result_t result) {
    switch (result) {
        case LINX_AUDIO_SUCCESS:
            return "Success";
        case LINX_AUDIO_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case LINX_AUDIO_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case LINX_AUDIO_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case LINX_AUDIO_ERROR_ALREADY_INITIALIZED:
            return "Already initialized";
        case LINX_AUDIO_ERROR_DEVICE_NOT_FOUND:
            return "Device not found";
        case LINX_AUDIO_ERROR_DEVICE_BUSY:
            return "Device busy";
        case LINX_AUDIO_ERROR_UNSUPPORTED_FORMAT:
            return "Unsupported format";
        case LINX_AUDIO_ERROR_BUFFER_OVERFLOW:
            return "Buffer overflow";
        case LINX_AUDIO_ERROR_BUFFER_UNDERFLOW:
            return "Buffer underflow";
        case LINX_AUDIO_ERROR_TIMEOUT:
            return "Timeout";
        case LINX_AUDIO_ERROR_IO_ERROR:
            return "IO error";
        case LINX_AUDIO_ERROR_CODEC_ERROR:
            return "Codec error";
        case LINX_AUDIO_ERROR_PLUGIN_ERROR:
            return "Plugin error";
        case LINX_AUDIO_ERROR_STREAM_ERROR:
            return "Stream error";
        case LINX_AUDIO_ERROR_NOT_FOUND:
            return "Not found";
        case LINX_AUDIO_ERROR_INVALID_STATE:
            return "Invalid state";
        case LINX_AUDIO_ERROR_MUTEX_INIT:
            return "Mutex init failed";
        case LINX_AUDIO_ERROR_CONDITION_INIT:
            return "Condition init failed";
        case LINX_AUDIO_ERROR_THREAD_CREATE:
            return "Thread create failed";
        case LINX_AUDIO_ERROR_RESOURCE_LIMIT:
            return "Resource limit";
        case LINX_AUDIO_ERROR_UNKNOWN:
            return "Unknown error";
        default:
            return "Unknown error";
    }
}