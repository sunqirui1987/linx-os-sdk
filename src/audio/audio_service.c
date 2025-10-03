#include "audio_service.h"
#include "audio_packet_queue.h"
#include "audio_task_queue.h"
#include "timestamp_queue.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

// 音频服务日志标签
#define AUDIO_SERVICE_TAG "AudioService"

// Helper function to get current time
static void get_current_time(struct timespec* ts) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

// Helper function to calculate time difference in milliseconds
static long time_diff_ms(const struct timespec* start, const struct timespec* end) {
    long diff_sec = end->tv_sec - start->tv_sec;
    long diff_nsec = end->tv_nsec - start->tv_nsec;
    return diff_sec * 1000 + diff_nsec / 1000000;
}

// Event management functions
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

// 队列函数实现已移至独立文件：
// - audio_packet_queue.c
// - audio_task_queue.c  
// - timestamp_queue.c

// AudioStreamPacket和AudioTask函数实现已移至独立文件：
// - audio_packet_queue.c中的audio_stream_packet_*函数
// - audio_task_queue.c中的audio_task_*函数

// 音频处理回调函数
/**
 * @brief 音频处理器输出回调函数
 * @param data 音频数据
 * @param size 数据大小
 * @param user_data 用户数据（AudioService指针）
 */
static void audio_processor_output_callback(const int16_t* data, size_t size, void* user_data) {
    AudioService* service = (AudioService*)user_data;
    if (!service || !data || size == 0) {
        return;
    }
    
    // 创建音频数据副本
    int16_t* audio_data = (int16_t*)malloc(size * sizeof(int16_t));
    if (!audio_data) {
        LINX_LOGE(AUDIO_SERVICE_TAG, "创建音频数据副本失败");
        return;
    }
    memcpy(audio_data, data, size * sizeof(int16_t));
    
    // 创建新的音频任务
    AudioTask* task = audio_task_create(AUDIO_TASK_PROCESS_AUDIO, audio_data, size * sizeof(int16_t));
    if (!task) {
        free(audio_data);
        LINX_LOGE(AUDIO_SERVICE_TAG, "创建音频任务失败");
        return;
    }
    
    // 推送到编码队列
    pthread_mutex_lock(&service->audio_queue_mutex);
    while (audio_task_queue_is_full(&service->audio_encode_queue) && !service->service_stopped) {
        pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
    }
    
    if (!service->service_stopped) {
        if (!audio_task_queue_push(&service->audio_encode_queue, task)) {
            LINX_LOGE(AUDIO_SERVICE_TAG, "推送任务到编码队列失败");
            audio_task_destroy(task);
        } else {
            pthread_cond_broadcast(&service->audio_queue_cv);
        }
    } else {
        audio_task_destroy(task);
    }
    pthread_mutex_unlock(&service->audio_queue_mutex);
}

/**
 * @brief 语音活动检测回调函数
 * @param speaking 是否检测到语音
 * @param user_data 用户数据（AudioService指针）
 */
static void audio_processor_vad_callback(bool speaking, void* user_data) {
    AudioService* service = (AudioService*)user_data;
    if (!service) {
        return;
    }
    
    service->voice_detected = speaking;
    if (service->callbacks.on_vad_change) {
        service->callbacks.on_vad_change(speaking, service->callbacks.user_data);
    }
}

