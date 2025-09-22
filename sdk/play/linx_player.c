#include "linx_player.h"
#include "../audio/audio_interface.h"
#include "../codecs/audio_codec.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// 默认配置常量
#define DEFAULT_BUFFER_CAPACITY (64 * 1024)  // 64KB 缓冲区
#define DECODE_BUFFER_SIZE 4096               // 解码缓冲区大小
#define PLAYBACK_THREAD_SLEEP_US 10000        // 播放线程休眠时间（微秒）

// 内部函数声明
static void* playback_thread_func(void* arg);
static player_error_t change_state(linx_player_t* player, player_state_t new_state);
static size_t circular_buffer_write(linx_player_t* player, const uint8_t* data, size_t size);
static size_t circular_buffer_read(linx_player_t* player, uint8_t* data, size_t size);
static size_t circular_buffer_available_space(linx_player_t* player);
static size_t circular_buffer_available_data(linx_player_t* player);

/**
 * 创建播放器实例
 */
linx_player_t* linx_player_create(AudioInterface* audio_interface, audio_codec_t* decoder) {
    if (!audio_interface || !decoder) {
        LOG_ERROR("Invalid parameters for player creation");
        return NULL;
    }
    
    linx_player_t* player = (linx_player_t*)calloc(1, sizeof(linx_player_t));
    if (!player) {
        LOG_ERROR("Failed to allocate memory for player");
        return NULL;
    }
    
    player->audio_interface = audio_interface;
    player->decoder = decoder;
    player->state = PLAYER_STATE_IDLE;
    player->initialized = false;
    player->running = false;
    
    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&player->state_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize state mutex");
        free(player);
        return NULL;
    }
    
    if (pthread_mutex_init(&player->buffer_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize buffer mutex");
        pthread_mutex_destroy(&player->state_mutex);
        free(player);
        return NULL;
    }
    
    if (pthread_cond_init(&player->buffer_cond, NULL) != 0) {
        LOG_ERROR("Failed to initialize buffer condition");
        pthread_mutex_destroy(&player->state_mutex);
        pthread_mutex_destroy(&player->buffer_mutex);
        free(player);
        return NULL;
    }
    
    return player;
}

/**
 * 初始化播放器
 */
player_error_t linx_player_init(linx_player_t* player, const player_audio_config_t* config) {
    if (!player || !config) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    if (player->initialized) {
        LOG_WARN("Player already initialized");
        return PLAYER_SUCCESS;
    }
    
    // 保存配置
    player->config = *config;
    
    // 分配音频缓冲区
    player->buffer_capacity = DEFAULT_BUFFER_CAPACITY;
    player->audio_buffer = (uint8_t*)malloc(player->buffer_capacity);
    if (!player->audio_buffer) {
        LOG_ERROR("Failed to allocate audio buffer");
        return PLAYER_ERROR_AUDIO_INTERFACE;
    }
    
    // 初始化缓冲区状态
    player->buffer_head = 0;
    player->buffer_tail = 0;
    player->buffer_count = 0;
    
    // 先初始化音频接口（初始化PortAudio）
    if (audio_interface_init(player->audio_interface) != 0) {
        LOG_ERROR("Failed to initialize audio interface");
        free(player->audio_buffer);
        player->audio_buffer = NULL;
        return PLAYER_ERROR_AUDIO_INTERFACE;
    }
    
    // 再配置音频接口（需要在PortAudio初始化后才能获取默认设备）
    audio_interface_set_config(player->audio_interface, 
                              config->sample_rate, 
                              config->frame_size,
                              config->channels, 
                              4,  // periods
                              config->buffer_size, 
                              config->frame_size);
    
    // 初始化播放
    if (audio_interface_init_play(player->audio_interface) != 0) {
        LOG_ERROR("Failed to initialize audio playback");
        free(player->audio_buffer);
        player->audio_buffer = NULL;
        return PLAYER_ERROR_AUDIO_INTERFACE;
    }
    
    player->initialized = true;
    LOG_INFO("Player initialized successfully");
    
    return PLAYER_SUCCESS;
}

/**
 * 设置事件回调函数
 */
player_error_t linx_player_set_event_callback(linx_player_t* player, 
                                             player_event_callback_t callback, 
                                             void* user_data) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    player->event_callback = callback;
    player->callback_user_data = user_data;
    pthread_mutex_unlock(&player->state_mutex);
    
    return PLAYER_SUCCESS;
}

