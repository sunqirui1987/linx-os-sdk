#ifndef AUDIO_PROCESSOR_SPEEXDSP_H
#define AUDIO_PROCESSOR_SPEEXDSP_H

#include "audio/processor/audio_processor.h"
#include "common/log/linx_log.h"
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_processor_speexdsp.h
 * @brief 基于SpeexDSP的音频处理器实现
 * @details 使用SpeexDSP库实现回声消除、噪声抑制和语音活动检测
 */

// SpeexDSP相关头文件
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

/**
 * @brief SpeexDSP音频处理器私有数据结构
 */
typedef struct {
    // 基础配置
    audio_processor_config_t config;
    AudioInterface* audio_interface;
    
    // 运行状态
    bool initialized;
    bool running;
    pthread_mutex_t mutex;
    pthread_t processing_thread;
    
    // 回调函数
    audio_processor_output_callback_t output_callback;
    void* output_user_data;
    audio_processor_vad_callback_t vad_callback;
    void* vad_user_data;
    
    // 音频缓冲区
    int16_t* input_buffer;
    int16_t* output_buffer;
    int16_t* reference_buffer;  // 用于AEC的参考信号
    size_t buffer_size;
    size_t buffer_pos;
    
    // VAD状态
    bool vad_state;
    bool prev_vad_state;
    
    // SpeexDSP状态
    SpeexEchoState* echo_state;
    SpeexPreprocessState* preprocess_state;
    
    // SpeexDSP配置
    int frame_size;
    int filter_length;
    int sampling_rate;
    
    // 处理缓冲区
    spx_int16_t* speex_input_frame;
    spx_int16_t* speex_output_frame;
    spx_int16_t* speex_reference_frame;
    
    // 统计信息
    unsigned long frames_processed;
    unsigned long vad_speech_frames;
    unsigned long vad_silence_frames;
    
} SpeexDspProcessorData;

/**
 * @brief 创建SpeexDSP音频处理器实例
 * @return 音频处理器指针，失败返回NULL
 */
AudioProcessor* audio_processor_speexdsp_create(void);

/**
 * @brief 销毁SpeexDSP音频处理器实例
 * @param processor 音频处理器指针
 */
void audio_processor_speexdsp_destroy(AudioProcessor* processor);

/**
 * @brief 获取SpeexDSP处理器的统计信息
 * @param processor 音频处理器指针
 * @param frames_processed 处理的帧数
 * @param vad_speech_frames VAD检测到语音的帧数
 * @param vad_silence_frames VAD检测到静音的帧数
 * @return 错误代码
 */
audio_processor_error_t audio_processor_speexdsp_get_stats(const AudioProcessor* processor,
                                                          unsigned long* frames_processed,
                                                          unsigned long* vad_speech_frames,
                                                          unsigned long* vad_silence_frames);

/**
 * @brief 设置SpeexDSP AEC参数
 * @param processor 音频处理器指针
 * @param filter_length 滤波器长度（毫秒）
 * @return 错误代码
 */
audio_processor_error_t audio_processor_speexdsp_set_aec_params(AudioProcessor* processor,
                                                               int filter_length);

/**
 * @brief 设置SpeexDSP降噪参数
 * @param processor 音频处理器指针
 * @param noise_suppress 噪声抑制强度（-15到0 dB）
 * @param agc_level AGC目标电平（1到32768）
 * @return 错误代码
 */
audio_processor_error_t audio_processor_speexdsp_set_denoise_params(AudioProcessor* processor,
                                                                   int noise_suppress,
                                                                   int agc_level);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PROCESSOR_SPEEXDSP_H