/**
 * @file audio_utils.h
 * @brief LinxOS音频工具函数
 * 
 * 提供基本的音频处理工具函数：
 * 1. 格式转换和验证
 * 2. 错误处理工具
 */

#ifndef LINX_AUDIO_UTILS_H
#define LINX_AUDIO_UTILS_H

#include "../core/types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 格式转换和验证工具
// =============================================================================

/**
 * @brief 获取音频格式的字节大小
 * @param format 音频格式
 * @return 每个样本的字节数，失败返回0
 */
uint32_t audio_format_get_sample_size(linx_audio_format_t format);

/**
 * @brief 音频格式转字符串
 * @param format 音频格式
 * @return 格式字符串
 */
const char* audio_format_to_string(linx_audio_format_t format);

/**
 * @brief 获取通道布局的通道数
 * @param layout 通道布局
 * @return 通道数
 */
uint32_t audio_channel_layout_get_count(linx_audio_channel_layout_t layout);

/**
 * @brief 通道布局转字符串
 * @param layout 通道布局
 * @return 布局字符串
 */
const char* audio_channel_layout_to_string(linx_audio_channel_layout_t layout);

// =============================================================================
// 错误处理工具
// =============================================================================

/**
 * @brief 错误结果转字符串
 * @param result 错误结果
 * @return 错误字符串
 */
const char* audio_result_to_string(linx_audio_result_t result);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_UTILS_H