// Thread functions
static void* audio_input_thread_func(void* arg) {
    AudioService* service = (AudioService*)arg;
    
    while (!service->service_stopped) {
        pthread_mutex_lock(&service->event_mutex);
        while (!(service->event_bits & (AS_EVENT_AUDIO_TESTING_RUNNING | 
                                       AS_EVENT_WAKE_WORD_RUNNING | 
                                       AS_EVENT_AUDIO_PROCESSOR_RUNNING)) && 
               !service->service_stopped) {
            pthread_cond_wait(&service->audio_queue_cv, &service->event_mutex);
        }
        uint32_t bits = service->event_bits;
        pthread_mutex_unlock(&service->event_mutex);
        
        if (service->service_stopped) {
            break;
        }
        
        if (service->audio_input_need_warmup) {
            service->audio_input_need_warmup = false;
            usleep(120000); // 120ms warmup
            continue;
        }
        
        // Audio testing mode
        if (bits & AS_EVENT_AUDIO_TESTING_RUNNING) {
            if (audio_packet_queue_size(&service->audio_testing_queue) >= 
                AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Audio testing queue is full, stopping audio testing");
                audio_service_enable_audio_testing(service, false);
                continue;
            }
            
            int samples = OPUS_FRAME_DURATION_MS * 16000 / 1000;
            int16_t* data = (int16_t*)malloc(samples * sizeof(int16_t));
            if (data && audio_service_read_audio_data(service, data, 16000, samples)) {
                // 创建音频测试任务
                AudioTask* task = audio_task_create(AUDIO_TASK_PROCESS_AUDIO, data, samples * sizeof(int16_t));
                if (task) {
                    pthread_mutex_lock(&service->audio_queue_mutex);
                    if (!audio_task_queue_push(&service->audio_encode_queue, task)) {
                        audio_task_destroy(task);
                    } else {
                        pthread_cond_broadcast(&service->audio_queue_cv);
                    }
                    pthread_mutex_unlock(&service->audio_queue_mutex);
                } else {
                    free(data);
                }
            }
            if (data) {
                free(data);
            }
            continue;
        }
        
        // Wake word detection
        if (bits & AS_EVENT_WAKE_WORD_RUNNING) {
            if (service->wake_word) {
                size_t feed_size = wake_word_interface_get_feed_size(service->wake_word);
                if (feed_size > 0) {
                    int16_t* data = (int16_t*)malloc(feed_size * sizeof(int16_t));
                    if (data && audio_service_read_audio_data(service, data, 16000, feed_size)) {
                        wake_word_interface_feed(service->wake_word, data, feed_size);
                    }
                    if (data) {
                        free(data);
                    }
                    continue;
                }
            }
        }
        
        // 音频处理器处理
        if (bits & AS_EVENT_AUDIO_PROCESSOR_RUNNING) {
            if (service->audio_processor) {
                size_t feed_size = audio_processor_get_feed_size(service->audio_processor);
                if (feed_size > 0) {
                    int16_t* data = (int16_t*)malloc(feed_size * sizeof(int16_t));
                    if (data && audio_service_read_audio_data(service, data, 16000, feed_size)) {
                        audio_processor_feed(service->audio_processor, data, feed_size);
                    }
                    if (data) {
                        free(data);
                    }
                    continue;
                }
            }
        }
        
        // Small delay to prevent busy waiting
        usleep(1000);
    }
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "Audio input thread stopped");
    return NULL;
}

