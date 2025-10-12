/**
 * @file audio_utils.h
 * @brief LinxOS音频工具函数
 * 
 * 提供常用的音频处理工具函数：
 * 1. 格式转换和验证
 * 2. 音频数据处理
 * 3. 时间和延迟计算
 * 4. 缓冲区管理工具
 * 5. 调试和诊断工具
 */

#ifndef LINX_AUDIO_UTILS_H
#define LINX_AUDIO_UTILS_H

#include "../core/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 格式转换和验证工具
// =============================================================================

/**
 * @brief 验证音频格式
 * @param format 音频格式信息
 * @return true表示有效，false表示无效
 */
bool audio_format_is_valid(const audio_format_info_t* format);

/**
 * @brief 比较两个音频格式是否相同
 * @param format1 格式1
 * @param format2 格式2
 * @return true表示相同，false表示不同
 */
bool audio_format_is_equal(const audio_format_info_t* format1,
                          const audio_format_info_t* format2);

/**
 * @brief 检查格式是否兼容
 * @param src_format 源格式
 * @param dst_format 目标格式
 * @return true表示兼容，false表示不兼容
 */
bool audio_format_is_compatible(const audio_format_info_t* src_format,
                               const audio_format_info_t* dst_format);

/**
 * @brief 获取音频格式的字节大小
 * @param format 音频格式
 * @return 每个样本的字节数，失败返回0
 */
uint32_t audio_format_get_sample_size(audio_format_t format);

/**
 * @brief 获取音频格式的位深度
 * @param format 音频格式
 * @return 位深度，失败返回0
 */
uint32_t audio_format_get_bit_depth(audio_format_t format);

/**
 * @brief 检查音频格式是否为浮点格式
 * @param format 音频格式
 * @return true表示浮点格式，false表示整数格式
 */
bool audio_format_is_float(audio_format_t format);

/**
 * @brief 检查音频格式是否为有符号格式
 * @param format 音频格式
 * @return true表示有符号，false表示无符号
 */
bool audio_format_is_signed(audio_format_t format);

/**
 * @brief 音频格式转字符串
 * @param format 音频格式
 * @return 格式字符串
 */
const char* audio_format_to_string(audio_format_t format);

/**
 * @brief 字符串转音频格式
 * @param str 格式字符串
 * @return 音频格式，失败返回AUDIO_FORMAT_UNKNOWN
 */
audio_format_t audio_format_from_string(const char* str);

// =============================================================================
// 通道布局工具
// =============================================================================

/**
 * @brief 获取通道布局的通道数
 * @param layout 通道布局
 * @return 通道数
 */
uint32_t audio_channel_layout_get_count(audio_channel_layout_t layout);

/**
 * @brief 检查通道布局是否有效
 * @param layout 通道布局
 * @return true表示有效，false表示无效
 */
bool audio_channel_layout_is_valid(audio_channel_layout_t layout);

/**
 * @brief 通道布局转字符串
 * @param layout 通道布局
 * @return 布局字符串
 */
const char* audio_channel_layout_to_string(audio_channel_layout_t layout);

/**
 * @brief 字符串转通道布局
 * @param str 布局字符串
 * @return 通道布局，失败返回AUDIO_CHANNEL_LAYOUT_UNKNOWN
 */
audio_channel_layout_t audio_channel_layout_from_string(const char* str);

// =============================================================================
// 时间和延迟计算工具
// =============================================================================

/**
 * @brief 帧数转时间（微秒）
 * @param frames 帧数
 * @param sample_rate 采样率
 * @return 时间（微秒）
 */
uint64_t audio_frames_to_microseconds(uint32_t frames, uint32_t sample_rate);

/**
 * @brief 时间（微秒）转帧数
 * @param microseconds 时间（微秒）
 * @param sample_rate 采样率
 * @return 帧数
 */
uint32_t audio_microseconds_to_frames(uint64_t microseconds, uint32_t sample_rate);

/**
 * @brief 帧数转时间（毫秒）
 * @param frames 帧数
 * @param sample_rate 采样率
 * @return 时间（毫秒）
 */
uint32_t audio_frames_to_milliseconds(uint32_t frames, uint32_t sample_rate);

/**
 * @brief 时间（毫秒）转帧数
 * @param milliseconds 时间（毫秒）
 * @param sample_rate 采样率
 * @return 帧数
 */
uint32_t audio_milliseconds_to_frames(uint32_t milliseconds, uint32_t sample_rate);

/**
 * @brief 字节数转帧数
 * @param bytes 字节数
 * @param format 音频格式信息
 * @return 帧数
 */
uint32_t audio_bytes_to_frames(uint32_t bytes, const audio_format_info_t* format);

/**
 * @brief 帧数转字节数
 * @param frames 帧数
 * @param format 音频格式信息
 * @return 字节数
 */
