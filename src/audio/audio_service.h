#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include "audio/audio_interface.h"
#include "codecs/audio_codec.h"
#include "wake_words/wake_word_interface.h"
#include "processor/audio_processor.h"
#include "audio_packet_queue.h"
#include "audio_task_queue.h"
#include "timestamp_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_service.h
 * @brief 音频服务接口定义
 * @details 提供音频录制、播放、编解码、唤醒词检测等功能的统一服务接口
 */

// 常量定义
#define OPUS_FRAME_DURATION_MS 60                                      /**< Opus帧持续时间（毫秒） */
#define MAX_ENCODE_TASKS_IN_QUEUE 2                                     /**< 编码任务队列最大容量 */
#define MAX_PLAYBACK_TASKS_IN_QUEUE 2                                   /**< 播放任务队列最大容量 */
#define MAX_DECODE_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)     /**< 解码数据包队列最大容量 */
#define MAX_SEND_PACKETS_IN_QUEUE (2400 / OPUS_FRAME_DURATION_MS)       /**< 发送数据包队列最大容量 */
#define AUDIO_TESTING_MAX_DURATION_MS 10000                             /**< 音频测试最大持续时间（毫秒） */
#define MAX_TIMESTAMPS_IN_QUEUE 3                                       /**< 时间戳队列最大容量 */
#define AUDIO_POWER_TIMEOUT_MS 15000                                    /**< 音频功率超时时间（毫秒） */
#define AUDIO_POWER_CHECK_INTERVAL_MS 1000                              /**< 音频功率检查间隔（毫秒） */

// 事件位定义
#define AS_EVENT_AUDIO_TESTING_RUNNING      (1 << 0)                    /**< 音频测试运行中事件位 */
#define AS_EVENT_WAKE_WORD_RUNNING          (1 << 1)                    /**< 唤醒词检测运行中事件位 */
#define AS_EVENT_AUDIO_PROCESSOR_RUNNING    (1 << 2)                    /**< 音频处理器运行中事件位 */
#define AS_EVENT_PLAYBACK_NOT_EMPTY         (1 << 3)                    /**< 播放队列非空事件位 */

// 前向声明
typedef struct AudioService AudioService;

// 回调函数类型定义
/**
 * @brief 发送队列可用回调函数类型
 * @param user_data 用户数据指针
 */
typedef void (*audio_service_send_queue_callback_t)(void* user_data);

/**
 * @brief 唤醒词检测回调函数类型
 * @param wake_word 检测到的唤醒词
 * @param user_data 用户数据指针
 */
typedef void (*audio_service_wake_word_callback_t)(const char* wake_word, void* user_data);

/**
 * @brief VAD状态变化回调函数类型
 * @param speaking 是否在说话
 * @param user_data 用户数据指针
 */
typedef void (*audio_service_vad_callback_t)(bool speaking, void* user_data);

/**
 * @brief 音频测试队列满回调函数类型
 * @param user_data 用户数据指针
 */
typedef void (*audio_service_testing_queue_full_callback_t)(void* user_data);

/**
 * @brief 音频服务回调函数结构体
 * @details 包含音频服务各种事件的回调函数指针
 */
typedef struct {
    audio_service_send_queue_callback_t on_send_queue_available;        /**< 发送队列可用回调 */
    audio_service_wake_word_callback_t on_wake_word_detected;           /**< 唤醒词检测回调 */
    audio_service_vad_callback_t on_vad_change;                         /**< VAD状态变化回调 */
    audio_service_testing_queue_full_callback_t on_audio_testing_queue_full; /**< 音频测试队列满回调 */
    void* user_data;                                                    /**< 用户数据指针 */
} AudioServiceCallbacks;

/**
 * @brief 调试统计信息结构体
 * @details 记录音频服务各种操作的统计数据
 */
typedef struct {
    uint32_t input_count;       /**< 输入计数 */
    uint32_t decode_count;      /**< 解码计数 */
    uint32_t encode_count;      /**< 编码计数 */
    uint32_t playback_count;    /**< 播放计数 */
} DebugStatistics;