static void* audio_output_thread_func(void* arg) {
    AudioService* service = (AudioService*)arg;
    
    while (!service->service_stopped) {
        pthread_mutex_lock(&service->audio_queue_mutex);
        while (audio_task_queue_is_empty(&service->audio_playback_queue) && !service->service_stopped) {
            pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
        }
        
        if (service->service_stopped) {
            pthread_mutex_unlock(&service->audio_queue_mutex);
            break;
        }
        
        AudioTask* task = audio_task_queue_pop(&service->audio_playback_queue);
        pthread_cond_broadcast(&service->audio_queue_cv);
        pthread_mutex_unlock(&service->audio_queue_mutex);
        
        if (task) {
            // 通过编解码器输出音频数据
            if (service->codec && task->data && task->data_size > 0) {
                // 简化的音频输出 - 在实际实现中，这将使用音频接口
                // audio_interface_write(service->codec, task->data, task->data_size);
            }
            
            // Update statistics and timing
            get_current_time(&service->last_output_time);
            service->debug_statistics.playback_count++;
            
            audio_task_destroy(task);
        }
    }
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "Audio output thread stopped");
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
        
        // Decode audio packets
        if (!audio_packet_queue_is_empty(&service->audio_decode_queue) && 
            audio_task_queue_size(&service->audio_playback_queue) < MAX_PLAYBACK_TASKS_IN_QUEUE) {
            
            AudioStreamPacket* packet = audio_packet_queue_pop(&service->audio_decode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (packet) {
                // 估算解码后的数据大小
                size_t estimated_samples = (packet->sample_rate * packet->frame_duration) / 1000;
                int16_t* decoded_data = (int16_t*)malloc(estimated_samples * sizeof(int16_t));
                
                if (decoded_data && service->opus_decoder && packet->payload && packet->payload_size > 0) {
                    size_t decoded_size = 0;
                    audio_format_t format;
                    format.sample_rate = packet->sample_rate;
                    format.channels = 1;
                    format.bits_per_sample = 16;
                    format.frame_size_ms = packet->frame_duration;
                    
                    codec_error_t result = audio_codec_decode(service->opus_decoder, 
                                                            packet->payload, packet->payload_size,
                                                            decoded_data, estimated_samples, &decoded_size);
                    if (result == CODEC_SUCCESS) {
                        // 创建播放任务
                        AudioTask* task = audio_task_create(AUDIO_TASK_PLAY_SOUND, decoded_data, decoded_size * sizeof(int16_t));
                        if (task) {
                            task->timestamp = packet->timestamp;
                                
                            pthread_mutex_lock(&service->audio_queue_mutex);
                            if (!audio_task_queue_push(&service->audio_playback_queue, task)) {
                                audio_task_destroy(task);
                            } else {
                                pthread_cond_broadcast(&service->audio_queue_cv);
                            }
                            pthread_mutex_unlock(&service->audio_queue_mutex);
                            
                            service->debug_statistics.decode_count++;
                        } else {
                            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to create audio task");
                        }
                    } else {
                        LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to decode audio");
                        free(decoded_data);
                    }
                }
                audio_stream_packet_destroy(packet);
            }
            pthread_mutex_lock(&service->audio_queue_mutex);
        }
        
        // Encode audio tasks
        if (!audio_task_queue_is_empty(&service->audio_encode_queue) && 
            audio_packet_queue_size(&service->audio_send_queue) < MAX_SEND_PACKETS_IN_QUEUE) {
            
            AudioTask* task = audio_task_queue_pop(&service->audio_encode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (task) {
                AudioStreamPacket* packet = audio_stream_packet_create();
                if (packet) {
                    packet->frame_duration = OPUS_FRAME_DURATION_MS;
                    packet->sample_rate = 16000;
                    packet->timestamp = task->timestamp;
                    
                    // 使用opus编码器编码音频数据
                    if (service->opus_encoder && task->data && task->data_size > 0) {
                        size_t encoded_size = 0;
                        size_t sample_count = task->data_size / sizeof(int16_t);
                        codec_error_t result = audio_codec_encode(service->opus_encoder,
                                                                (int16_t*)task->data, sample_count,
                                                                packet->payload, packet->payload_capacity, &encoded_size);
                        if (result == CODEC_SUCCESS) {
                            packet->payload_size = encoded_size;
                            
                            // 根据任务类型决定推送到哪个队列
                            if (task->type == AUDIO_TASK_PROCESS_AUDIO) {
                                pthread_mutex_lock(&service->audio_queue_mutex);
                                if (!audio_packet_queue_push(&service->audio_send_queue, packet)) {
                                    audio_stream_packet_destroy(packet);
                                } else {
                                    pthread_cond_broadcast(&service->audio_queue_cv);
                                    if (service->callbacks.on_send_queue_available) {
                                        service->callbacks.on_send_queue_available(service->callbacks.user_data);
                                    }
                                }
                                pthread_mutex_unlock(&service->audio_queue_mutex);
                            }
                            
                            service->debug_statistics.encode_count++;
                        } else {
                            LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to encode audio");
                            audio_stream_packet_destroy(packet);
                        }
                    } else {
                        audio_stream_packet_destroy(packet);
                    }
                }
                audio_task_destroy(task);
            }
            pthread_mutex_lock(&service->audio_queue_mutex);
        }
        
        pthread_mutex_unlock(&service->audio_queue_mutex);
        
        // Small delay to prevent busy waiting
        usleep(1000);
    }
    
    LINX_LOGW(AUDIO_SERVICE_TAG, "Opus codec thread stopped");
    return NULL;
}

