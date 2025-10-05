/**
 * @file audio_service.c
 * @brief 音频服务实现
 * @details 提供音频录制、播放、编解码、唤醒词检测等功能的统一服务实现
 */

#include "audio_service.h"
#include "audio_packet_queue.h"
#include "audio_task_queue.h"
#include "timestamp_queue.h"
#include "codecs/opus_codec.h"
#include "../common/log/linx_log.h"
#include "../third/opus/silk/SigProc_FIX.h"
#include "../common/std/vector.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 内部常量和宏定义
// ============================================================================

#define AUDIO_SERVICE_TAG "AudioService"

// ============================================================================
// 内部函数声明
// ============================================================================

static void trigger_callbacks(AudioService* service, const char* event_type, const void* event_data);
static bool is_component_available(AudioService* service, const char* component_name);

// ============================================================================
// 内部工具函数
// ============================================================================

static void get_current_time(struct timespec* ts) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

static long time_diff_ms(const struct timespec* start, const struct timespec* end) {
    long diff_sec = end->tv_sec - start->tv_sec;
    long diff_nsec = end->tv_nsec - start->tv_nsec;
    return diff_sec * 1000 + diff_nsec / 1000000;
}

static void set_event_bit(AudioService* service, uint32_t bit) {
    pthread_mutex_lock(&service->event_mutex);
    service->event_bits |= bit;
    pthread_mutex_unlock(&service->event_mutex);
    pthread_cond_broadcast(&service->audio_queue_cv);
}

static void clear_event_bit(AudioService* service, uint32_t bit) {
    pthread_mutex_lock(&service->event_mutex);
    service->event_bits &= ~bit;
    pthread_mutex_unlock(&service->event_mutex);
}

static uint32_t get_event_bits(AudioService* service) {
    pthread_mutex_lock(&service->event_mutex);
    uint32_t bits = service->event_bits;
    pthread_mutex_unlock(&service->event_mutex);
    return bits;
}

// ============================================================================
// 回调处理函数
// ============================================================================

static void trigger_callbacks(AudioService* service, const char* event_type, const void* event_data) {
    if (!service) {
        LINX_LOGW(AUDIO_SERVICE_TAG, "[CALLBACK] ❌ 服务为空，无法触发回调");
        return;
    }
    
    LINX_LOGD(AUDIO_SERVICE_TAG, "[CALLBACK] 准备触发回调: %s", event_type);
    
    if (strcmp(event_type, "send_queue_available") == 0 && service->callbacks.on_send_queue_available) {
        LINX_LOGI(AUDIO_SERVICE_TAG, "[CALLBACK] 🔔 触发发送队列可用回调");
        service->callbacks.on_send_queue_available(service->callbacks.user_data);
        LINX_LOGD(AUDIO_SERVICE_TAG, "[CALLBACK] ✅ 发送队列可用回调执行完成");
    } else if (strcmp(event_type, "wake_word_detected") == 0 && service->callbacks.on_wake_word_detected) {
        const char* wake_word = (const char*)event_data;
        LINX_LOGI(AUDIO_SERVICE_TAG, "[CALLBACK] 🔔 触发唤醒词检测回调: %s", wake_word ? wake_word : "NULL");
        service->callbacks.on_wake_word_detected(wake_word, service->callbacks.user_data);
        LINX_LOGD(AUDIO_SERVICE_TAG, "[CALLBACK] ✅ 唤醒词检测回调执行完成");
    } else if (strcmp(event_type, "vad_change") == 0 && service->callbacks.on_vad_change) {
        bool speaking = *(const bool*)event_data;
        LINX_LOGI(AUDIO_SERVICE_TAG, "[CALLBACK] 🔔 触发VAD状态变化回调: %s", speaking ? "说话中" : "静音");
        service->callbacks.on_vad_change(speaking, service->callbacks.user_data);
        LINX_LOGD(AUDIO_SERVICE_TAG, "[CALLBACK] ✅ VAD状态变化回调执行完成");
    } else if (strcmp(event_type, "testing_queue_full") == 0 && service->callbacks.on_audio_testing_queue_full) {
        LINX_LOGI(AUDIO_SERVICE_TAG, "[CALLBACK] 🔔 触发测试队列满回调");
        service->callbacks.on_audio_testing_queue_full(service->callbacks.user_data);
        LINX_LOGD(AUDIO_SERVICE_TAG, "[CALLBACK] ✅ 测试队列满回调执行完成");
    } else {
        LINX_LOGW(AUDIO_SERVICE_TAG, "[CALLBACK] ❌ 未知回调类型或回调函数未设置: %s", event_type);
        if (strcmp(event_type, "send_queue_available") == 0) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "[CALLBACK] 发送队列可用回调函数: %p", service->callbacks.on_send_queue_available);
        }
    }
}

static void audio_processor_output_callback(const int16_t* data, size_t size, void* user_data) {
    AudioService* service = (AudioService*)user_data;
    if (!service || !data || size == 0) {
        LINX_LOGW(AUDIO_SERVICE_TAG, "[PROCESS] ❌ 音频处理器回调参数无效: service=%p, data=%p, size=%zu", service, data, size);
        return;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "[PROCESS] 🎵 音频处理器输出回调被调用，数据大小: %zu 个样本", size);
    
    // 计算音频数据的能量级别
    float energy = 0.0f;
    for (size_t i = 0; i < size; i++) {
        energy += (float)(data[i] * data[i]);
    }
    energy = sqrtf(energy / size);
    LINX_LOGD(AUDIO_SERVICE_TAG, "[PROCESS] 处理后音频数据能量级别: %.2f", energy);
    
    // 创建音频数据副本
    int16_t* audio_data = (int16_t*)malloc(size * sizeof(int16_t));
    if (!audio_data) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "[PROCESS] ❌ 创建音频数据副本失败，大小: %zu", size * sizeof(int16_t));
        return;
    }
    memcpy(audio_data, data, size * sizeof(int16_t));
    LINX_LOGD(AUDIO_SERVICE_TAG, "[PROCESS] ✅ 音频数据副本创建成功");
    
    // 创建新的音频任务
    AudioTask* task = audio_task_create(AUDIO_TASK_PROCESS_AUDIO, audio_data, size * sizeof(int16_t));
    if (!task) {
        free(audio_data);
        LINX_LOGE(AUDIO_SERVICE_TAG, "[PROCESS] ❌ 创建音频任务失败");
        return;
    }
    LINX_LOGD(AUDIO_SERVICE_TAG, "[PROCESS] ✅ 音频任务创建成功，任务类型: AUDIO_TASK_PROCESS_AUDIO");
    
    // 推送到编码队列
    pthread_mutex_lock(&service->audio_queue_mutex);
    
    // 检查编码队列状态
    size_t encode_queue_size = audio_task_queue_size(&service->audio_encode_queue);
    LINX_LOGD(AUDIO_SERVICE_TAG, "[PROCESS] 编码队列当前大小: %zu/%d", encode_queue_size, MAX_ENCODE_TASKS_IN_QUEUE);
    
    while (audio_task_queue_is_full(&service->audio_encode_queue) && !service->service_stopped) {
        LINX_LOGW(AUDIO_SERVICE_TAG, "[PROCESS] 编码队列已满，等待空间...");
        pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
    }
    
    if (!service->service_stopped) {
        if (!audio_task_queue_push(&service->audio_encode_queue, task)) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "[PROCESS] ❌ 推送任务到编码队列失败");
            audio_task_destroy(task);
        } else {
            LINX_LOGI(AUDIO_SERVICE_TAG, "[PROCESS] ✅ 音频任务已推送到编码队列，队列大小: %zu", audio_task_queue_size(&service->audio_encode_queue));
            pthread_cond_broadcast(&service->audio_queue_cv);
        }
    } else {
        LINX_LOGW(AUDIO_SERVICE_TAG, "[PROCESS] 服务已停止，丢弃音频任务");
        audio_task_destroy(task);
    }
    pthread_mutex_unlock(&service->audio_queue_mutex);
}

