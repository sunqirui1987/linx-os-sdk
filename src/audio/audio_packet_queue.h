#ifndef AUDIO_PACKET_QUEUE_H
#define AUDIO_PACKET_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_packet_queue.h
 * @brief 音频数据包队列管理
 * @details 提供音频数据包的队列操作，支持循环队列实现
 */

// 前向声明
typedef struct AudioStreamPacket AudioStreamPacket;

/**
 * @brief 音频流数据包结构体
 * @details 封装音频数据包的所有信息，包括负载数据、时间戳等
 */
struct AudioStreamPacket {
    uint8_t* payload;               /**< 音频数据负载 */
    size_t payload_size;            /**< 当前负载大小 */
    size_t payload_capacity;        /**< 负载缓冲区容量 */
    int frame_duration;             /**< 帧持续时间（毫秒） */
    int sample_rate;                /**< 采样率 */
    uint32_t timestamp;             /**< 时间戳 */
};

/**
 * @brief 音频数据包队列结构体
 * @details 使用循环队列实现的音频数据包队列
 */
typedef struct AudioPacketQueue {
    AudioStreamPacket** packets;    /**< 数据包指针数组 */
    size_t capacity;                /**< 队列容量 */
    size_t size;                    /**< 当前队列大小 */
    size_t head;                    /**< 队列头索引 */
    size_t tail;                    /**< 队列尾索引 */
} AudioPacketQueue;

// 音频数据包操作函数

/**
 * @brief 创建音频流数据包
 * @return 新创建的数据包指针，失败返回NULL
 */
AudioStreamPacket* audio_stream_packet_create(void);

/**
 * @brief 销毁音频流数据包
 * @param packet 要销毁的数据包指针
 */
void audio_stream_packet_destroy(AudioStreamPacket* packet);

/**
 * @brief 重置音频流数据包
 * @param packet 要重置的数据包指针
 * @details 清空数据包内容但保留缓冲区
 */
void audio_stream_packet_reset(AudioStreamPacket* packet);

/**
 * @brief 调整音频流数据包缓冲区大小
 * @param packet 数据包指针
 * @param new_capacity 新的缓冲区容量
 * @return 0成功，负数失败
 */
int audio_stream_packet_resize(AudioStreamPacket* packet, size_t new_capacity);

// 音频数据包队列操作函数

/**
 * @brief 初始化音频数据包队列
 * @param queue 队列指针
 * @param capacity 队列容量
 * @return 0成功，负数失败
 */
int audio_packet_queue_init(AudioPacketQueue* queue, size_t capacity);

/**
 * @brief 销毁音频数据包队列
 * @param queue 队列指针
 * @details 会自动销毁队列中剩余的所有数据包
 */
void audio_packet_queue_destroy(AudioPacketQueue* queue);

/**
 * @brief 向队列中添加数据包
 * @param queue 队列指针
 * @param packet 要添加的数据包
 * @return true成功，false失败（队列已满）
 */
bool audio_packet_queue_push(AudioPacketQueue* queue, AudioStreamPacket* packet);

/**
 * @brief 从队列中取出数据包
 * @param queue 队列指针
 * @return 数据包指针，队列为空时返回NULL
 */
AudioStreamPacket* audio_packet_queue_pop(AudioPacketQueue* queue);

/**
 * @brief 查看队列头部数据包但不移除
 * @param queue 队列指针
 * @return 数据包指针，队列为空时返回NULL
 */
AudioStreamPacket* audio_packet_queue_peek(const AudioPacketQueue* queue);

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return true为空，false不为空
 */
bool audio_packet_queue_is_empty(const AudioPacketQueue* queue);

/**
 * @brief 检查队列是否已满
 * @param queue 队列指针
 * @return true已满，false未满
 */
bool audio_packet_queue_is_full(const AudioPacketQueue* queue);

/**
 * @brief 获取队列当前大小
 * @param queue 队列指针
 * @return 队列中数据包的数量
 */
size_t audio_packet_queue_size(const AudioPacketQueue* queue);

/**
 * @brief 获取队列容量
 * @param queue 队列指针
 * @return 队列的最大容量
 */
size_t audio_packet_queue_capacity(const AudioPacketQueue* queue);

/**
 * @brief 清空队列
 * @param queue 队列指针
 * @details 会销毁队列中的所有数据包
 */
void audio_packet_queue_clear(AudioPacketQueue* queue);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_PACKET_QUEUE_H