// Main AudioService API implementation

AudioService* audio_service_create(void) {
    AudioService* service = (AudioService*)calloc(1, sizeof(AudioService));
    if (!service) {
        return NULL;
    }
    
    // Initialize mutexes and condition variables
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
    
    // Initialize queues
    if (audio_packet_queue_init(&service->audio_decode_queue, MAX_DECODE_PACKETS_IN_QUEUE) != 0 ||
        audio_packet_queue_init(&service->audio_send_queue, MAX_SEND_PACKETS_IN_QUEUE) != 0 ||
        audio_packet_queue_init(&service->audio_testing_queue, AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS) != 0 ||
        audio_task_queue_init(&service->audio_encode_queue, MAX_ENCODE_TASKS_IN_QUEUE, true) != 0 ||
        audio_task_queue_init(&service->audio_playback_queue, MAX_PLAYBACK_TASKS_IN_QUEUE, true) != 0 ||
        timestamp_queue_init(&service->timestamp_queue, MAX_TIMESTAMPS_IN_QUEUE, true) != 0) {
        
        audio_service_destroy(service);
        return NULL;
    }
    
    // Initialize state
    service->service_stopped = true;
    service->wake_word_initialized = false;
    service->audio_processor_initialized = false;
    service->voice_detected = false;
    service->audio_input_need_warmup = false;
    service->event_bits = 0;
    
    // Initialize timing
    get_current_time(&service->last_input_time);
    get_current_time(&service->last_output_time);
    
    // Initialize debug statistics
    memset(&service->debug_statistics, 0, sizeof(DebugStatistics));
    
    return service;
}

int audio_service_initialize(AudioService* service, audio_codec_t* codec) {
    if (!service || !codec) {
        return -1;
    }
    
    service->codec = codec;
    
    // Initialize Opus encoder and decoder (simplified - would need actual Opus codec implementation)
    // service->opus_encoder = create_opus_encoder();
    // service->opus_decoder = create_opus_decoder();
    
    return 0;
}

int audio_service_start(AudioService* service) {
    if (!service) {
        return -1;
    }
    
    service->service_stopped = false;
    clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING | AS_EVENT_WAKE_WORD_RUNNING | AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    
    // Create threads
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
    
    return 0;
}

void audio_service_stop(AudioService* service) {
    if (!service) {
        return;
    }
    
    service->service_stopped = true;
    set_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING | AS_EVENT_WAKE_WORD_RUNNING | AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    
    // Wait for threads to finish
    pthread_join(service->audio_input_thread, NULL);
    pthread_join(service->audio_output_thread, NULL);
    pthread_join(service->opus_codec_thread, NULL);
    
    // Clear queues
    pthread_mutex_lock(&service->audio_queue_mutex);
    audio_packet_queue_destroy(&service->audio_decode_queue);
    audio_packet_queue_destroy(&service->audio_send_queue);
    audio_packet_queue_destroy(&service->audio_testing_queue);
    audio_task_queue_destroy(&service->audio_encode_queue);
    audio_task_queue_destroy(&service->audio_playback_queue);
    
    // Reinitialize queues
    audio_packet_queue_init(&service->audio_decode_queue, MAX_DECODE_PACKETS_IN_QUEUE);
    audio_packet_queue_init(&service->audio_send_queue, MAX_SEND_PACKETS_IN_QUEUE);
    audio_packet_queue_init(&service->audio_testing_queue, AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS);
    audio_task_queue_init(&service->audio_encode_queue, MAX_ENCODE_TASKS_IN_QUEUE, true);
    audio_task_queue_init(&service->audio_playback_queue, MAX_PLAYBACK_TASKS_IN_QUEUE, true);
    
    pthread_cond_broadcast(&service->audio_queue_cv);
    pthread_mutex_unlock(&service->audio_queue_mutex);
}