static void audio_processor_vad_callback(bool speaking, void* user_data) {
    AudioService* service = (AudioService*)user_data;
    if (!service) {
        return;
    }
    
    service->voice_detected = speaking;
    
    // 触发VAD事件
    trigger_callbacks(service, "vad_change", &speaking);
}

// ============================================================================
// 工作线程函数
// ============================================================================

// AudioService::AudioInputTask
static void* audio_input_thread_func(void* arg) {
    AudioService* service;
    uint32_t bits;
    int samples;
    vector_int16_t_t audio_data;
    int16_t* mono_data;
    size_t i, j;
    
    service = (AudioService*)arg;
    
    // 初始化vector
    vector_int16_t_init(&audio_data);
    
    while (1) {
        pthread_mutex_lock(&service->event_mutex);
        while (!(service->event_bits & (AS_EVENT_AUDIO_TESTING_RUNNING | 
                                       AS_EVENT_WAKE_WORD_RUNNING | 
                                       AS_EVENT_AUDIO_PROCESSOR_RUNNING)) && 
               !service->service_stopped) {
            pthread_cond_wait(&service->audio_queue_cv, &service->event_mutex);
        }
        bits = service->event_bits;
        pthread_mutex_unlock(&service->event_mutex);
        
        if (service->service_stopped) {
            break;
        }
        
        if (service->audio_input_need_warmup) {
            service->audio_input_need_warmup = false;
            usleep(120000); /* 120ms warmup */
            continue;
        }
        
        /* Used for audio testing in NetworkConfiguring mode by clicking the BOOT button */
        if (bits & AS_EVENT_AUDIO_TESTING_RUNNING) {
            if (audio_packet_queue_size(&service->audio_testing_queue) >= 
                AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Audio testing queue is full, stopping audio testing");
                clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
                trigger_callbacks(service, "testing_queue_full", NULL);
                continue;
            }
            
            samples = OPUS_FRAME_DURATION_MS * 16000 / 1000;
            if (audio_service_read_audio_data(service, &audio_data, 16000, samples) == 0) {
                /* If input channels is 2, we need to fetch the left channel data */
                if (service->audio_interface->channels == 2) {
                    mono_data = (int16_t*)malloc(samples * sizeof(int16_t));
                    if (mono_data) {
                        int16_t* data_ptr = vector_int16_t_data(&audio_data);
                        for (i = 0, j = 0; i < (size_t)samples; i++, j += 2) {
                            mono_data[i] = data_ptr[j];
                        }
                        audio_service_push_task_to_encode_queue(service, AUDIO_TASK_PROCESS_AUDIO, mono_data, samples * sizeof(int16_t));
                        free(mono_data);
                    }
                } else {
                    audio_service_push_task_to_encode_queue(service, AUDIO_TASK_PROCESS_AUDIO, vector_int16_t_data(&audio_data), vector_int16_t_size(&audio_data) * sizeof(int16_t));
                }
            }
            continue;
        }
        
        /* Feed the wake word */
        if (bits & AS_EVENT_WAKE_WORD_RUNNING) {
            if (!is_component_available(service, COMPONENT_WAKE_WORD)) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Wake word detection enabled but component not available, disabling");
                clear_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
                service->current_features.wake_word_detection = false;
                continue;
            }
            
            samples = wake_word_interface_get_feed_size(service->wake_word);
            if (samples > 0) {
                if (audio_service_read_audio_data(service, &audio_data, 16000, samples) == 0) {
                    wake_word_interface_feed(service->wake_word, vector_int16_t_data(&audio_data), vector_int16_t_size(&audio_data));
                }
            }
            continue;
        }
        
        /* Feed the audio processor */
        if (bits & AS_EVENT_AUDIO_PROCESSOR_RUNNING) {
            if (!is_component_available(service, COMPONENT_AUDIO_PROCESSOR)) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Audio processor enabled but component not available, disabling");
                clear_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
                service->current_features.voice_processing = false;
                continue;
            }
            
            samples = audio_processor_get_feed_size(service->audio_processor);
            if (samples > 0) {
                if (audio_service_read_audio_data(service, &audio_data, 16000, samples) == 0) {
                    audio_processor_feed(service->audio_processor, vector_int16_t_data(&audio_data), vector_int16_t_size(&audio_data));
                }
            }
            continue;
        }
        
        LINX_LOGE(AUDIO_SERVICE_TAG, "Should not be here, bits: %x", bits);
        break;
    }
    
    // 清理vector资源
    vector_int16_t_destroy(&audio_data);
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "Audio input task stopped");
    return NULL;
}

static void* audio_output_thread_func(void* arg) {
    AudioService* service = (AudioService*)arg;
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "[PLAY] 🎵 音频输出线程启动");
    
    while (!service->service_stopped) {
        pthread_mutex_lock(&service->audio_queue_mutex);
        
        size_t playback_queue_size = audio_task_queue_size(&service->audio_playback_queue);
        if (playback_queue_size > 0) {
            LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 播放队列有 %zu 个任务等待处理", playback_queue_size);
        }
        
        while (audio_task_queue_is_empty(&service->audio_playback_queue) && !service->service_stopped) {
            LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 播放队列为空，等待任务...");
            pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
        }
        
        if (service->service_stopped) {
            LINX_LOGI(AUDIO_SERVICE_TAG, "[PLAY] 服务已停止，退出播放线程");
            pthread_mutex_unlock(&service->audio_queue_mutex);
            break;
        }
        
        AudioTask* task = audio_task_queue_pop(&service->audio_playback_queue);
        pthread_cond_broadcast(&service->audio_queue_cv);
        pthread_mutex_unlock(&service->audio_queue_mutex);
        
        if (task) {
            LINX_LOGI(AUDIO_SERVICE_TAG, "[PLAY] 🎵 从播放队列获取任务，数据大小: %zu 字节", task->data_size);
            
            // 播放音频数据
            if (task->data && task->data_size > 0) {
                size_t sample_count = task->data_size / sizeof(int16_t);
                
                LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 准备播放 %zu 个样本", sample_count);
                
                // 计算音频数据的能量级别
                int16_t* samples = (int16_t*)task->data;
                float energy = 0.0f;
                for (size_t i = 0; i < sample_count; i++) {
                    energy += (float)(samples[i] * samples[i]);
                }
                energy = sqrtf(energy / sample_count);
                LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 播放音频数据能量级别: %.2f", energy);
                
                if (service->audio_interface && service->audio_interface->vtable && 
                    service->audio_interface->vtable->write) {
                    
                    LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 调用音频接口写入数据");
                    int write_result = service->audio_interface->vtable->write(service->audio_interface, 
                                                          (short*)task->data, sample_count);
                    
                    if (write_result == 0) {
                        LINX_LOGI(AUDIO_SERVICE_TAG, "[PLAY] ✅ 音频数据写入成功，样本数: %zu", sample_count);
                    } else {
                        LINX_LOGE(AUDIO_SERVICE_TAG, "[PLAY] ❌ 音频数据写入失败，错误码: %d", write_result);
                    }
                } else {
                    LINX_LOGE(AUDIO_SERVICE_TAG, "[PLAY] ❌ 音频接口或写入函数不可用: interface=%p", service->audio_interface);
                    if (service->audio_interface) {
                        LINX_LOGE(AUDIO_SERVICE_TAG, "[PLAY] vtable=%p, write=%p", 
                                 service->audio_interface->vtable, 
                                 service->audio_interface->vtable ? service->audio_interface->vtable->write : NULL);
                    }
                }
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "[PLAY] ❌ 播放任务数据无效: data=%p, size=%zu", task->data, task->data_size);
            }
            
            get_current_time(&service->last_output_time);
            service->debug_statistics.playback_count++;
            
            LINX_LOGD(AUDIO_SERVICE_TAG, "[PLAY] 播放统计: 总播放次数 %u", service->debug_statistics.playback_count);
            
            audio_task_destroy(task);
        } else {
            LINX_LOGW(AUDIO_SERVICE_TAG, "[PLAY] 从播放队列获取的任务为空");
        }
    }
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "[PLAY] 音频输出线程停止");
    return NULL;
}

