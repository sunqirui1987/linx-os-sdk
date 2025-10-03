#ifndef AUDIO_PROCESSOR_MAC_H
#define AUDIO_PROCESSOR_MAC_H

#include "processor/audio_processor.h"
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_processor_mac.h
 * @brief Mac平台音频处理器实现
 * @details 基于Core Audio框架实现的音频处理器，支持VAD、AEC等功能
 */

// Mac平台特定的配置
#define MAC_PROCESSOR_MAX_FRAME_SIZE 4096
#define MAC_PROCESSOR_VAD_WINDOW_SIZE 160  // 10ms at 16kHz
#define MAC_PROCESSOR_VAD_HISTORY_SIZE 50  // 500ms history
#define MAC_PROCESSOR_AEC_FILTER_LENGTH 512

/**
 * @brief VAD状态枚举
 */
typedef enum {
    MAC_VAD_STATE_SILENCE = 0,
    MAC_VAD_STATE_SPEECH,
    MAC_VAD_STATE_UNCERTAIN
} mac_vad_state_enum_t;

/**
 * @brief VAD统计信息
 */
typedef struct {
    float energy;                   /**< 当前帧能量 */
    float zero_crossing_rate;       /**< 过零率 */
    float spectral_centroid;        /**< 频谱重心 */
    float snr_estimate;             /**< 信噪比估计 */
} mac_vad_stats_t;

/**
 * @brief AEC状态结构体
 */
typedef struct {
    float* filter_coeffs;           /**< 自适应滤波器系数 */
    float* reference_buffer;        /**< 参考信号缓冲区 */
    float* error_buffer;            /**< 误差信号缓冲区 */
    size_t filter_length;           /**< 滤波器长度 */
    size_t buffer_pos;              /**< 缓冲区位置 */
    float step_size;                /**< 自适应步长 */
    bool enabled;                   /**< 是否启用 */
} mac_aec_state_t;

/**
 * @brief 噪声抑制状态结构体
 */
typedef struct {
    float* noise_spectrum;          /**< 噪声频谱估计 */
    float* signal_spectrum;         /**< 信号频谱 */
    float* gain_factors;            /**< 增益因子 */
    size_t fft_size;                /**< FFT大小 */
    float noise_floor;              /**< 噪声底噪 */
    float alpha;                    /**< 平滑因子 */
    bool enabled;                   /**< 是否启用 */
} mac_ns_state_t;

/**
 * @brief VAD状态结构体
 */
typedef struct {
    float* energy_history;          /**< 能量历史 */
    float* zcr_history;             /**< 过零率历史 */
    size_t history_pos;             /**< 历史位置 */
    size_t history_size;            /**< 历史大小 */
    float energy_threshold;         /**< 能量阈值 */
    float zcr_threshold;            /**< 过零率阈值 */
    float hangover_time;            /**< 拖尾时间 */
    float hangover_counter;         /**< 拖尾计数器 */
    mac_vad_state_enum_t current_state;  /**< 当前状态 */
    bool enabled;                   /**< 是否启用 */
} mac_vad_state_t;

/**
 * @brief Mac音频处理器私有数据结构
 */
typedef struct {
    // 基本配置
    audio_processor_config_t config;
    audio_codec_t* codec;
    
    // 状态标志
    bool initialized;
    bool started;
    
    // 回调函数
    audio_processor_output_callback_t output_callback;
    void* output_callback_user_data;
    audio_processor_vad_callback_t vad_callback;
    void* vad_callback_user_data;
    
    // 音频处理组件
    mac_vad_state_t vad_state;
    mac_aec_state_t aec_state;
    mac_ns_state_t ns_state;
    
    // 音频缓冲区
    int16_t* input_buffer;
    int16_t* output_buffer;
    float* processing_buffer;
    size_t buffer_size;
    
    // Core Audio组件
    AudioUnit audio_unit;
    AudioStreamBasicDescription audio_format;
    
    // 线程同步
    pthread_mutex_t processing_mutex;
    pthread_cond_t processing_cond;
    
    // 统计信息
    uint64_t frames_processed;
    uint64_t vad_speech_frames;
    uint64_t vad_silence_frames;
    
} MacAudioProcessorData;

/**
 * @brief 创建Mac音频处理器实例
 * @return 音频处理器指针，失败时返回NULL
 */
AudioProcessor* audio_processor_mac_create(void);

/**
 * @brief 销毁Mac音频处理器实例
 * @param processor 音频处理器指针
 */
void audio_processor_mac_destroy(AudioProcessor* processor);

// 内部辅助函数声明

/**
 * @brief 初始化VAD组件
 * @param data Mac音频处理器数据
 * @return 成功返回0，失败返回负数
 */
int mac_vad_init(MacAudioProcessorData* data);

/**
 * @brief 处理VAD检测
 * @param data Mac音频处理器数据
 * @param samples 音频样本
 * @param sample_count 样本数量
 * @return VAD状态
 */
mac_vad_state_enum_t mac_vad_process(MacAudioProcessorData* data, const int16_t* samples, size_t sample_count);

/**
 * @brief 初始化AEC组件
 * @param data Mac音频处理器数据
 * @return 成功返回0，失败返回负数
 */
int mac_aec_init(MacAudioProcessorData* data);

/**
 * @brief 处理AEC
 * @param data Mac音频处理器数据
 * @param input 输入信号
 * @param reference 参考信号
 * @param output 输出信号
 * @param sample_count 样本数量
 */
void mac_aec_process(MacAudioProcessorData* data, const int16_t* input, 
                     const int16_t* reference, int16_t* output, size_t sample_count);

/**
 * @brief 初始化噪声抑制组件
 * @param data Mac音频处理器数据
 * @return 成功返回0，失败返回负数
 */
int mac_ns_init(MacAudioProcessorData* data);

/**
 * @brief 处理噪声抑制
 * @param data Mac音频处理器数据
 * @param input 输入信号
 * @param output 输出信号
 * @param sample_count 样本数量
 */
void mac_ns_process(MacAudioProcessorData* data, const int16_t* input, 
                    int16_t* output, size_t sample_count);

/**
 * @brief 计算音频能量
 * @param samples 音频样本
 * @param sample_count 样本数量
 * @return 能量值
 */
float mac_calculate_energy(const int16_t* samples, size_t sample_count);

/**
 * @brief 计算过零率
 * @param samples 音频样本
 * @param sample_count 样本数量
 * @return 过零率
 */
float mac_calculate_zero_crossing_rate(const int16_t* samples, size_t sample_count);

/**
 * @brief 计算频谱重心
 * @param samples 音频样本
 * @param sample_count 样本数量
 * @param sample_rate 采样率
 * @return 频谱重心
 */
float mac_calculate_spectral_centroid(const int16_t* samples, size_t sample_count, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PROCESSOR_MAC_H