void audio_service_destroy(AudioService* service) {
    if (!service) {
        return;
    }
    
    // Stop service if running
    if (!service->service_stopped) {
        audio_service_stop(service);
    }
    
    // 销毁音频处理器
    if (service->audio_processor) {
        audio_processor_destroy(service->audio_processor);
        service->audio_processor = NULL;
    }
    
    // Destroy wake word interface
    if (service->wake_word) {
        wake_word_interface_destroy(service->wake_word);
        service->wake_word = NULL;
    }
    
    // Destroy queues
    audio_packet_queue_destroy(&service->audio_decode_queue);
    audio_packet_queue_destroy(&service->audio_send_queue);
    audio_packet_queue_destroy(&service->audio_testing_queue);
    audio_task_queue_destroy(&service->audio_encode_queue);
    audio_task_queue_destroy(&service->audio_playback_queue);
    timestamp_queue_destroy(&service->timestamp_queue);
    
    // Destroy synchronization objects
    pthread_cond_destroy(&service->audio_queue_cv);
    pthread_mutex_destroy(&service->event_mutex);
    pthread_mutex_destroy(&service->audio_queue_mutex);
    
    free(service);
}

void audio_service_set_callbacks(AudioService* service, const AudioServiceCallbacks* callbacks) {
    if (!service || !callbacks) {
        return;
    }
    
    service->callbacks = *callbacks;
}

void audio_service_enable_wake_word_detection(AudioService* service, bool enable) {
    if (!service || !service->wake_word) {
        return;
    }
    
    LINX_LOGD(AUDIO_SERVICE_TAG, "%s wake word detection", enable ? "Enabling" : "Disabling");
    
    if (enable) {
        if (!service->wake_word_initialized) {
            if (wake_word_interface_initialize(service->wake_word, service->codec, service->models_list) != 0) {
                LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to initialize wake word");
                return;
            }
            service->wake_word_initialized = true;
        }
        wake_word_interface_start(service->wake_word);
        set_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
    } else {
        wake_word_interface_stop(service->wake_word);
        clear_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
    }
}

void audio_service_enable_voice_processing(AudioService* service, bool enable) {
    if (!service) {
        return;
    }
    
    LINX_LOGD(AUDIO_SERVICE_TAG, "%s voice processing", enable ? "Enabling" : "Disabling");
    
    if (enable) {
        if (!service->audio_processor_initialized && service->audio_processor) {
            // 需要创建配置结构体来初始化音频处理器
            audio_processor_config_t config;
            audio_processor_config_init_default(&config, 16000, 1, OPUS_FRAME_DURATION_MS);
            config.models_list = service->models_list;
            
            audio_processor_initialize(service->audio_processor, &config, service->codec);
            audio_processor_set_output_callback(service->audio_processor, audio_processor_output_callback, service);
            audio_processor_set_vad_callback(service->audio_processor, audio_processor_vad_callback, service);
            service->audio_processor_initialized = true;
        }
        
        // 重置解码器并设置预热
        audio_service_reset_decoder(service);
        service->audio_input_need_warmup = true;
        
        if (service->audio_processor) {
            audio_processor_start(service->audio_processor);
        }
        set_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    } else {
        if (service->audio_processor) {
            audio_processor_stop(service->audio_processor);
        }
        clear_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    }
}