static void* opus_codec_thread_func(void* arg) {
    AudioService* service = (AudioService*)arg;
    
    while (!service->service_stopped) {
        pthread_mutex_lock(&service->audio_queue_mutex);
        while (service->service_stopped == false &&
               (audio_packet_queue_is_empty(&service->audio_decode_queue) || 
                audio_task_queue_size(&service->audio_playback_queue) >= MAX_PLAYBACK_TASKS_IN_QUEUE) &&
               (audio_task_queue_is_empty(&service->audio_encode_queue) || 
                audio_packet_queue_size(&service->audio_send_queue) >= MAX_SEND_PACKETS_IN_QUEUE)) {
            pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
        }
        
        if (service->service_stopped) {
            pthread_mutex_unlock(&service->audio_queue_mutex);
            break;
        }
        
        // 解码处理
        if (!audio_packet_queue_is_empty(&service->audio_decode_queue) && 
            audio_task_queue_size(&service->audio_playback_queue) < MAX_PLAYBACK_TASKS_IN_QUEUE) {
            
            LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 开始处理解码队列，队列大小: %zu", audio_packet_queue_size(&service->audio_decode_queue));
            
            AudioStreamPacket* packet = audio_packet_queue_pop(&service->audio_decode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (packet) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "[DECODE] 🎵 从解码队列获取音频包，负载大小: %zu 字节", packet->payload_size);
                
                // 解码逻辑
                size_t estimated_samples = (packet->sample_rate * packet->frame_duration) / 1000;
                LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 估计解码样本数: %zu (采样率: %d, 帧时长: %dms)", 
                         estimated_samples, packet->sample_rate, packet->frame_duration);
                
                int16_t* decoded_data = (int16_t*)malloc(estimated_samples * sizeof(int16_t));
                
                if (decoded_data && service->opus_decoder && packet->payload && packet->payload_size > 0) {
                    size_t decoded_size = 0;
                    
                    LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 开始Opus解码，输入: %zu 字节", packet->payload_size);
                    
                    codec_error_t result = audio_codec_decode(service->opus_decoder, 
                                                            packet->payload, packet->payload_size,
                                                            decoded_data, estimated_samples, &decoded_size);
                    if (result == CODEC_SUCCESS) {
                        LINX_LOGI(AUDIO_SERVICE_TAG, "[DECODE] ✅ Opus解码成功，编码: %zu 字节 -> 解码: %zu 样本", 
                                 packet->payload_size, decoded_size);
                        
                        AudioTask* task = audio_task_create(AUDIO_TASK_PLAY_SOUND, decoded_data, decoded_size * sizeof(int16_t));
                        if (task) {
                            task->timestamp = packet->timestamp;
                            
                            LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 创建播放任务，数据大小: %zu 字节", decoded_size * sizeof(int16_t));
                            
                            pthread_mutex_lock(&service->audio_queue_mutex);
                            size_t playback_queue_size = audio_task_queue_size(&service->audio_playback_queue);
                            LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 播放队列当前大小: %zu/%d", playback_queue_size, MAX_PLAYBACK_TASKS_IN_QUEUE);
                            
                            if (!audio_task_queue_push(&service->audio_playback_queue, task)) {
                                LINX_LOGE(AUDIO_SERVICE_TAG, "[DECODE] ❌ 推送到播放队列失败");
                                audio_task_destroy(task);
                            } else {
                                LINX_LOGI(AUDIO_SERVICE_TAG, "[DECODE] ✅ 播放任务已推送到播放队列，队列大小: %zu", audio_task_queue_size(&service->audio_playback_queue));
                                pthread_cond_broadcast(&service->audio_queue_cv);
                            }
                            pthread_mutex_unlock(&service->audio_queue_mutex);
                            
                            service->debug_statistics.decode_count++;
                            LINX_LOGD(AUDIO_SERVICE_TAG, "[DECODE] 解码统计: 总解码次数 %u", service->debug_statistics.decode_count);
                        } else {
                            LINX_LOGE(AUDIO_SERVICE_TAG, "[DECODE] ❌ 创建播放任务失败");
                            free(decoded_data);
                        }
                    } else {
                        LINX_LOGE(AUDIO_SERVICE_TAG, "[DECODE] ❌ Opus解码失败，错误码: %d", result);
                        free(decoded_data);
                    }
                } else {
                    LINX_LOGE(AUDIO_SERVICE_TAG, "[DECODE] ❌ 解码器或数据无效: decoder=%p, data=%p, payload=%p, size=%zu", 
                             service->opus_decoder, decoded_data, packet->payload, packet->payload_size);
                    if (decoded_data) free(decoded_data);
                }
                audio_stream_packet_destroy(packet);
            } else {
                LINX_LOGW(AUDIO_SERVICE_TAG, "[DECODE] 从解码队列获取的音频包为空");
            }
            pthread_mutex_lock(&service->audio_queue_mutex);
        }
        
        // 编码处理
        if (!audio_task_queue_is_empty(&service->audio_encode_queue) && 
            audio_packet_queue_size(&service->audio_send_queue) < MAX_SEND_PACKETS_IN_QUEUE) {
            
            LINX_LOGD(AUDIO_SERVICE_TAG, "[ENCODE] 开始处理编码队列，队列大小: %zu", audio_task_queue_size(&service->audio_encode_queue));
            
            AudioTask* task = audio_task_queue_pop(&service->audio_encode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (task) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "[ENCODE] 🎵 从编码队列获取任务，数据大小: %zu 字节", task->data_size);
                
                AudioStreamPacket* packet = audio_stream_packet_create();
                if (packet) {
                    packet->frame_duration = OPUS_FRAME_DURATION_MS;
                    packet->sample_rate = service->config.output_format.sample_rate;
                    packet->timestamp = task->timestamp;
                    
                    LINX_LOGD(AUDIO_SERVICE_TAG, "[ENCODE] 创建音频包，帧时长: %dms, 采样率: %d", packet->frame_duration, packet->sample_rate);
                    
                    if (service->opus_encoder && task->data && task->data_size > 0) {
                        size_t encoded_size = 0;
                        size_t sample_count = task->data_size / sizeof(int16_t);
                        
                        LINX_LOGD(AUDIO_SERVICE_TAG, "[ENCODE] 开始Opus编码，样本数: %zu", sample_count);
                        
                        codec_error_t result = audio_codec_encode(service->opus_encoder,
                                                                (int16_t*)task->data, sample_count,
                                                                packet->payload, packet->payload_capacity, &encoded_size);
                        if (result == CODEC_SUCCESS) {
                            packet->payload_size = encoded_size;
                            
                            LINX_LOGI(AUDIO_SERVICE_TAG, "[ENCODE] ✅ Opus编码成功，原始: %zu 样本 -> 编码: %zu 字节", sample_count, encoded_size);
                            
                            pthread_mutex_lock(&service->audio_queue_mutex);
                            size_t send_queue_size = audio_packet_queue_size(&service->audio_send_queue);
                            LINX_LOGD(AUDIO_SERVICE_TAG, "[ENCODE] 发送队列当前大小: %zu/%d", send_queue_size, MAX_SEND_PACKETS_IN_QUEUE);
                            
                            if (!audio_packet_queue_push(&service->audio_send_queue, packet)) {
                                LINX_LOGE(AUDIO_SERVICE_TAG, "[ENCODE] ❌ 推送到发送队列失败");
                                audio_stream_packet_destroy(packet);
                            } else {
                                LINX_LOGI(AUDIO_SERVICE_TAG, "[ENCODE] ✅ 音频包已推送到发送队列，队列大小: %zu", audio_packet_queue_size(&service->audio_send_queue));
                                pthread_cond_broadcast(&service->audio_queue_cv);
                                
                                LINX_LOGI(AUDIO_SERVICE_TAG, "[ENCODE] 🔔 触发发送队列可用回调");
                                trigger_callbacks(service, "send_queue_available", NULL);
                            }
                            pthread_mutex_unlock(&service->audio_queue_mutex);
                            
                            service->debug_statistics.encode_count++;
                            LINX_LOGD(AUDIO_SERVICE_TAG, "[ENCODE] 编码统计: 总编码次数 %u", service->debug_statistics.encode_count);
                        } else {
                            LINX_LOGE(AUDIO_SERVICE_TAG, "[ENCODE] ❌ Opus编码失败，错误码: %d", result);
                            audio_stream_packet_destroy(packet);
                        }
                    } else {
                        LINX_LOGE(AUDIO_SERVICE_TAG, "[ENCODE] ❌ 编码器或数据无效: encoder=%p, data=%p, size=%zu", 
                                 service->opus_encoder, task->data, task->data_size);
                        audio_stream_packet_destroy(packet);
                    }
                } else {
                    LINX_LOGE(AUDIO_SERVICE_TAG, "[ENCODE] ❌ 创建音频包失败");
                }
                audio_task_destroy(task);
            } else {
                LINX_LOGW(AUDIO_SERVICE_TAG, "[ENCODE] 从编码队列获取的任务为空");
            }
            pthread_mutex_lock(&service->audio_queue_mutex);
        }
        
        pthread_mutex_unlock(&service->audio_queue_mutex);
        usleep(1000);
    }
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "Opus codec thread stopped");
    return NULL;
}

