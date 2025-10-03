#include "audio_packet_queue.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file audio_packet_queue.c
 * @brief 音频数据包队列管理实现
 * @details 提供音频数据包的队列操作实现，使用循环队列算法
 */

#define AUDIO_PACKET_QUEUE_TAG "AudioPacketQueue"
#define DEFAULT_PACKET_CAPACITY 1024  /**< 默认数据包缓冲区容量 */

// 音频数据包操作函数实现

AudioStreamPacket* audio_stream_packet_create(void) {
    AudioStreamPacket* packet = (AudioStreamPacket*)calloc(1, sizeof(AudioStreamPacket));
    if (!packet) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "创建音频数据包失败：内存分配失败");
        return NULL;
    }
    
    // 初始化默认容量的缓冲区
    packet->payload_capacity = DEFAULT_PACKET_CAPACITY;
    packet->payload = (uint8_t*)malloc(packet->payload_capacity);
    if (!packet->payload) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "创建音频数据包失败：负载缓冲区分配失败");
        free(packet);
        return NULL;
    }
    
    packet->payload_size = 0;
    packet->frame_duration = 60;  // 默认60ms帧
    packet->sample_rate = 16000;  // 默认16kHz采样率
    packet->timestamp = 0;
    
    LINX_LOGD(AUDIO_PACKET_QUEUE_TAG, "创建音频数据包成功，容量：%zu字节", packet->payload_capacity);
    return packet;
}

void audio_stream_packet_destroy(AudioStreamPacket* packet) {
    if (!packet) {
        return;
    }
    
    if (packet->payload) {
        free(packet->payload);
        packet->payload = NULL;
    }
    
    free(packet);
    LINX_LOGD(AUDIO_PACKET_QUEUE_TAG, "销毁音频数据包完成");
}

void audio_stream_packet_reset(AudioStreamPacket* packet) {
    if (!packet) {
        LINX_LOGW(AUDIO_PACKET_QUEUE_TAG, "重置音频数据包失败：无效参数");
        return;
    }
    
    packet->payload_size = 0;
    packet->timestamp = 0;
    // 保留其他配置参数
}

int audio_stream_packet_resize(AudioStreamPacket* packet, size_t new_capacity) {
    if (!packet) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "调整数据包大小失败：无效参数");
        return -1;
    }
    
    if (new_capacity == packet->payload_capacity) {
        return 0;  // 大小相同，无需调整
    }
    
    uint8_t* new_payload = (uint8_t*)realloc(packet->payload, new_capacity);
    if (!new_payload) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "调整数据包大小失败：内存重分配失败，目标大小：%zu", new_capacity);
        return -1;
    }
    
    packet->payload = new_payload;
    packet->payload_capacity = new_capacity;
    
    // 如果新容量小于当前数据大小，截断数据
    if (packet->payload_size > new_capacity) {
        packet->payload_size = new_capacity;
        LINX_LOGW(AUDIO_PACKET_QUEUE_TAG, "数据包大小调整导致数据截断，新大小：%zu", new_capacity);
    }
    
    LINX_LOGD(AUDIO_PACKET_QUEUE_TAG, "数据包大小调整成功，新容量：%zu字节", new_capacity);
    return 0;
}

// 音频数据包队列操作函数实现

int audio_packet_queue_init(AudioPacketQueue* queue, size_t capacity) {
    if (!queue || capacity == 0) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "初始化队列失败：无效参数");
        return -1;
    }
    
    queue->packets = (AudioStreamPacket**)calloc(capacity, sizeof(AudioStreamPacket*));
    if (!queue->packets) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "初始化队列失败：内存分配失败，容量：%zu", capacity);
        return -1;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    
    LINX_LOGI(AUDIO_PACKET_QUEUE_TAG, "音频数据包队列初始化成功，容量：%zu", capacity);
    return 0;
}

void audio_packet_queue_destroy(AudioPacketQueue* queue) {
    if (!queue) {
        return;
    }
    
    if (queue->packets) {
        // 销毁队列中剩余的所有数据包
        while (!audio_packet_queue_is_empty(queue)) {
            AudioStreamPacket* packet = audio_packet_queue_pop(queue);
            if (packet) {
                audio_stream_packet_destroy(packet);
            }
        }
        
        free(queue->packets);
        queue->packets = NULL;
    }
    
    queue->capacity = 0;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    
    LINX_LOGI(AUDIO_PACKET_QUEUE_TAG, "音频数据包队列销毁完成");
}

bool audio_packet_queue_push(AudioPacketQueue* queue, AudioStreamPacket* packet) {
    if (!queue || !packet) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "添加数据包失败：无效参数");
        return false;
    }
    
    if (audio_packet_queue_is_full(queue)) {
        LINX_LOGW(AUDIO_PACKET_QUEUE_TAG, "添加数据包失败：队列已满，当前大小：%zu", queue->size);
        return false;
    }
    
    queue->packets[queue->tail] = packet;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    LINX_LOGD(AUDIO_PACKET_QUEUE_TAG, "数据包添加成功，队列大小：%zu/%zu", queue->size, queue->capacity);
    return true;
}

AudioStreamPacket* audio_packet_queue_pop(AudioPacketQueue* queue) {
    if (!queue) {
        LINX_LOGE(AUDIO_PACKET_QUEUE_TAG, "取出数据包失败：无效参数");
        return NULL;
    }
    
    if (audio_packet_queue_is_empty(queue)) {
        return NULL;  // 队列为空，正常情况
    }
    
    AudioStreamPacket* packet = queue->packets[queue->head];
    queue->packets[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    LINX_LOGD(AUDIO_PACKET_QUEUE_TAG, "数据包取出成功，队列大小：%zu/%zu", queue->size, queue->capacity);
    return packet;
}

AudioStreamPacket* audio_packet_queue_peek(const AudioPacketQueue* queue) {
    if (!queue || audio_packet_queue_is_empty(queue)) {
        return NULL;
    }
    
    return queue->packets[queue->head];
}

bool audio_packet_queue_is_empty(const AudioPacketQueue* queue) {
    return !queue || queue->size == 0;
}

bool audio_packet_queue_is_full(const AudioPacketQueue* queue) {
    return queue && queue->size >= queue->capacity;
}

size_t audio_packet_queue_size(const AudioPacketQueue* queue) {
    return queue ? queue->size : 0;
}

size_t audio_packet_queue_capacity(const AudioPacketQueue* queue) {
    return queue ? queue->capacity : 0;
}

void audio_packet_queue_clear(AudioPacketQueue* queue) {
    if (!queue) {
        LINX_LOGW(AUDIO_PACKET_QUEUE_TAG, "清空队列失败：无效参数");
        return;
    }
    
    size_t cleared_count = 0;
    while (!audio_packet_queue_is_empty(queue)) {
        AudioStreamPacket* packet = audio_packet_queue_pop(queue);
        if (packet) {
            audio_stream_packet_destroy(packet);
            cleared_count++;
        }
    }
    
    LINX_LOGI(AUDIO_PACKET_QUEUE_TAG, "队列清空完成，销毁了%zu个数据包", cleared_count);
}