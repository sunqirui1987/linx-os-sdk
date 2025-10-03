#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "codecs/audio_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_processor.h
 * @brief 音频处理器接口定义
 * @details 提供音频处理的统一接口，支持多种音频处理算法的实现
 */

// 前向声明
typedef struct AudioProcessor AudioProcessor;
typedef struct AudioProcessorVTable AudioProcessorVTable;

/**
 * @brief 音频处理器配置结构体
 * @details 包含音频处理器初始化所需的所有配置参数
 */
typedef struct {
    int sample_rate;                /**< 采样率 */
    int channels;                   /**< 声道数 */
    int frame_duration_ms;          /**< 帧持续时间（毫秒） */
    int frame_size;                 /**< 帧大小（样本数） */
    bool enable_aec;                /**< 是否启用回声消除 */
    bool enable_ns;                 /**< 是否启用噪声抑制 */
    bool enable_vad;                /**< 是否启用语音活动检测 */
    float vad_threshold;            /**< VAD阈值 */
    void* models_list;              /**< 模型列表指针 */
} audio_processor_config_t;

/**
 * @brief 音频处理器错误代码枚举
 */
typedef enum {
    AUDIO_PROCESSOR_SUCCESS = 0,            /**< 成功 */
    AUDIO_PROCESSOR_ERROR_INVALID_PARAM,    /**< 无效参数 */
    AUDIO_PROCESSOR_ERROR_NOT_INITIALIZED,  /**< 未初始化 */
    AUDIO_PROCESSOR_ERROR_ALREADY_STARTED,  /**< 已经启动 */
    AUDIO_PROCESSOR_ERROR_NOT_STARTED,      /**< 未启动 */
    AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC,     /**< 内存分配失败 */
    AUDIO_PROCESSOR_ERROR_CODEC_INIT,       /**< 编解码器初始化失败 */
    AUDIO_PROCESSOR_ERROR_PROCESSING,       /**< 处理失败 */
    AUDIO_PROCESSOR_ERROR_UNKNOWN           /**< 未知错误 */
} audio_processor_error_t;

/**
 * @brief 音频输出回调函数类型
 * @param data 音频数据指针
 * @param size 数据大小（样本数）
 * @param user_data 用户数据指针
 */
typedef void (*audio_processor_output_callback_t)(const int16_t* data, size_t size, void* user_data);

/**
 * @brief VAD状态变化回调函数类型
 * @param speaking 是否在说话
 * @param user_data 用户数据指针
 */
typedef void (*audio_processor_vad_callback_t)(bool speaking, void* user_data);

/**
 * @brief 音频处理器虚函数表
 * @details 定义音频处理器的所有操作函数指针
 */
struct AudioProcessorVTable {
    /**
     * @brief 初始化音频处理器
     * @param self 音频处理器实例
     * @param config 配置参数
     * @param codec 音频编解码器
     * @return 错误代码
     */
    audio_processor_error_t (*initialize)(AudioProcessor* self, 
                                         const audio_processor_config_t* config,
                                         audio_codec_t* codec);
    
    /**
     * @brief 启动音频处理器
     * @param self 音频处理器实例
     * @return 错误代码
     */
    audio_processor_error_t (*start)(AudioProcessor* self);
    
    /**
     * @brief 停止音频处理器
     * @param self 音频处理器实例
     * @return 错误代码
     */
    audio_processor_error_t (*stop)(AudioProcessor* self);
    
    /**
     * @brief 输入音频数据进行处理
     * @param self 音频处理器实例
     * @param data 音频数据指针
     * @param size 数据大小（样本数）
     * @return 错误代码
     */
    audio_processor_error_t (*feed)(AudioProcessor* self, const int16_t* data, size_t size);
    
    /**
     * @brief 获取期望的输入数据大小
     * @param self 音频处理器实例
     * @return 期望的输入数据大小（样本数）
     */
    size_t (*get_feed_size)(const AudioProcessor* self);
    
    /**
     * @brief 启用或禁用设备回声消除
     * @param self 音频处理器实例
     * @param enable 是否启用
     * @return 错误代码
     */
    audio_processor_error_t (*enable_device_aec)(AudioProcessor* self, bool enable);
    
    /**
     * @brief 设置音频输出回调函数
     * @param self 音频处理器实例
     * @param callback 回调函数指针
     * @param user_data 用户数据指针
     * @return 错误代码
     */
    audio_processor_error_t (*set_output_callback)(AudioProcessor* self, 
                                                  audio_processor_output_callback_t callback,
                                                  void* user_data);
    
    /**
     * @brief 设置VAD状态变化回调函数
     * @param self 音频处理器实例
     * @param callback 回调函数指针
     * @param user_data 用户数据指针
     * @return 错误代码
     */
    audio_processor_error_t (*set_vad_callback)(AudioProcessor* self,
                                               audio_processor_vad_callback_t callback,
                                               void* user_data);
    