// ============================================================================
// 核心生命周期管理API实现
// ============================================================================

AudioService* audio_service_create(const AudioServiceConfig* config) {
    AudioService* service = (AudioService*)calloc(1, sizeof(AudioService));
    if (!service) {
        return NULL;
    }
    
    // 复制配置
    if (config) {
        service->config = *config;
    } else {
        audio_service_config_init_default(&service->config);
    }
    
    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&service->audio_queue_mutex, NULL) != 0) {
        free(service);
        return NULL;
    }
    
    if (pthread_mutex_init(&service->event_mutex, NULL) != 0) {
        pthread_mutex_destroy(&service->audio_queue_mutex);
        free(service);
        return NULL;
    }
    
    if (pthread_cond_init(&service->audio_queue_cv, NULL) != 0) {
        pthread_mutex_destroy(&service->event_mutex);
        pthread_mutex_destroy(&service->audio_queue_mutex);
        free(service);
        return NULL;
    }
    
    // 初始化队列
    if (audio_packet_queue_init(&service->audio_decode_queue, MAX_DECODE_PACKETS_IN_QUEUE) != 0 ||
        audio_packet_queue_init(&service->audio_send_queue, MAX_SEND_PACKETS_IN_QUEUE) != 0 ||
        audio_packet_queue_init(&service->audio_testing_queue, AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS) != 0 ||
        audio_task_queue_init(&service->audio_encode_queue, MAX_ENCODE_TASKS_IN_QUEUE, true) != 0 ||
        audio_task_queue_init(&service->audio_playback_queue, MAX_PLAYBACK_TASKS_IN_QUEUE, true) != 0 ||
        timestamp_queue_init(&service->timestamp_queue, MAX_TIMESTAMPS_IN_QUEUE, true) != 0) {
        
        audio_service_destroy(service);
        return NULL;
    }
    
    // 初始化状态
    service->service_stopped = true;
    service->voice_detected = false;
    service->audio_input_need_warmup = false;
    service->event_bits = 0;
    
    // 初始化时间
    get_current_time(&service->last_input_time);
    get_current_time(&service->last_output_time);
    
    return service;
}

void audio_service_destroy(AudioService* service) {
    if (!service) {
        return;
    }
    
    // 停止服务
    if (!service->service_stopped) {
        audio_service_stop(service);
    }
    
    // 销毁队列
    audio_packet_queue_destroy(&service->audio_decode_queue);
    audio_packet_queue_destroy(&service->audio_send_queue);
    audio_packet_queue_destroy(&service->audio_testing_queue);
    audio_task_queue_destroy(&service->audio_encode_queue);
    audio_task_queue_destroy(&service->audio_playback_queue);
    timestamp_queue_destroy(&service->timestamp_queue);
    
    // 销毁同步对象
    pthread_cond_destroy(&service->audio_queue_cv);
    pthread_mutex_destroy(&service->event_mutex);
    pthread_mutex_destroy(&service->audio_queue_mutex);
    
    free(service);
}