uint32_t audio_frames_to_bytes(uint32_t frames, const audio_format_info_t* format);

// =============================================================================
// 音频数据处理工具
// =============================================================================

/**
 * @brief 静音检测
 * @param buffer 音频缓冲区
 * @param threshold 静音阈值（0.0-1.0）
 * @return true表示静音，false表示有声音
 */
bool audio_detect_silence(const audio_buffer_t* buffer, float threshold);

/**
 * @brief 计算音频RMS值
 * @param buffer 音频缓冲区
 * @return RMS值（0.0-1.0）
 */
float audio_calculate_rms(const audio_buffer_t* buffer);

/**
 * @brief 计算音频峰值
 * @param buffer 音频缓冲区
 * @return 峰值（0.0-1.0）
 */
float audio_calculate_peak(const audio_buffer_t* buffer);

/**
 * @brief 应用音量增益
 * @param buffer 音频缓冲区
 * @param gain 增益值（1.0为原始音量）
 * @return 操作结果
 */
audio_result_t audio_apply_gain(audio_buffer_t* buffer, float gain);

/**
 * @brief 混合两个音频缓冲区
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param mix_ratio 混合比例（0.0-1.0）
 * @return 操作结果
 */
audio_result_t audio_mix_buffers(audio_buffer_t* dst,
                                const audio_buffer_t* src,
                                float mix_ratio);

/**
 * @brief 复制音频缓冲区
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @return 操作结果
 */
audio_result_t audio_copy_buffer(audio_buffer_t* dst, const audio_buffer_t* src);

/**
 * @brief 清零音频缓冲区
 * @param buffer 音频缓冲区
 * @return 操作结果
 */
audio_result_t audio_clear_buffer(audio_buffer_t* buffer);

// =============================================================================
// 缓冲区管理工具
// =============================================================================

/**
 * @brief 创建音频缓冲区
 * @param format 音频格式信息
 * @param frame_count 帧数
 * @return 音频缓冲区，失败返回NULL
 */
audio_buffer_t* audio_buffer_create(const audio_format_info_t* format,
                                   uint32_t frame_count);

/**
 * @brief 销毁音频缓冲区
 * @param buffer 音频缓冲区
 */
void audio_buffer_destroy(audio_buffer_t* buffer);

/**
 * @brief 重置音频缓冲区大小
 * @param buffer 音频缓冲区
 * @param frame_count 新的帧数
 * @return 操作结果
 */
audio_result_t audio_buffer_resize(audio_buffer_t* buffer, uint32_t frame_count);

/**
 * @brief 获取缓冲区可用空间
 * @param buffer 音频缓冲区
 * @return 可用帧数
 */
uint32_t audio_buffer_get_available_frames(const audio_buffer_t* buffer);

/**
 * @brief 获取缓冲区已用空间
 * @param buffer 音频缓冲区
 * @return 已用帧数
 */
uint32_t audio_buffer_get_used_frames(const audio_buffer_t* buffer);

/**
 * @brief 检查缓冲区是否为空
 * @param buffer 音频缓冲区
 * @return true表示为空，false表示非空
 */
bool audio_buffer_is_empty(const audio_buffer_t* buffer);

/**
 * @brief 检查缓冲区是否已满
 * @param buffer 音频缓冲区
 * @return true表示已满，false表示未满
 */
bool audio_buffer_is_full(const audio_buffer_t* buffer);

// =============================================================================
// 事件工具
// =============================================================================

/**
 * @brief 创建音频事件
 * @param type 事件类型
 * @param source_id 源ID
 * @param data 事件数据
 * @param data_size 数据大小
 * @return 音频事件，失败返回NULL
 */
audio_event_t* audio_event_create(audio_event_type_t type,
                                 uint32_t source_id,
                                 const void* data,
                                 uint32_t data_size);

/**
 * @brief 销毁音频事件
 * @param event 音频事件
 */
void audio_event_destroy(audio_event_t* event);

/**
 * @brief 复制音频事件
 * @param event 源事件
 * @return 新事件，失败返回NULL
 */
audio_event_t* audio_event_clone(const audio_event_t* event);

/**
 * @brief 事件类型转字符串
 * @param type 事件类型
 * @return 类型字符串
 */
const char* audio_event_type_to_string(audio_event_type_t type);

// =============================================================================
// 调试和诊断工具
// =============================================================================

/**
 * @brief 打印音频格式信息
 * @param format 音频格式信息
 * @param prefix 前缀字符串
 */
void audio_format_print(const audio_format_info_t* format, const char* prefix);

/**
 * @brief 打印音频缓冲区信息
 * @param buffer 音频缓冲区
 * @param prefix 前缀字符串
 */
void audio_buffer_print(const audio_buffer_t* buffer, const char* prefix);