/**
 * 开始播放
 */
player_error_t linx_player_start(linx_player_t* player) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    if (!player->initialized) {
        return PLAYER_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (player->state == PLAYER_STATE_PLAYING) {
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_SUCCESS;
    }
    
    if (player->state != PLAYER_STATE_IDLE && player->state != PLAYER_STATE_STOPPED) {
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_ERROR_INVALID_STATE;
    }
    
    // 启动播放线程
    player->running = true;
    if (pthread_create(&player->playback_thread, NULL, playback_thread_func, player) != 0) {
        LOG_ERROR("Failed to create playback thread");
        player->running = false;
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_ERROR_THREAD;
    }
    
    change_state(player, PLAYER_STATE_PLAYING);
    pthread_mutex_unlock(&player->state_mutex);
    
    LOG_INFO("Player started");
    return PLAYER_SUCCESS;
}

/**
 * 暂停播放
 */
player_error_t linx_player_pause(linx_player_t* player) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (player->state != PLAYER_STATE_PLAYING) {
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_ERROR_INVALID_STATE;
    }
    
    change_state(player, PLAYER_STATE_PAUSED);
    pthread_mutex_unlock(&player->state_mutex);
    
    LOG_INFO("Player paused");
    return PLAYER_SUCCESS;
}

/**
 * 恢复播放
 */
player_error_t linx_player_resume(linx_player_t* player) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (player->state != PLAYER_STATE_PAUSED) {
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_ERROR_INVALID_STATE;
    }
    
    change_state(player, PLAYER_STATE_PLAYING);
    pthread_cond_signal(&player->buffer_cond);
    pthread_mutex_unlock(&player->state_mutex);
    
    LOG_INFO("Player resumed");
    return PLAYER_SUCCESS;
}

/**
 * 停止播放
 */
player_error_t linx_player_stop(linx_player_t* player) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    
    if (player->state == PLAYER_STATE_IDLE || player->state == PLAYER_STATE_STOPPED) {
        pthread_mutex_unlock(&player->state_mutex);
        return PLAYER_SUCCESS;
    }
    
    // 停止播放线程
    player->running = false;
    change_state(player, PLAYER_STATE_STOPPED);
    pthread_cond_signal(&player->buffer_cond);
    pthread_mutex_unlock(&player->state_mutex);
    
    // 等待播放线程结束
    if (player->playback_thread) {
        pthread_join(player->playback_thread, NULL);
        player->playback_thread = 0;
    }
    
    // 清空缓冲区
    linx_player_clear_buffer(player);
    
    LOG_INFO("Player stopped");
    return PLAYER_SUCCESS;
}

/**
 * 添加音频数据到播放缓冲区
 */
player_error_t linx_player_feed_data(linx_player_t* player, const uint8_t* data, size_t size) {
    if (!player || !data || size == 0) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    if (!player->initialized) {
        return PLAYER_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&player->buffer_mutex);
    
    // 检查缓冲区是否有足够空间
    if (circular_buffer_available_space(player) < size) {
        pthread_mutex_unlock(&player->buffer_mutex);
        return PLAYER_ERROR_BUFFER_FULL;
    }
    
    // 写入数据到环形缓冲区
    size_t written = circular_buffer_write(player, data, size);
    
    // 通知播放线程有新数据
    pthread_cond_signal(&player->buffer_cond);
    pthread_mutex_unlock(&player->buffer_mutex);
    
    if (written != size) {
        LOG_WARN("Only wrote %zu of %zu bytes to buffer", written, size);
    }
    
    return PLAYER_SUCCESS;
}

/**
 * 获取当前播放器状态
 */