int audio_service_initialize(AudioService* service, audio_codec_t* codec) {
    if (!service || !codec) {
        return -1;
    }
    
    service->codec = codec;
    
    // 启动主编解码器
    if (codec->vtable && codec->vtable->reset) {
        codec_error_t result = codec->vtable->reset(codec);
        if (result != CODEC_SUCCESS) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to start main codec");
            return -1;
        }
    }
    
    // 初始化Opus编解码器
    if (!service->opus_decoder) {
        service->opus_decoder = opus_codec_create();
        if (!service->opus_decoder) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create Opus decoder");
            return -1;
        }
        
        // 初始化解码器 (输出采样率，单声道)
        audio_format_t decoder_format;
        audio_format_init(&decoder_format, 
                         service->config.output_format.sample_rate > 0 ? service->config.output_format.sample_rate : 16000,
                         1, 16, OPUS_FRAME_DURATION_MS);
        
        codec_error_t result = audio_codec_init_decoder(service->opus_decoder, &decoder_format);
        if (result != CODEC_SUCCESS) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to initialize Opus decoder");
            return -1;
        }
    }
    
    if (!service->opus_encoder) {
        service->opus_encoder = opus_codec_create();
        if (!service->opus_encoder) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create Opus encoder");
            return -1;
        }
        
        // 初始化编码器 (16kHz，单声道)
        audio_format_t encoder_format;
        audio_format_init(&encoder_format, 16000, 1, 16, OPUS_FRAME_DURATION_MS);
        
        codec_error_t result = audio_codec_init_encoder(service->opus_encoder, &encoder_format);
        if (result != CODEC_SUCCESS) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to initialize Opus encoder");
            return -1;
        }
        
        // 设置编码器复杂度为0（低复杂度，适合实时应用）
        opus_codec_set_complexity(service->opus_encoder, 0);
    }
    
    // 检查是否需要重采样器配置
    // 注意：C版本中我们暂时跳过重采样器的实现，因为需要额外的重采样库
    int input_sample_rate = service->config.input_format.sample_rate > 0 ? 
                           service->config.input_format.sample_rate : 16000;
    if (input_sample_rate != 16000) {
        LINX_LOGW(AUDIO_SERVICE_TAG, "Input sample rate %d != 16000, resampling may be needed", input_sample_rate);
        // TODO: 配置重采样器
        // input_resampler_.Configure(input_sample_rate, 16000);
        // reference_resampler_.Configure(input_sample_rate, 16000);
    }
    
    // 初始化音频处理器
    if (service->audio_processor && service->audio_processor->vtable) {
        audio_processor_config_t processor_config;
        audio_processor_config_init_default(&processor_config, 16000, 1, OPUS_FRAME_DURATION_MS);
        
        // 根据配置启用功能
        processor_config.enable_vad = service->config.features.voice_activity_detection;
        processor_config.enable_aec = service->config.features.device_aec;
        processor_config.enable_ns = service->config.features.noise_suppression;
        
        audio_processor_error_t result = service->audio_processor->vtable->initialize(
            service->audio_processor, &processor_config, service->codec);
        if (result != AUDIO_PROCESSOR_SUCCESS) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to initialize audio processor");
            return -1;
        }
        
        // 设置音频处理器输出回调
        service->audio_processor->vtable->set_output_callback(
            service->audio_processor, 
            audio_processor_output_callback, 
            service);
        
        // 设置VAD状态变化回调
        service->audio_processor->vtable->set_vad_callback(
            service->audio_processor,
            audio_processor_vad_callback,
            service);
        
        LINX_LOGI(AUDIO_SERVICE_TAG, "Audio processor initialized successfully");
    } else {
        LINX_LOGW(AUDIO_SERVICE_TAG, "No audio processor available");
    }
    
    // 创建音频功率检查定时器
    // 注意：C版本中我们暂时跳过定时器的实现，因为需要平台特定的定时器API
    // TODO: 实现音频功率检查定时器
    /*
    esp_timer_create_args_t audio_power_timer_args = {
        .callback = [](void* arg) {
            AudioService* audio_service = (AudioService*)arg;
            audio_service->CheckAndUpdateAudioPowerState();
        },
        .arg = service,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "audio_power_timer",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&audio_power_timer_args, &service->audio_power_timer);
    */
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service initialized successfully");
    return 0;
}

int audio_service_start(AudioService* service) {
    if (!service) {
        return -1;
    }
    
    service->service_stopped = false;
    clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING | AS_EVENT_WAKE_WORD_RUNNING | AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    
    // 配置和启动音频接口
    if (service->audio_interface && service->audio_interface->vtable) {
        // 首先初始化音频接口 (这是关键步骤！)
        if (service->audio_interface->vtable->init) {
            int init_result = service->audio_interface->vtable->init(service->audio_interface);
            if (init_result == 0) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "Audio interface initialized successfully");
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to initialize audio interface: %d", init_result);
                return -1;
            }
        }
        
        // 设置音频配置 (参考 audio_test_portaudio.c 的配置)
        if (service->audio_interface->vtable->set_config) {
            service->audio_interface->vtable->set_config(service->audio_interface, 
                                                       16000,  // sample_rate
                                                       1024,   // frame_size  
                                                       1,      // channels (mono)
                                                       4,      // periods
                                                       8192,   // buffer_size
                                                       2048);  // period_size
            LINX_LOGI(AUDIO_SERVICE_TAG, "Audio interface configured: 16kHz, 1024 frame, mono");
        }
        
        // 启动录制
        if (service->audio_interface->vtable->record) {
            int record_result = service->audio_interface->vtable->record(service->audio_interface);
            if (record_result == 0) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "Audio recording started successfully");
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to start audio recording: %d", record_result);
            }
        }
        
        // 启动播放
        if (service->audio_interface->vtable->init_play) {
            int play_result = service->audio_interface->vtable->init_play(service->audio_interface);
            if (play_result == 0) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "Audio playback started successfully");
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to start audio playback: %d", play_result);
            }
        }
    } else {
        LINX_LOGW(AUDIO_SERVICE_TAG, "No audio interface available, audio functionality will be limited");
    }
    
    // 创建线程
    if (pthread_create(&service->audio_input_thread, NULL, audio_input_thread_func, service) != 0) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create audio input thread");
        return -1;
    }
    
    if (pthread_create(&service->audio_output_thread, NULL, audio_output_thread_func, service) != 0) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create audio output thread");
        return -1;
    }
    
    if (pthread_create(&service->opus_codec_thread, NULL, opus_codec_thread_func, service) != 0) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create opus codec thread");
        return -1;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service started");
    return 0;
}

void audio_service_stop(AudioService* service) {
    if (!service) {
        return;
    }
    
    service->service_stopped = true;
    set_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING | AS_EVENT_WAKE_WORD_RUNNING | AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    
    // 等待线程结束
    pthread_join(service->audio_input_thread, NULL);
    pthread_join(service->audio_output_thread, NULL);
    pthread_join(service->opus_codec_thread, NULL);
    
    // 清空队列
    pthread_mutex_lock(&service->audio_queue_mutex);
    audio_packet_queue_destroy(&service->audio_decode_queue);
    audio_packet_queue_destroy(&service->audio_send_queue);
    audio_packet_queue_destroy(&service->audio_testing_queue);
    audio_task_queue_destroy(&service->audio_encode_queue);
    audio_task_queue_destroy(&service->audio_playback_queue);
    
    // 重新初始化队列
    audio_packet_queue_init(&service->audio_decode_queue, MAX_DECODE_PACKETS_IN_QUEUE);
    audio_packet_queue_init(&service->audio_send_queue, MAX_SEND_PACKETS_IN_QUEUE);
    audio_packet_queue_init(&service->audio_testing_queue, AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS);
    audio_task_queue_init(&service->audio_encode_queue, MAX_ENCODE_TASKS_IN_QUEUE, true);
    audio_task_queue_init(&service->audio_playback_queue, MAX_PLAYBACK_TASKS_IN_QUEUE, true);
    
    pthread_cond_broadcast(&service->audio_queue_cv);
    pthread_mutex_unlock(&service->audio_queue_mutex);
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service stopped");
}