/**
 * @brief 音频服务主结构体
 * @details 包含音频服务的所有核心组件、状态和配置信息
 */
struct AudioService {
    // 核心组件
    audio_codec_t* codec;                   /**< 主音频编解码器 */
    AudioProcessor* audio_processor;        /**< 音频处理器 */
    WakeWordInterface* wake_word;           /**< 唤醒词检测接口 */
    audio_codec_t* opus_encoder;            /**< Opus编码器 */
    audio_codec_t* opus_decoder;            /**< Opus解码器 */
    
    // 回调函数
    AudioServiceCallbacks callbacks;        /**< 回调函数集合 */
    
    // 线程管理
    pthread_t audio_input_thread;           /**< 音频输入线程 */
    pthread_t audio_output_thread;          /**< 音频输出线程 */
    pthread_t opus_codec_thread;            /**< Opus编解码线程 */
    pthread_mutex_t audio_queue_mutex;      /**< 音频队列互斥锁 */
    pthread_cond_t audio_queue_cv;          /**< 音频队列条件变量 */
    uint32_t event_bits;                    /**< 事件位标志 */
    pthread_mutex_t event_mutex;            /**< 事件互斥锁 */
    
    // 队列管理
    AudioPacketQueue audio_decode_queue;    /**< 音频解码队列 */
    AudioPacketQueue audio_send_queue;      /**< 音频发送队列 */
    AudioPacketQueue audio_testing_queue;   /**< 音频测试队列 */
    AudioTaskQueue audio_encode_queue;      /**< 音频编码任务队列 */
    AudioTaskQueue audio_playback_queue;    /**< 音频播放任务队列 */
    TimestampQueue timestamp_queue;         /**< 时间戳队列 */
    
    // 状态标志
    bool wake_word_initialized;             /**< 唤醒词检测是否已初始化 */
    bool audio_processor_initialized;       /**< 音频处理器是否已初始化 */
    bool voice_detected;                    /**< 是否检测到语音 */
    bool service_stopped;                   /**< 服务是否已停止 */
    bool audio_input_need_warmup;           /**< 音频输入是否需要预热 */
    
    // 时间管理
    struct timespec last_input_time;        /**< 最后输入时间 */
    struct timespec last_output_time;       /**< 最后输出时间 */
    
    // 调试信息
    DebugStatistics debug_statistics;       /**< 调试统计信息 */
    
    // 模型管理
    void* models_list;                      /**< 模型列表指针 */
};

// 音频服务API函数

/**
 * @brief 创建新的音频服务实例
 * @return 音频服务指针，失败时返回NULL
 */
AudioService* audio_service_create(void);

/**
 * @brief 使用音频编解码器初始化音频服务
 * @param service 音频服务实例
 * @param codec 要使用的音频编解码器
 * @return 成功返回0，失败返回负数
 */
int audio_service_initialize(AudioService* service, audio_codec_t* codec);

/**
 * @brief 启动音频服务
 * @param service 音频服务实例
 * @return 成功返回0，失败返回负数
 */
int audio_service_start(AudioService* service);

/**
 * @brief 停止音频服务
 * @param service 音频服务实例
 */
void audio_service_stop(AudioService* service);

/**
 * @brief 销毁音频服务并释放资源
 * @param service 音频服务实例
 */
void audio_service_destroy(AudioService* service);

/**
 * @brief 设置音频服务事件回调函数
 * @param service 音频服务实例
 * @param callbacks 回调函数结构体
 */
void audio_service_set_callbacks(AudioService* service, const AudioServiceCallbacks* callbacks);

/**
 * @brief 启用或禁用唤醒词检测
 * @param service 音频服务实例
 * @param enable true表示启用，false表示禁用
 */
void audio_service_enable_wake_word_detection(AudioService* service, bool enable);

/**
 * @brief 启用或禁用语音处理
 * @param service 音频服务实例
 * @param enable true表示启用，false表示禁用
 */