player_state_t linx_player_get_state(linx_player_t* player) {
    if (!player) {
        return PLAYER_STATE_ERROR;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    player_state_t state = player->state;
    pthread_mutex_unlock(&player->state_mutex);
    
    return state;
}

/**
 * 检查缓冲区是否为空
 */
bool linx_player_is_buffer_empty(linx_player_t* player) {
    if (!player) {
        return true;
    }
    
    pthread_mutex_lock(&player->buffer_mutex);
    bool empty = (player->buffer_count == 0);
    pthread_mutex_unlock(&player->buffer_mutex);
    
    return empty;
}

/**
 * 检查缓冲区是否已满
 */
bool linx_player_is_buffer_full(linx_player_t* player) {
    if (!player) {
        return false;
    }
    
    pthread_mutex_lock(&player->buffer_mutex);
    bool full = (player->buffer_count >= player->buffer_capacity);
    pthread_mutex_unlock(&player->buffer_mutex);
    
    return full;
}

/**
 * 获取缓冲区使用率
 */
float linx_player_get_buffer_usage(linx_player_t* player) {
    if (!player || player->buffer_capacity == 0) {
        return 0.0f;
    }
    
    pthread_mutex_lock(&player->buffer_mutex);
    float usage = (float)player->buffer_count / (float)player->buffer_capacity;
    pthread_mutex_unlock(&player->buffer_mutex);
    
    return usage;
}

/**
 * 清空播放缓冲区
 */
player_error_t linx_player_clear_buffer(linx_player_t* player) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->buffer_mutex);
    player->buffer_head = 0;
    player->buffer_tail = 0;
    player->buffer_count = 0;
    pthread_mutex_unlock(&player->buffer_mutex);
    
    return PLAYER_SUCCESS;
}

/**
 * 获取播放统计信息
 */
player_error_t linx_player_get_stats(linx_player_t* player, size_t* total_bytes, size_t* total_frames) {
    if (!player) {
        return PLAYER_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&player->state_mutex);
    if (total_bytes) {
        *total_bytes = player->total_bytes_played;
    }
    if (total_frames) {
        *total_frames = player->total_frames_played;
    }
    pthread_mutex_unlock(&player->state_mutex);
    
    return PLAYER_SUCCESS;
}

/**
 * 销毁播放器实例
 */
void linx_player_destroy(linx_player_t* player) {
    if (!player) {
        return;
    }
    
    // 停止播放
    linx_player_stop(player);
    
    // 销毁音频接口
    if (player->audio_interface) {
        audio_interface_destroy(player->audio_interface);
    }
    
    // 释放缓冲区
    if (player->audio_buffer) {
        free(player->audio_buffer);
    }
    
    // 销毁同步对象
    pthread_mutex_destroy(&player->state_mutex);
    pthread_mutex_destroy(&player->buffer_mutex);
    pthread_cond_destroy(&player->buffer_cond);
    
    free(player);
    LOG_INFO("Player destroyed");
}

/**
 * 获取错误描述字符串
 */
const char* linx_player_error_string(player_error_t error) {
    switch (error) {
        case PLAYER_SUCCESS:
            return "Success";
        case PLAYER_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case PLAYER_ERROR_NOT_INITIALIZED:
            return "Player not initialized";
        case PLAYER_ERROR_AUDIO_INTERFACE:
            return "Audio interface error";
        case PLAYER_ERROR_CODEC:
            return "Codec error";
        case PLAYER_ERROR_THREAD:
            return "Thread error";
        case PLAYER_ERROR_BUFFER_FULL:
            return "Buffer full";
        case PLAYER_ERROR_INVALID_STATE:
            return "Invalid state";
        default:
            return "Unknown error";
    }
}

// 内部函数实现

/**
 * 播放线程函数
 */