    /**
     * @brief 获取当前VAD状态
     * @param self 音频处理器实例
     * @return VAD状态（true表示检测到语音）
     */
    bool (*get_vad_status)(const AudioProcessor* self);
    
    /**
     * @brief 重置音频处理器状态
     * @param self 音频处理器实例
     * @return 错误代码
     */
    audio_processor_error_t (*reset)(AudioProcessor* self);
    
    /**
     * @brief 获取处理延迟（毫秒）
     * @param self 音频处理器实例
     * @return 处理延迟
     */
    int (*get_delay_ms)(const AudioProcessor* self);
    
    /**
     * @brief 销毁音频处理器
     * @param self 音频处理器实例
     */
    void (*destroy)(AudioProcessor* self);
};

/**
 * @brief 音频处理器基础结构体
 * @details 所有音频处理器实现的基础结构，包含虚函数表和私有数据
 */
struct AudioProcessor {
    const AudioProcessorVTable* vtable;     /**< 虚函数表指针 */
    void* private_data;                     /**< 私有数据指针，由具体实现使用 */
};

// 公共API函数声明

/**
 * @brief 初始化音频处理器
 * @param processor 音频处理器指针
 * @param config 配置参数
 * @param codec 音频编解码器
 * @return 错误代码
 */
audio_processor_error_t audio_processor_initialize(AudioProcessor* processor,
                                                   const audio_processor_config_t* config,
                                                   audio_codec_t* codec);

/**
 * @brief 启动音频处理器
 * @param processor 音频处理器指针
 * @return 错误代码
 */
audio_processor_error_t audio_processor_start(AudioProcessor* processor);

/**
 * @brief 停止音频处理器
 * @param processor 音频处理器指针
 * @return 错误代码
 */
audio_processor_error_t audio_processor_stop(AudioProcessor* processor);

/**
 * @brief 输入音频数据进行处理
 * @param processor 音频处理器指针
 * @param data 音频数据指针
 * @param size 数据大小（样本数）
 * @return 错误代码
 */
audio_processor_error_t audio_processor_feed(AudioProcessor* processor, 
                                            const int16_t* data, 
                                            size_t size);

/**
 * @brief 获取期望的输入数据大小
 * @param processor 音频处理器指针
 * @return 期望的输入数据大小（样本数）
 */
size_t audio_processor_get_feed_size(const AudioProcessor* processor);

/**
 * @brief 启用或禁用设备回声消除
 * @param processor 音频处理器指针
 * @param enable 是否启用
 * @return 错误代码
 */
audio_processor_error_t audio_processor_enable_device_aec(AudioProcessor* processor, bool enable);

/**
 * @brief 设置音频输出回调函数
 * @param processor 音频处理器指针
 * @param callback 回调函数指针
 * @param user_data 用户数据指针
 * @return 错误代码
 */
audio_processor_error_t audio_processor_set_output_callback(AudioProcessor* processor,
                                                           audio_processor_output_callback_t callback,
                                                           void* user_data);

/**
 * @brief 设置VAD状态变化回调函数
 * @param processor 音频处理器指针
 * @param callback 回调函数指针
 * @param user_data 用户数据指针
 * @return 错误代码
 */
audio_processor_error_t audio_processor_set_vad_callback(AudioProcessor* processor,
                                                        audio_processor_vad_callback_t callback,
                                                        void* user_data);

/**
 * @brief 获取当前VAD状态
 * @param processor 音频处理器指针
 * @return VAD状态（true表示检测到语音）
 */
bool audio_processor_get_vad_status(const AudioProcessor* processor);

/**
 * @brief 重置音频处理器状态
 * @param processor 音频处理器指针
 * @return 错误代码
 */
audio_processor_error_t audio_processor_reset(AudioProcessor* processor);

/**
 * @brief 获取处理延迟（毫秒）
 * @param processor 音频处理器指针
 * @return 处理延迟
 */
int audio_processor_get_delay_ms(const AudioProcessor* processor);

/**
 * @brief 销毁音频处理器
 * @param processor 音频处理器指针
 */
void audio_processor_destroy(AudioProcessor* processor);

/**
 * @brief 获取错误代码的字符串描述
 * @param error 错误代码
 * @return 错误描述字符串
 */
const char* audio_processor_error_to_string(audio_processor_error_t error);

/**
 * @brief 创建默认配置
 * @param config 配置结构体指针
 * @param sample_rate 采样率
 * @param channels 声道数
 * @param frame_duration_ms 帧持续时间
 */
void audio_processor_config_init_default(audio_processor_config_t* config,
                                        int sample_rate,
                                        int channels,
                                        int frame_duration_ms);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PROCESSOR_H