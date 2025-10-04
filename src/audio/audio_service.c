/**
 * @file audio_service.c
 * @brief 音频服务实现
 * @details 提供音频录制、播放、编解码、唤醒词检测等功能的统一服务实现
 */

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
    if (!service) return;
    
    if (strcmp(event_type, "send_queue_available") == 0 && service->callbacks.on_send_queue_available) {
        service->callbacks.on_send_queue_available(service->callbacks.user_data);
    } else if (strcmp(event_type, "wake_word_detected") == 0 && service->callbacks.on_wake_word_detected) {
        const char* wake_word = (const char*)event_data;
        service->callbacks.on_wake_word_detected(wake_word, service->callbacks.user_data);
    } else if (strcmp(event_type, "vad_change") == 0 && service->callbacks.on_vad_change) {
        bool speaking = *(const bool*)event_data;
        service->callbacks.on_vad_change(speaking, service->callbacks.user_data);
    } else if (strcmp(event_type, "testing_queue_full") == 0 && service->callbacks.on_audio_testing_queue_full) {
        service->callbacks.on_audio_testing_queue_full(service->callbacks.user_data);
    }
}

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
        
        // 处理各种模式的音频输入
        if (bits & AS_EVENT_AUDIO_TESTING_RUNNING) {
            // 音频测试模式处理
            if (audio_packet_queue_size(&service->audio_testing_queue) >= 
                AUDIO_TESTING_MAX_DURATION_MS / OPUS_FRAME_DURATION_MS) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Audio testing queue is full, stopping audio testing");
                clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING);
                trigger_callbacks(service, "testing_queue_full", NULL);
                continue;
            }
            
            // 从音频接口读取数据
            if (service->audio_interface && service->audio_interface->vtable && service->audio_interface->vtable->read) {
                int samples = OPUS_FRAME_DURATION_MS * service->config.input_format.sample_rate / 1000;
                int16_t* data = (int16_t*)malloc(samples * sizeof(int16_t));
                if (data) {
                    int result = service->audio_interface->vtable->read(service->audio_interface, data, samples);
                    if (result > 0) {
                        // 创建测试数据包
                        AudioStreamPacket* packet = audio_stream_packet_create();
                        if (packet) {
                            audio_stream_packet_set_data(packet, data, result * sizeof(int16_t), &service->config.input_format);
                            
                            pthread_mutex_lock(&service->audio_queue_mutex);
                            if (!audio_packet_queue_push(&service->audio_testing_queue, packet)) {
                                audio_stream_packet_destroy(packet);
                            }
                            pthread_mutex_unlock(&service->audio_queue_mutex);
                        }
                    }
                    free(data);
                }
            }
            continue;
        }
        
        if (bits & AS_EVENT_WAKE_WORD_RUNNING) {
            // 唤醒词检测处理
            if (!is_component_available(service, COMPONENT_WAKE_WORD) || !is_component_available(service, COMPONENT_AUDIO_INTERFACE)) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Wake word detection enabled but components not available, disabling");
                clear_event_bit(service, AS_EVENT_WAKE_WORD_RUNNING);
                service->current_features.wake_word_detection = false;
                continue;
            }
            
            size_t feed_size = wake_word_interface_get_feed_size(service->wake_word);
            if (feed_size > 0) {
                int16_t* data = (int16_t*)malloc(feed_size * sizeof(int16_t));
                if (data) {
                    int result = service->audio_interface->vtable->read(service->audio_interface, data, feed_size);
                    if (result > 0) {
                        wake_word_interface_feed(service->wake_word, data, result);
                        
                        // 检查是否检测到唤醒词
                        const char* wake_word = wake_word_interface_get_last_detected_wake_word(service->wake_word);
                        if (wake_word) {
                            trigger_callbacks(service, "wake_word_detected", wake_word);
                        }
                    } else if (result < 0) {
                        LINX_LOGW(AUDIO_SERVICE_TAG, "Audio interface read error: %d", result);
                    }
                    free(data);
                } else {
                    LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to allocate memory for wake word audio data");
                }
            }
            continue;
        }
        
        if (bits & AS_EVENT_AUDIO_PROCESSOR_RUNNING) {
            // 音频处理器处理
            if (!is_component_available(service, COMPONENT_AUDIO_PROCESSOR) || !is_component_available(service, COMPONENT_AUDIO_INTERFACE)) {
                LINX_LOGW(AUDIO_SERVICE_TAG, "Audio processor enabled but components not available, disabling");
                clear_event_bit(service, AS_EVENT_AUDIO_PROCESSOR_RUNNING);
                service->current_features.voice_processing = false;
                continue;
            }
            
            size_t feed_size = audio_processor_get_feed_size(service->audio_processor);
            if (feed_size > 0) {
                int16_t* data = (int16_t*)malloc(feed_size * sizeof(int16_t));
                if (data) {
                    int result = service->audio_interface->vtable->read(service->audio_interface, data, feed_size);
                    if (result > 0) {
                        audio_processor_feed(service->audio_processor, data, result);
                    } else if (result < 0) {
                        LINX_LOGW(AUDIO_SERVICE_TAG, "Audio interface read error for processor: %d", result);
                    }
                    free(data);
                } else {
                    LINX_LOGE(AUDIO_SERVICE_TAG, "Failed to allocate memory for audio processor data");
                }
            }
            continue;
        }
        
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
            // 播放音频数据
            if (task->data && task->data_size > 0) {
                size_t sample_count = task->data_size / sizeof(int16_t);
                
                if (service->audio_interface && service->audio_interface->vtable && 
                    service->audio_interface->vtable->write) {
                    service->audio_interface->vtable->write(service->audio_interface, 
                                                          (short*)task->data, sample_count);
                }
            }
            
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
        
        // 解码处理
        if (!audio_packet_queue_is_empty(&service->audio_decode_queue) && 
            audio_task_queue_size(&service->audio_playback_queue) < MAX_PLAYBACK_TASKS_IN_QUEUE) {
            
            AudioStreamPacket* packet = audio_packet_queue_pop(&service->audio_decode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (packet) {
                // 解码逻辑
                size_t estimated_samples = (packet->sample_rate * packet->frame_duration) / 1000;
                int16_t* decoded_data = (int16_t*)malloc(estimated_samples * sizeof(int16_t));
                
                if (decoded_data && service->opus_decoder && packet->payload && packet->payload_size > 0) {
                    size_t decoded_size = 0;
                    codec_error_t result = audio_codec_decode(service->opus_decoder, 
                                                            packet->payload, packet->payload_size,
                                                            decoded_data, estimated_samples, &decoded_size);
                    if (result == CODEC_SUCCESS) {
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
                        }
                    } else {
                        free(decoded_data);
                    }
                }
                audio_stream_packet_destroy(packet);
            }
            pthread_mutex_lock(&service->audio_queue_mutex);
        }
        
        // 编码处理
        if (!audio_task_queue_is_empty(&service->audio_encode_queue) && 
            audio_packet_queue_size(&service->audio_send_queue) < MAX_SEND_PACKETS_IN_QUEUE) {
            
            AudioTask* task = audio_task_queue_pop(&service->audio_encode_queue);
            pthread_cond_broadcast(&service->audio_queue_cv);
            pthread_mutex_unlock(&service->audio_queue_mutex);
            
            if (task) {
                AudioStreamPacket* packet = audio_stream_packet_create();
                if (packet) {
                    packet->frame_duration = OPUS_FRAME_DURATION_MS;
                    packet->sample_rate = service->config.output_format.sample_rate;
                    packet->timestamp = task->timestamp;
                    
                    if (service->opus_encoder && task->data && task->data_size > 0) {
                        size_t encoded_size = 0;
                        size_t sample_count = task->data_size / sizeof(int16_t);
                        codec_error_t result = audio_codec_encode(service->opus_encoder,
                                                                (int16_t*)task->data, sample_count,
                                                                packet->payload, packet->payload_capacity, &encoded_size);
                        if (result == CODEC_SUCCESS) {
                            packet->payload_size = encoded_size;
                            
                            pthread_mutex_lock(&service->audio_queue_mutex);
                            if (!audio_packet_queue_push(&service->audio_send_queue, packet)) {
                                audio_stream_packet_destroy(packet);
                            } else {
                                pthread_cond_broadcast(&service->audio_queue_cv);
                                trigger_callbacks(service, "send_queue_available", NULL);
                            }
                            pthread_mutex_unlock(&service->audio_queue_mutex);
                            
                            service->debug_statistics.encode_count++;
                        } else {
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
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service initialized");
    return 0;
}

int audio_service_start(AudioService* service) {
    if (!service) {
        return -1;
    }
    
    service->service_stopped = false;
    clear_event_bit(service, AS_EVENT_AUDIO_TESTING_RUNNING | AS_EVENT_WAKE_WORD_RUNNING | AS_EVENT_AUDIO_PROCESSOR_RUNNING);
    
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
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service components set");
}

void audio_service_config_init_default(AudioServiceConfig* config) {
    if (!config) {
        return;
    }
    
    // 初始化默认音频格式
    audio_format_default(&config->input_format);
    audio_format_default(&config->output_format);
    
    // 初始化默认功能配置
    config->features.wake_word_detection = false;
    config->features.voice_processing = false;
    config->features.audio_testing = false;
    config->features.device_aec = false;
    config->features.noise_suppression = false;
    config->features.voice_activity_detection = false;
    
    // 初始化模型列表
    config->models_list = NULL;
    
    LINX_LOGI(AUDIO_SERVICE_TAG, "Audio service config initialized with default values");
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