/**
 * @brief 打印音频事件信息
 * @param event 音频事件
 * @param prefix 前缀字符串
 */
void audio_event_print(const audio_event_t* event, const char* prefix);

/**
 * @brief 验证音频缓冲区数据
 * @param buffer 音频缓冲区
 * @return true表示数据有效，false表示数据损坏
 */
bool audio_buffer_validate(const audio_buffer_t* buffer);

/**
 * @brief 生成音频测试信号
 * @param buffer 音频缓冲区
 * @param frequency 频率（Hz）
 * @param amplitude 振幅（0.0-1.0）
 * @param phase 相位偏移（弧度）
 * @return 操作结果
 */
audio_result_t audio_generate_sine_wave(audio_buffer_t* buffer,
                                       float frequency,
                                       float amplitude,
                                       float phase);

/**
 * @brief 生成白噪声
 * @param buffer 音频缓冲区
 * @param amplitude 振幅（0.0-1.0）
 * @return 操作结果
 */
audio_result_t audio_generate_white_noise(audio_buffer_t* buffer, float amplitude);

// =============================================================================
// 性能测量工具
// =============================================================================

/**
 * @brief 性能计时器结构
 */
typedef struct {
    struct timespec start_time;    ///< 开始时间
    struct timespec end_time;      ///< 结束时间
    uint64_t total_time_ns;        ///< 总时间（纳秒）
    uint32_t measurement_count;    ///< 测量次数
} audio_perf_timer_t;

/**
 * @brief 创建性能计时器
 * @return 性能计时器，失败返回NULL
 */
audio_perf_timer_t* audio_perf_timer_create(void);

/**
 * @brief 销毁性能计时器
 * @param timer 性能计时器
 */
void audio_perf_timer_destroy(audio_perf_timer_t* timer);

/**
 * @brief 开始计时
 * @param timer 性能计时器
 */
void audio_perf_timer_start(audio_perf_timer_t* timer);

/**
 * @brief 结束计时
 * @param timer 性能计时器
 * @return 本次测量时间（纳秒）
 */
uint64_t audio_perf_timer_stop(audio_perf_timer_t* timer);

/**
 * @brief 获取平均时间
 * @param timer 性能计时器
 * @return 平均时间（纳秒）
 */
uint64_t audio_perf_timer_get_average(const audio_perf_timer_t* timer);

/**
 * @brief 重置计时器
 * @param timer 性能计时器
 */
void audio_perf_timer_reset(audio_perf_timer_t* timer);

// =============================================================================
// 错误处理工具
// =============================================================================

/**
 * @brief 错误结果转字符串
 * @param result 错误结果
 * @return 错误字符串
 */
const char* audio_result_to_string(audio_result_t result);

/**
 * @brief 检查结果是否成功
 * @param result 操作结果
 * @return true表示成功，false表示失败
 */
bool audio_result_is_success(audio_result_t result);

/**
 * @brief 检查结果是否为错误
 * @param result 操作结果
 * @return true表示错误，false表示成功
 */
bool audio_result_is_error(audio_result_t result);

// =============================================================================
// 内存管理工具
// =============================================================================

/**
 * @brief 对齐内存分配
 * @param size 分配大小
 * @param alignment 对齐字节数
 * @return 分配的内存指针，失败返回NULL
 */
void* audio_aligned_malloc(size_t size, size_t alignment);

/**
 * @brief 释放对齐内存
 * @param ptr 内存指针
 */
void audio_aligned_free(void* ptr);

/**
 * @brief 获取系统页面大小
 * @return 页面大小（字节）
 */
size_t audio_get_page_size(void);

/**
 * @brief 获取CPU缓存行大小
 * @return 缓存行大小（字节）
 */
size_t audio_get_cache_line_size(void);

// =============================================================================
// 线程和同步工具
// =============================================================================

/**
 * @brief 设置线程优先级
 * @param thread 线程ID
 * @param priority 优先级
 * @return 操作结果
 */
audio_result_t audio_set_thread_priority(pthread_t thread,
                                        audio_thread_priority_t priority);

/**
 * @brief 获取线程优先级
 * @param thread 线程ID
 * @param priority 输出优先级
 * @return 操作结果
 */
audio_result_t audio_get_thread_priority(pthread_t thread,
                                        audio_thread_priority_t* priority);

/**
 * @brief 设置线程名称
 * @param thread 线程ID
 * @param name 线程名称
 * @return 操作结果
 */
audio_result_t audio_set_thread_name(pthread_t thread, const char* name);

/**
 * @brief 线程优先级转字符串
 * @param priority 线程优先级
 * @return 优先级字符串
 */
const char* audio_thread_priority_to_string(audio_thread_priority_t priority);

/**
 * @brief 获取当前时间戳（微秒）
 * @return 时间戳（微秒）
 */
uint64_t audio_get_timestamp_us(void);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_UTILS_H