int audio_service_read_audio_data(AudioService* service, vector_int16_t_t *data, int sample_rate, int samples) {
    vector_int16_t_t mic_channel;
    vector_int16_t_t reference_channel;
    vector_int16_t_t resampled_mic;
    vector_int16_t_t resampled_reference;
    vector_int16_t_t resampled;
    size_t i, j;
    int result = 0;
    
    if (!service || !data || samples <= 0) {
        return -1;
    }
    
    // 初始化临时vector
    vector_int16_t_init(&mic_channel);
    vector_int16_t_init(&reference_channel);
    vector_int16_t_init(&resampled_mic);
    vector_int16_t_init(&resampled_reference);
    vector_int16_t_init(&resampled);
    
    // 检查并启用音频输入（类似C++版本的!codec_->input_enabled()检查）
    if (!service->audio_interface || !service->audio_interface->vtable) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Audio interface not available");
        result = -1;
        goto cleanup;
    }
    
    // 启用音频输入（类似C++版本的codec_->EnableInput(true)）
    if (service->audio_interface->vtable->record) {
        service->audio_interface->vtable->record(service->audio_interface);
    }
    
    // 获取输入采样率和声道数
    int input_sample_rate = 16000; // 假设输入采样率为16kHz，实际应该从接口获取
    int input_channels = service->audio_interface->channels;
    
    if (input_sample_rate != sample_rate) {
        // 需要重采样（类似C++版本的codec_->input_sample_rate() != sample_rate）
        int required_input_samples = samples * input_sample_rate / sample_rate * input_channels;
        
        // 调整data大小并读取数据（类似C++版本的data.resize()）
        vector_int16_t_resize(data, required_input_samples);
        
        // 读取音频数据（类似C++版本的codec_->InputData(data)）
        if (service->audio_interface->vtable->read) {
            int read_result = service->audio_interface->vtable->read(
                service->audio_interface, 
                vector_int16_t_data(data), 
                required_input_samples * sizeof(int16_t)
            );
            if (read_result < 0) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to read audio data");
                result = -1;
                goto cleanup;
            }
        }
        
        if (input_channels == 2) {
            // 双声道处理：分离麦克风和参考声道（类似C++版本的codec_->input_channels() == 2）
            size_t channel_samples = vector_int16_t_size(data) / 2;
            
            // 创建mic_channel和reference_channel（类似C++版本的auto mic_channel = std::vector<int16_t>(data.size() / 2)）
            vector_int16_t_resize(&mic_channel, channel_samples);
            vector_int16_t_resize(&reference_channel, channel_samples);
            
            // 分离声道（类似C++版本的for循环分离）
            int16_t* data_ptr = vector_int16_t_data(data);
            int16_t* mic_ptr = vector_int16_t_data(&mic_channel);
            int16_t* ref_ptr = vector_int16_t_data(&reference_channel);
            
            for (i = 0, j = 0; i < channel_samples; i++, j += 2) {
                mic_ptr[i] = data_ptr[j];
                ref_ptr[i] = data_ptr[j + 1];
            }
            
            // 重采样麦克风声道（类似C++版本的input_resampler_.GetOutputSamples()）
            size_t resampled_mic_size = channel_samples * sample_rate / input_sample_rate;
            vector_int16_t_resize(&resampled_mic, resampled_mic_size);
            
            // TODO: 这里应该调用实际的重采样器（类似C++版本的input_resampler_.Process()）
            // 暂时使用简单的线性插值
            int16_t* resampled_mic_ptr = vector_int16_t_data(&resampled_mic);
            for (i = 0; i < resampled_mic_size; i++) {
                size_t src_idx = i * channel_samples / resampled_mic_size;
                if (src_idx < channel_samples) {
                    resampled_mic_ptr[i] = mic_ptr[src_idx];
                }
            }
            
            // 重采样参考声道（类似C++版本的reference_resampler_.Process()）
            size_t resampled_ref_size = channel_samples * sample_rate / input_sample_rate;
            vector_int16_t_resize(&resampled_reference, resampled_ref_size);
            
            int16_t* resampled_ref_ptr = vector_int16_t_data(&resampled_reference);
            for (i = 0; i < resampled_ref_size; i++) {
                size_t src_idx = i * channel_samples / resampled_ref_size;
                if (src_idx < channel_samples) {
                    resampled_ref_ptr[i] = ref_ptr[src_idx];
                }
            }
            
            // 合并重采样后的数据（类似C++版本的data.resize()和交错存储）
            size_t total_output_samples = resampled_mic_size + resampled_ref_size;
            vector_int16_t_resize(data, total_output_samples);
            
            int16_t* output_ptr = vector_int16_t_data(data);
            // 交错存储双声道数据（类似C++版本的for循环合并）
            for (i = 0, j = 0; i < resampled_mic_size && j < total_output_samples - 1; i++, j += 2) {
                output_ptr[j] = resampled_mic_ptr[i];
                output_ptr[j + 1] = resampled_ref_ptr[i];
            }
            
        } else {
            // 单声道处理（类似C++版本的else分支）
            size_t resampled_size = vector_int16_t_size(data) * sample_rate / input_sample_rate;
            vector_int16_t_resize(&resampled, resampled_size);
            
            // 简单的重采样（实际应该使用专业的重采样器，类似C++版本的input_resampler_.Process()）
            int16_t* data_ptr = vector_int16_t_data(data);
            int16_t* resampled_ptr = vector_int16_t_data(&resampled);
            
            for (i = 0; i < resampled_size; i++) {
                size_t src_idx = i * vector_int16_t_size(data) / resampled_size;
                if (src_idx < vector_int16_t_size(data)) {
                    resampled_ptr[i] = data_ptr[src_idx];
                }
            }
            
            // 移动数据到输出vector（类似C++版本的data = std::move(resampled)）
            vector_int16_t_copy(data, &resampled);
        }
        
    } else {
        // 不需要重采样，直接读取（类似C++版本的else分支）
        int required_samples = samples * input_channels;
        vector_int16_t_resize(data, required_samples);
        
        if (service->audio_interface->vtable->read) {
            int read_result = service->audio_interface->vtable->read(
                service->audio_interface, 
                vector_int16_t_data(data), 
                required_samples * sizeof(int16_t)
            );
            if (read_result < 0) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to read audio data");
                result = -1;
                goto cleanup;
            }
        }
    }
    
    // 更新最后输入时间（类似C++版本的last_input_time_ = std::chrono::steady_clock::now()）
    get_current_time(&service->last_input_time);
    service->debug_statistics.input_count++;
    
    LINX_LOGD(AUDIO_SERVICE_TAG, "Audio data read successfully: %d samples at %d Hz", samples, sample_rate);
    