void audio_service_enable_voice_processing(AudioService* service, bool enable);

/**
 * @brief 启用或禁用音频测试
 * @param service 音频服务实例
 * @param enable true表示启用，false表示禁用
 */
void audio_service_enable_audio_testing(AudioService* service, bool enable);

/**
 * @brief 启用或禁用设备AEC（声学回声消除）
 * @param service 音频服务实例
 * @param enable true表示启用，false表示禁用
 */
void audio_service_enable_device_aec(AudioService* service, bool enable);

/**
 * @brief 将音频数据包推送到解码队列
 * @param service 音频服务实例
 * @param packet 要解码的音频数据包
 * @param wait 队列满时是否等待
 * @return 成功返回true，失败返回false
 */
bool audio_service_push_packet_to_decode_queue(AudioService* service, AudioStreamPacket* packet, bool wait);

/**
 * @brief 从发送队列弹出音频数据包
 * @param service 音频服务实例
 * @return 音频数据包指针，队列为空时返回NULL
 */
AudioStreamPacket* audio_service_pop_packet_from_send_queue(AudioService* service);

/**
 * @brief 编码唤醒词数据
 * @param service 音频服务实例
 */
void audio_service_encode_wake_word(AudioService* service);

/**
 * @brief 弹出唤醒词数据包
 * @param service 音频服务实例
 * @return 唤醒词数据包指针，无可用数据时返回NULL
 */
AudioStreamPacket* audio_service_pop_wake_word_packet(AudioService* service);

/**
 * @brief 获取最后检测到的唤醒词
 * @param service 音频服务实例
 * @return 唤醒词字符串，无唤醒词时返回NULL
 */
const char* audio_service_get_last_wake_word(AudioService* service);

/**
 * @brief 检查当前是否检测到语音
 * @param service 音频服务实例
 * @return 检测到语音返回true，否则返回false
 */
bool audio_service_is_voice_detected(const AudioService* service);

/**
 * @brief 检查服务是否处于空闲状态
 * @param service 音频服务实例
 * @return 空闲状态返回true，否则返回false
 */
bool audio_service_is_idle(const AudioService* service);

/**
 * @brief 检查唤醒词检测是否正在运行
 * @param service 音频服务实例
 * @return 正在运行返回true，否则返回false
 */
bool audio_service_is_wake_word_running(const AudioService* service);

/**
 * @brief 检查音频处理器是否正在运行
 * @param service 音频服务实例
 * @return 正在运行返回true，否则返回false
 */
bool audio_service_is_audio_processor_running(const AudioService* service);

/**
 * @brief 播放OGG格式的音频数据
 * @param service 音频服务实例
 * @param ogg_data OGG音频数据
 * @param ogg_size OGG数据大小
 */
void audio_service_play_sound(AudioService* service, const uint8_t* ogg_data, size_t ogg_size);

/**
 * @brief 读取音频数据
 * @param service 音频服务实例
 * @param data 存储音频数据的缓冲区
 * @param sample_rate 期望的采样率
 * @param samples 要读取的样本数
 * @return 成功返回true，失败返回false
 */
bool audio_service_read_audio_data(AudioService* service, int16_t* data, int sample_rate, int samples);

/**
 * @brief 重置音频解码器
 * @param service 音频服务实例
 */
void audio_service_reset_decoder(AudioService* service);

/**
 * @brief 设置服务的模型列表
 * @param service 音频服务实例
 * @param models_list 模型列表指针
 */
void audio_service_set_models_list(AudioService* service, void* models_list);

// 工具函数用于数据包和任务管理

/**
 * @brief 创建音频流数据包
 * @return 新数据包指针，失败时返回NULL
 */
AudioStreamPacket* audio_stream_packet_create(void);

/**
 * @brief 销毁音频流数据包
 * @param packet 要销毁的数据包
 */
void audio_stream_packet_destroy(AudioStreamPacket* packet);



#ifdef __cplusplus
}
#endif

#endif // AUDIO_SERVICE_H