static void* playback_thread_func(void* arg) {
    linx_player_t* player = (linx_player_t*)arg;
    uint8_t encoded_buffer[DECODE_BUFFER_SIZE];
    int16_t decoded_buffer[DECODE_BUFFER_SIZE];
    
    LOG_INFO("Playback thread started");
    
    while (player->running) {
        pthread_mutex_lock(&player->state_mutex);
        player_state_t current_state = player->state;
        pthread_mutex_unlock(&player->state_mutex);
        
        // 如果暂停，等待恢复
        if (current_state == PLAYER_STATE_PAUSED) {
            pthread_mutex_lock(&player->buffer_mutex);
            pthread_cond_wait(&player->buffer_cond, &player->buffer_mutex);
            pthread_mutex_unlock(&player->buffer_mutex);
            continue;
        }
        
        // 如果不是播放状态，休眠后继续
        if (current_state != PLAYER_STATE_PLAYING) {
            usleep(PLAYBACK_THREAD_SLEEP_US);
            continue;
        }
        
        // 从缓冲区读取数据
        pthread_mutex_lock(&player->buffer_mutex);
        
        // 如果缓冲区为空，等待数据
        if (player->buffer_count == 0) {
            pthread_cond_wait(&player->buffer_cond, &player->buffer_mutex);
            pthread_mutex_unlock(&player->buffer_mutex);
            continue;
        }
        
        // 读取一帧编码数据（简化处理，假设每次读取固定大小）
        size_t to_read = (player->buffer_count < sizeof(encoded_buffer)) ? 
                        player->buffer_count : sizeof(encoded_buffer);
        size_t read_size = circular_buffer_read(player, encoded_buffer, to_read);
        pthread_mutex_unlock(&player->buffer_mutex);
        
        if (read_size > 0) {
            // 解码音频数据
            size_t decoded_size = 0;
            if (audio_codec_decode(player->decoder, encoded_buffer, read_size,
                                 decoded_buffer, sizeof(decoded_buffer)/sizeof(int16_t), 
                                 &decoded_size) == CODEC_SUCCESS) {
                
                // 播放解码后的音频
                if (audio_interface_write(player->audio_interface, decoded_buffer, decoded_size) == 0) {
                    // 更新统计信息
                    pthread_mutex_lock(&player->state_mutex);
                    player->total_bytes_played += read_size;
                    player->total_frames_played++;
                    pthread_mutex_unlock(&player->state_mutex);
                } else {
                    LOG_ERROR("Failed to write audio data");
                }
            } else {
                LOG_ERROR("Failed to decode audio data");
            }
        }
        
        // 短暂休眠避免过度占用CPU
        usleep(PLAYBACK_THREAD_SLEEP_US);
    }
    
    LOG_INFO("Playback thread ended");
    return NULL;
}

/**
 * 改变播放器状态
 */
static player_error_t change_state(linx_player_t* player, player_state_t new_state) {
    player_state_t old_state = player->state;
    player->state = new_state;
    
    // 调用事件回调
    if (player->event_callback) {
        player->event_callback(old_state, new_state, player->callback_user_data);
    }
    
    return PLAYER_SUCCESS;
}

/**
 * 环形缓冲区写入
 */
static size_t circular_buffer_write(linx_player_t* player, const uint8_t* data, size_t size) {
    size_t available = circular_buffer_available_space(player);
    size_t to_write = (size < available) ? size : available;
    
    if (to_write == 0) {
        return 0;
    }
    
    // 处理环形缓冲区的边界情况
    size_t first_part = player->buffer_capacity - player->buffer_head;
    if (to_write <= first_part) {
        // 数据可以一次性写入
        memcpy(player->audio_buffer + player->buffer_head, data, to_write);
        player->buffer_head = (player->buffer_head + to_write) % player->buffer_capacity;
    } else {
        // 需要分两次写入
        memcpy(player->audio_buffer + player->buffer_head, data, first_part);
        memcpy(player->audio_buffer, data + first_part, to_write - first_part);
        player->buffer_head = to_write - first_part;
    }
    
    player->buffer_count += to_write;
    return to_write;
}

/**
 * 环形缓冲区读取
 */
static size_t circular_buffer_read(linx_player_t* player, uint8_t* data, size_t size) {
    size_t available = circular_buffer_available_data(player);
    size_t to_read = (size < available) ? size : available;
    
    if (to_read == 0) {
        return 0;
    }
    
    // 处理环形缓冲区的边界情况
    size_t first_part = player->buffer_capacity - player->buffer_tail;
    if (to_read <= first_part) {
        // 数据可以一次性读取
        memcpy(data, player->audio_buffer + player->buffer_tail, to_read);
        player->buffer_tail = (player->buffer_tail + to_read) % player->buffer_capacity;
    } else {
        // 需要分两次读取
        memcpy(data, player->audio_buffer + player->buffer_tail, first_part);
        memcpy(data + first_part, player->audio_buffer, to_read - first_part);
        player->buffer_tail = to_read - first_part;
    }
    
    player->buffer_count -= to_read;
    return to_read;
}

/**
 * 获取环形缓冲区可用空间
 */
static size_t circular_buffer_available_space(linx_player_t* player) {
    return player->buffer_capacity - player->buffer_count;
}

/**
 * 获取环形缓冲区可用数据
 */
static size_t circular_buffer_available_data(linx_player_t* player) {
    return player->buffer_count;
}