cleanup:
    // 清理临时vector资源
    vector_int16_t_destroy(&mic_channel);
    vector_int16_t_destroy(&reference_channel);
    vector_int16_t_destroy(&resampled_mic);
    vector_int16_t_destroy(&resampled_reference);
    vector_int16_t_destroy(&resampled);
    
    return result;
}

void audio_service_set_callbacks(AudioService* service, const AudioServiceCallbacks* callbacks) {
    if (!service || !callbacks) {
        return;
    }
    
    service->callbacks = *callbacks;
}

void audio_service_set_components(AudioService* service,
                                 AudioInterface* audio_interface,
                                 AudioProcessor* audio_processor,
                                 WakeWordInterface* wake_word_interface,
                                 audio_codec_t* opus_encoder,
                                 audio_codec_t* opus_decoder) {
    if (!service) {
        return;
    }
    
    service->audio_interface = audio_interface;
    service->audio_processor = audio_processor;
    service->wake_word = wake_word_interface;
    service->opus_encoder = opus_encoder;
    service->opus_decoder = opus_decoder;
    
    // 设置音频处理器回调函数
    if (service->audio_processor && service->audio_processor->vtable) {
        if (service->audio_processor->vtable->set_output_callback) {
            audio_processor_error_t result = service->audio_processor->vtable->set_output_callback(
                service->audio_processor, audio_processor_output_callback, service);
            if (result == AUDIO_PROCESSOR_SUCCESS) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "Audio processor output callback set successfully");
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to set audio processor output callback: %d", result);
            }
        }
        
        if (service->audio_processor->vtable->set_vad_callback) {
            audio_processor_error_t result = service->audio_processor->vtable->set_vad_callback(
                service->audio_processor, audio_processor_vad_callback, service);
            if (result == AUDIO_PROCESSOR_SUCCESS) {
                LINX_LOGI(AUDIO_SERVICE_TAG, "Audio processor VAD callback set successfully");
            } else {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to set audio processor VAD callback: %d", result);
            }
        }
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service components set");
}

void audio_service_config_init_default(AudioServiceConfig* config) {
    if (!config) {
        return;
    }
    
    // 初始化默认音频格式
    audio_format_default(&config->input_format);
    audio_format_default(&config->output_format);
    
    // 初始化默认功能配置 - 像C++版本一样默认启用主要功能
    config->features.wake_word_detection = false;        // 唤醒词检测需要外部模型，默认关闭
    config->features.voice_processing = true;           // 默认启用语音处理
    config->features.audio_testing = false;             // 音频测试默认关闭
    config->features.device_aec = true;                  // 默认启用设备AEC（回声消除）
    config->features.noise_suppression = true;          // 默认启用噪声抑制
    config->features.voice_activity_detection = true;   // 默认启用语音活动检测
    
    // 初始化模型列表
    config->models_list = NULL;
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service config initialized with default values (AEC, NS, VAD enabled)");
}




// ============================================================================
// 组件管理和状态检查
// ============================================================================

static bool is_component_available(AudioService* service, const char* component_name) {
    if (!service) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Service is NULL");
        return false;
    }
    
    if (strcmp(component_name, COMPONENT_WAKE_WORD) == 0) {
        if (!service->wake_word) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Wake word component not set");
            return false;
        }
    } else if (strcmp(component_name, COMPONENT_AUDIO_PROCESSOR) == 0) {
        if (!service->audio_processor) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Audio processor component not set");
            return false;
        }
    } else if (strcmp(component_name, COMPONENT_AUDIO_INTERFACE) == 0) {
        if (!service->audio_interface) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Audio interface component not set");
            return false;
        }
    } else if (strcmp(component_name, COMPONENT_OPUS_ENCODER) == 0) {
        if (!service->opus_encoder) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Opus encoder component not set");
            return false;
        }
    } else if (strcmp(component_name, COMPONENT_OPUS_DECODER) == 0) {
        if (!service->opus_decoder) {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Opus decoder component not set");
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// 功能配置接口实现
// ============================================================================

int audio_service_configure_features(AudioService* service, const AudioServiceFeatures* features) {
    if (!service || !features) {
        return -1;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Configuring audio service features");
    
    // 检查并配置唤醒词检测
    if (features->wake_word_detection != service->current_features.wake_word_detection) {
        if (features->wake_word_detection) {
            if (!is_component_available(service, COMPONENT_WAKE_WORD)) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Cannot enable wake word detection: component not available or not initialized");
                return -1;
            }
            // 组件应该已经在外部初始化好，这里只启动
            wake_word_interface_start(service->wake_word);
            set_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
        } else {
            if (service->wake_word) {
                wake_word_interface_stop(service->wake_word);
            }
            clear_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
        }
        service->current_features.wake_word_detection = features->wake_word_detection;
    }
    
    // 检查并配置语音处理
    if (features->voice_processing != service->current_features.voice_processing) {
        if (features->voice_processing) {
            if (!is_component_available(service, COMPONENT_AUDIO_PROCESSOR) || 
                !is_component_available(service, COMPONENT_OPUS_ENCODER) || 
                !is_component_available(service, COMPONENT_OPUS_DECODER)) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Cannot enable voice processing: required components not available");
                return -1;
            }
            // 组件应该已经在外部初始化好，这里只启动
            service->audio_input_need_warmup = true;
            if (audio_processor_start(service->audio_processor) != AUDIO_PROCESSOR_SUCCESS) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to start audio processor");
                return -1;
            }
            set_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
        } else {
            if (service->audio_processor) {
                audio_processor_stop(service->audio_processor);
            }
            clear_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
        }
        service->current_features.voice_processing = features->voice_processing;
    }
    
    // 检查并配置音频测试
    if (features->audio_testing != service->current_features.audio_testing) {
        if (features->audio_testing) {
            if (!is_component_available(service, COMPONENT_AUDIO_INTERFACE)) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Cannot enable audio testing: audio interface not available");
                return -1;
            }
            set_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
        } else {
            clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
            
            // 将测试队列数据移到解码队列
            pthread_mutex_lock(&service->audio_queue_mutex);
            while (!audio_packet_queue_is_empty(&service->audio_testing_queue)) {
                AudioStreamPacket* packet = audio_packet_queue_pop(&service->audio_testing_queue);
                if (packet) {
                    if (!audio_packet_queue_push(&service->audio_decode_queue, packet)) {
                        audio_stream_packet_destroy(packet);
                    }
                }
            }
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
        }
        service->current_features.audio_testing = features->audio_testing;
    }
    
    // 检查并配置设备AEC
    if (features->device_aec != service->current_features.device_aec) {
        if (features->device_aec) {
            if (!is_component_available(service, COMPONENT_AUDIO_PROCESSOR)) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Cannot enable device AEC: audio processor not available or not initialized");
                return -1;
            }
            // 组件应该已经在外部初始化好，这里只配置AEC
            audio_processor_enable_device_aec(service->audio_processor, true);
        } else {
            if (service->audio_processor) {
                audio_processor_enable_device_aec(service->audio_processor, false);
            }
        }
        service->current_features.device_aec = features->device_aec;
    }
    
    // 其他功能的配置可以在这里添加
    service->current_features.noise_suppression = features->noise_suppression;
    service->current_features.voice_activity_detection = features->voice_activity_detection;
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service features configured successfully");
    return 0;
}

