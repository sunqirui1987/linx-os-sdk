#ifndef LINX_PLAYER_H
#define LINX_PLAYER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct AudioInterface AudioInterface;
typedef struct audio_codec audio_codec_t;

/**
 * 播放器状态枚举
 */
typedef enum {
    PLAYER_STATE_IDLE,      // 空闲状态
    PLAYER_STATE_PLAYING,   // 播放中
    PLAYER_STATE_PAUSED,    // 暂停
    PLAYER_STATE_STOPPED,   // 停止
    PLAYER_STATE_ERROR      // 错误状态
} player_state_t;

/**
 * 播放器错误码
 */
typedef enum {
    PLAYER_SUCCESS = 0,
    PLAYER_ERROR_INVALID_PARAM,
    PLAYER_ERROR_NOT_INITIALIZED,
    PLAYER_ERROR_AUDIO_INTERFACE,
    PLAYER_ERROR_CODEC,
    PLAYER_ERROR_THREAD,
    PLAYER_ERROR_BUFFER_FULL,
    PLAYER_ERROR_INVALID_STATE
} player_error_t;

/**
 * 音频格式配置
 */
typedef struct {
    int sample_rate;        // 采样率 (Hz)
    int channels;           // 声道数
    int frame_size;         // 帧大小（样本数）
    int buffer_size;        // 缓冲区大小
} player_audio_config_t;

/**
 * 播放器事件回调函数类型
 */
typedef void (*player_event_callback_t)(player_state_t old_state, player_state_t new_state, void* user_data);

/**
 * 播放器结构体
 */
typedef struct {
    // 音频接口和编解码器
    AudioInterface* audio_interface;
    audio_codec_t* decoder;
    
    // 配置
    player_audio_config_t config;
    
    // 状态管理
    player_state_t state;
    bool initialized;
    bool running;
    
    // 线程和同步
    pthread_t playback_thread;
    pthread_mutex_t state_mutex;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cond;
    
    // 音频缓冲区（环形缓冲区）
    uint8_t* audio_buffer;
    size_t buffer_capacity;
    size_t buffer_head;     // 写入位置
    size_t buffer_tail;     // 读取位置
    size_t buffer_count;    // 当前缓冲区中的数据量
    
    // 事件回调
    player_event_callback_t event_callback;
    void* callback_user_data;
    
    // 统计信息
    size_t total_bytes_played;
    size_t total_frames_played;
} linx_player_t;

/**
 * 创建播放器实例
 * @param audio_interface 音频接口实例
 * @param decoder 音频解码器实例
 * @return 播放器实例指针，失败返回NULL
 */
linx_player_t* linx_player_create(AudioInterface* audio_interface, audio_codec_t* decoder);

/**
 * 初始化播放器
 * @param player 播放器实例
 * @param config 音频配置
 * @return 错误码
 */
player_error_t linx_player_init(linx_player_t* player, const player_audio_config_t* config);

/**
 * 设置事件回调函数
 * @param player 播放器实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 错误码
 */
player_error_t linx_player_set_event_callback(linx_player_t* player, 
                                             player_event_callback_t callback, 
                                             void* user_data);

/**
 * 开始播放
 * @param player 播放器实例
 * @return 错误码
 */
player_error_t linx_player_start(linx_player_t* player);

/**
 * 暂停播放
 * @param player 播放器实例
 * @return 错误码
 */
player_error_t linx_player_pause(linx_player_t* player);

/**
 * 恢复播放
 * @param player 播放器实例
 * @return 错误码
 */
player_error_t linx_player_resume(linx_player_t* player);

/**
 * 停止播放
 * @param player 播放器实例
 * @return 错误码
 */
player_error_t linx_player_stop(linx_player_t* player);

/**
 * 添加音频数据到播放缓冲区
 * @param player 播放器实例
 * @param data 编码的音频数据
 * @param size 数据大小（字节）
 * @return 错误码
 */
player_error_t linx_player_feed_data(linx_player_t* player, const uint8_t* data, size_t size);

/**
 * 获取当前播放器状态
 * @param player 播放器实例
 * @return 播放器状态
 */
player_state_t linx_player_get_state(linx_player_t* player);

/**
 * 检查缓冲区是否为空
 * @param player 播放器实例
 * @return true表示缓冲区为空
 */
bool linx_player_is_buffer_empty(linx_player_t* player);

/**
 * 检查缓冲区是否已满
 * @param player 播放器实例
 * @return true表示缓冲区已满
 */
bool linx_player_is_buffer_full(linx_player_t* player);

/**
 * 获取缓冲区使用率（0.0-1.0）
 * @param player 播放器实例
 * @return 缓冲区使用率
 */
float linx_player_get_buffer_usage(linx_player_t* player);

/**
 * 清空播放缓冲区
 * @param player 播放器实例
 * @return 错误码
 */
player_error_t linx_player_clear_buffer(linx_player_t* player);

/**
 * 获取播放统计信息
 * @param player 播放器实例
 * @param total_bytes 总播放字节数（输出参数）
 * @param total_frames 总播放帧数（输出参数）
 * @return 错误码
 */
player_error_t linx_player_get_stats(linx_player_t* player, size_t* total_bytes, size_t* total_frames);

/**
 * 销毁播放器实例
 * @param player 播放器实例
 */
void linx_player_destroy(linx_player_t* player);

/**
 * 获取错误描述字符串
 * @param error 错误码
 * @return 错误描述字符串
 */
const char* linx_player_error_string(player_error_t error);

#ifdef __cplusplus
}
#endif

#endif // LINX_PLAYER_H