void audio_service_enable_audio_testing(AudioService* service, bool enable) {
    if (!service) {
        return;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "%s audio testing", enable ? "Enabling" : "Disabling");
    
    if (enable) {
        set_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
    } else {
        clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
        
        // Move testing queue to decode queue
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
}

void audio_service_enable_device_aec(AudioService* service, bool enable) {
    if (!service) {
        return;
    }
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "%s device AEC", enable ? "Enabling" : "Disabling");
    
    if (!service->audio_processor_initialized && service->audio_processor) {
        // 需要创建配置结构体来初始化音频处理器
        audio_processor_config_t config;
        audio_processor_config_init_default(&config, 16000, 1, OPUS_FRAME_DURATION_MS);
        config.models_list = service->models_list;
        
        audio_processor_initialize(service->audio_processor, &config, service->codec);
        service->audio_processor_initialized = true;
    }
    
    if (service->audio_processor) {
        audio_processor_enable_device_aec(service->audio_processor, enable);
    }
}

bool audio_service_push_packet_to_decode_queue(AudioService* service, AudioStreamPacket* packet, bool wait) {
    if (!service || !packet) {
        return false;
    }
    
    pthread_mutex_lock(&service->audio_queue_mutex);
    
    if (audio_packet_queue_size(&service->audio_decode_queue) >= MAX_DECODE_PACKETS_IN_QUEUE) {
        if (wait) {
            while (audio_packet_queue_size(&service->audio_decode_queue) >= MAX_DECODE_PACKETS_IN_QUEUE && !service->service_stopped) {
                pthread_cond_wait(&service->audio_queue_cv, &service->audio_queue_mutex);
            }
        } else {
            pthread_mutex_unlock(&service->audio_queue_mutex);
            return false;
        }
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

void audio_service_encode_wake_word(AudioService* service) {
    if (!service || !service->wake_word) {
        return;
    }
    
    wake_word_interface_encode_wake_word_data(service->wake_word);
}

AudioStreamPacket* audio_service_pop_wake_word_packet(AudioService* service) {
    if (!service || !service->wake_word) {
        return NULL;
    }
    
    AudioStreamPacket* packet = audio_stream_packet_create();
    if (!packet) {
        return NULL;
    }
    
    size_t encoded_size = 0;
    if (wake_word_interface_get_wake_word_opus(service->wake_word, packet->payload, 
                                              packet->payload_capacity, &encoded_size)) {
        packet->payload_size = encoded_size;
        return packet;
    }
    
    audio_stream_packet_destroy(packet);
    return NULL;
}

const char* audio_service_get_last_wake_word(AudioService* service) {
    if (!service || !service->wake_word) {
        return NULL;
    }
    
    return wake_word_interface_get_last_detected_wake_word(service->wake_word);
}

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

bool audio_service_is_wake_word_running(const AudioService* service) {
    return service ? (get_event_bits((AudioService*)service) & AS_EVENT_WAKE_WORD_RUNNING) != 0 : false;
}

bool audio_service_is_audio_processor_running(const AudioService* service) {
    return service ? (get_event_bits((AudioService*)service) & AS_EVENT_AUDIO_PROCESSOR_RUNNING) != 0 : false;
}

void audio_service_play_sound(AudioService* service, const uint8_t* ogg_data, size_t ogg_size) {
    if (!service || !ogg_data || ogg_size == 0) {
        return;
    }
    
    // Simplified OGG playback - in real implementation, this would parse OGG and decode
    // For now, just log the action
    LINX_LOGI(AUDIO_SERVICE_TAG, "Playing sound of size %zu bytes", ogg_size);
}

bool audio_service_read_audio_data(AudioService* service, int16_t* data, int sample_rate, int samples) {
    if (!service || !data || !service->codec) {
        return false;
    }
    
    // Simplified audio reading - in real implementation, this would use the audio interface
    // For now, just fill with silence or test data
    memset(data, 0, samples * sizeof(int16_t));
    
    // Update timing
    get_current_time(&service->last_input_time);
    service->debug_statistics.input_count++;
    
    return true;
}

void audio_service_reset_decoder(AudioService* service) {
    if (!service || !service->opus_decoder) {
        return;
    }
    
    // Reset the opus decoder
    audio_codec_reset(service->opus_decoder);
}

void audio_service_set_models_list(AudioService* service, void* models_list) {
    if (!service) {
        return;
    }
    
    service->models_list = models_list;
}