int audio_service_get_features(const AudioService* service, AudioServiceFeatures* features) {
    if (!service || !features) {
        return -1;
    }
    
    *features = service->current_features;
    return 0;
}



// ============================================================================
// 数据处理接口实现
// ============================================================================

bool audio_service_push_task_to_encode_queue(AudioService* service, AudioTaskType type, int16_t* pcm_data, size_t data_size) {
    if (!service || !pcm_data || data_size == 0) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Push task to encode queue failed: invalid parameters");
        return false;
    }
    
    // 创建音频数据副本
    int16_t* data_copy = (int16_t*)malloc(data_size);
    if (!data_copy) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "Push task to encode queue failed: memory allocation failed");
        return false;
    }
    memcpy(data_copy, pcm_data, data_size);
    
    // 创建音频任务
    AudioTask* task = audio_task_create(type, data_copy, data_size);
    if (!task) {
        free(data_copy);
        LINX_LOGE(AUDIO_SERVICE_TAG, "Push task to encode queue failed: task creation failed");
        return false;
    }
    
    pthread_mutex_lock(&service->audio_queue_mutex);
    
    // 如果任务类型是发送到发送队列，需要设置时间戳
    if (type == AUDIO_TASK_PROCESS_AUDIO && !timestamp_queue_is_empty(&service->timestamp_queue)) {
        if (timestamp_queue_size(&service->timestamp_queue) <= MAX_TIMESTAMPS_IN_QUEUE) {
            uint32_t timestamp = 0;
            if (timestamp_queue_pop(&service->timestamp_queue, &timestamp)) {
                task->timestamp = timestamp;
            }
        } else {
            LINX_LOGW(AUDIO_SERVICE_TAG, "Timestamp queue (%zu) is full, dropping timestamp", 
                     timestamp_queue_size(&service->timestamp_queue));
            uint32_t timestamp = 0;
            timestamp_queue_pop(&service->timestamp_queue, &timestamp);
            task->timestamp = timestamp;
        }
    }
    
    // 等待编码队列有空间
    while (audio_task_queue_size(&service->audio_encode_queue) >= MAX_ENCODE_TASKS_IN_QUEUE && !service->service_stopped) {
        pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
    }
    
    bool result = false;
    if (!service->service_stopped) {
        result = audio_task_queue_push(&service->audio_encode_queue, task);
        if (result) {
            pthread_cond_broadcast(&service->audio_queue_cv);
            LINX_LOGD(AUDIO_SERVICE_TAG, "Task pushed to encode queue successfully, type: %s, queue size: %zu", 
                     audio_task_type_to_string(type), audio_task_queue_size(&service->audio_encode_queue));
        } else {
            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to push task to encode queue");
            audio_task_destroy(task);
        }
    } else {
        LINX_LOGW(AUDIO_SERVICE_TAG, "Service stopped, discarding task");
        audio_task_destroy(task);
    }
    
    pthread_mutex_unlock(&service->audio_queue_mutex);
    return result;
}

bool audio_service_push_packet_to_decode_queue(AudioService* service, AudioStreamPacket* packet, bool wait) {
    if (!service || !packet) {
        return false;
    }
    
    pthread_mutex_lock(&service->audio_queue_mutex);
    
    if (wait) {
        while (audio_packet_queue_size(&service->audio_decode_queue) >= MAX_DECODE_PACKETS_IN_QUEUE && !service->service_stopped) {
            pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
        }
    } else if (audio_packet_queue_size(&service->audio_decode_queue) >= MAX_DECODE_PACKETS_IN_QUEUE) {
        pthread_mutex_unlock(&service->audio_queue_mutex);
        return false;
    }
    
    bool result = false;
    if (!service->service_stopped) {
        result = audio_packet_queue_push(&service->audio_decode_queue, packet);
        if (result) {
            pthread_cond_broadcast(&service->audio_queue_cv);
        }
    }
    
    pthread_mutex_unlock(&service->audio_queue_mutex);
    return result;
}

AudioStreamPacket* audio_service_pop_packet_from_send_queue(AudioService* service) {
    if (!service) {
        return NULL;
    }
    
    pthread_mutex_lock(&service->audio_queue_mutex);
    AudioStreamPacket* packet = audio_packet_queue_pop(&service->audio_send_queue);
    if (packet) {
        pthread_cond_broadcast(&service->audio_queue_cv);
    }
    pthread_mutex_unlock(&service->audio_queue_mutex);
    
    return packet;
}



void audio_service_play_sound(AudioService* service, const uint8_t* sound_data, size_t sound_size) {
    if (!service || !sound_data || sound_size == 0) {
        return;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Playing audio of size %zu bytes", sound_size);
    
    // 创建播放任务
    uint8_t* data_copy = (uint8_t*)malloc(sound_size);
    if (data_copy) {
        memcpy(data_copy, sound_data, sound_size);
        AudioTask* task = audio_task_create(AUDIO_TASK_PLAY_SOUND, data_copy, sound_size);
        if (task) {
            pthread_mutex_lock(&service->audio_queue_mutex);
            if (!audio_task_queue_push(&service->audio_playback_queue, task)) {
                audio_task_destroy(task);
            } else {
                pthread_cond_broadcast(&service->audio_queue_cv);
            }
            pthread_mutex_unlock(&service->audio_queue_mutex);
        } else {
            free(data_copy);
        }
    }
}



// ============================================================================
// 状态查询接口实现
// ============================================================================



bool audio_service_is_voice_detected(const AudioService* service) {
    return service ? service->voice_detected : false;
}

bool audio_service_is_idle(const AudioService* service) {
    if (!service) {
        return true;
    }
    
    struct timespec current_time;
    get_current_time(&current_time);
    
    long input_diff = time_diff_ms(&service->last_input_time, &current_time);
    long output_diff = time_diff_ms(&service->last_output_time, &current_time);
    
    return (input_diff > AUDIO_POWER_TIMEOUT_MS && output_diff > AUDIO_POWER_TIMEOUT_MS);
}



bool audio_service_is_component_ready(const AudioService* service, const char* component_type) {
    if (!service || !component_type) {
        return false;
    }
    
    if (strcmp(component_type, COMPONENT_AUDIO_INTERFACE) == 0) {
        return service->audio_interface != NULL && service->audio_interface->vtable != NULL;
    } else if (strcmp(component_type, COMPONENT_AUDIO_PROCESSOR) == 0) {
        return service->audio_processor != NULL;
    } else if (strcmp(component_type, COMPONENT_WAKE_WORD) == 0) {
        return service->wake_word != NULL && service->wake_word->is_initialized;
    } else if (strcmp(component_type, COMPONENT_OPUS_ENCODER) == 0) {
        return service->opus_encoder != NULL;
    } else if (strcmp(component_type, COMPONENT_OPUS_DECODER) == 0) {
        return service->opus_decoder != NULL;
    }
    
    return false;
}