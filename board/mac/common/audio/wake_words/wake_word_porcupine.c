#include "wake_word_porcupine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

/**
 * @file wake_word_porcupine.c
 * @brief Mac版本基于Porcupine的唤醒词实现
 */

// 前向声明
static int porcupine_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data);
static void porcupine_feed(WakeWordInterface* self, const int16_t* data, size_t size);
static void porcupine_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);
static void porcupine_start(WakeWordInterface* self);
static void porcupine_stop(WakeWordInterface* self);
static size_t porcupine_get_feed_size(WakeWordInterface* self);
static void porcupine_encode_wake_word_data(WakeWordInterface* self);
static bool porcupine_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                        size_t buffer_size, size_t* encoded_size);
static const char* porcupine_get_last_detected_wake_word(WakeWordInterface* self);
static void porcupine_destroy(WakeWordInterface* self);

// 音频处理线程
static void* porcupine_processing_thread(void* arg);

// 虚函数表
static const WakeWordInterfaceVTable porcupine_vtable = {
    .initialize = porcupine_initialize,
    .feed = porcupine_feed,
    .set_callback = porcupine_set_callback,
    .start = porcupine_start,
    .stop = porcupine_stop,
    .get_feed_size = porcupine_get_feed_size,
    .encode_wake_word_data = porcupine_encode_wake_word_data,
    .get_wake_word_opus = porcupine_get_wake_word_opus,
    .get_last_detected_wake_word = porcupine_get_last_detected_wake_word,
    .destroy = porcupine_destroy
};

WakeWordInterface* wake_word_porcupine_create(const porcupine_config_t* config) {
    if (!config || !config->access_key) {
        return NULL;
    }
    
    // 分配接口结构体
    WakeWordInterface* interface = (WakeWordInterface*)calloc(1, sizeof(WakeWordInterface));
    if (!interface) {
        return NULL;
    }
    
    // 分配实现数据
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)calloc(1, sizeof(PorcupineWakeWordData));
    if (!data) {
        free(interface);
        return NULL;
    }
    
    // 初始化接口
    interface->vtable = &porcupine_vtable;
    interface->impl_data = data;
    interface->is_initialized = false;
    interface->is_running = false;
    interface->last_detected_wake_word = NULL;
    interface->feed_size = 512; // Porcupine默认帧大小
    
    // 复制配置
    data->config.access_key = strdup(config->access_key);
    data->config.num_keywords = config->num_keywords;
    
    if (config->num_keywords > 0) {
        data->config.keyword_paths = (char**)malloc(config->num_keywords * sizeof(char*));
        data->config.sensitivities = (float*)malloc(config->num_keywords * sizeof(float));
        
        for (int i = 0; i < config->num_keywords; i++) {
            data->config.keyword_paths[i] = strdup(config->keyword_paths[i]);
            data->config.sensitivities[i] = config->sensitivities[i];
        }
    }
    
    if (config->model_path) {
        data->config.model_path = strdup(config->model_path);
    }
    
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&data->mutex, NULL);
    pthread_cond_init(&data->cond, NULL);
    data->thread_running = false;
    data->should_stop = false;
    data->last_keyword_index = -1;
    
    return interface;
}

static int porcupine_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    // 初始化Porcupine引擎
    pv_status_t status = pv_porcupine_init(
        data->config.access_key,
        data->config.model_path,
        data->config.num_keywords,
        (const char* const*)data->config.keyword_paths,
        data->config.sensitivities,
        &data->porcupine
    );
    
    if (status != PV_STATUS_SUCCESS) {
        return -1;
    }
    
    // 获取帧长度
    self->feed_size = pv_porcupine_frame_length();
    
    // 分配音频缓冲区
    data->buffer_size = self->feed_size * 2; // 双缓冲
    data->audio_buffer = (int16_t*)malloc(data->buffer_size * sizeof(int16_t));
    if (!data->audio_buffer) {
        pv_porcupine_delete(data->porcupine);
        return -1;
    }
    data->buffer_pos = 0;
    
    // 分配Opus编码缓冲区
    data->opus_encoded_data = (uint8_t*)malloc(4096); // 4KB缓冲区
    if (!data->opus_encoded_data) {
        free(data->audio_buffer);
        pv_porcupine_delete(data->porcupine);
        return -1;
    }
    
    self->codec = codec;
    self->user_data = user_data;
    self->is_initialized = true;
    
    return 0;
}

static void porcupine_feed(WakeWordInterface* self, const int16_t* data, size_t size) {
    if (!self || !self->impl_data || !self->is_initialized || !data) {
        return;
    }
    
    PorcupineWakeWordData* pdata = (PorcupineWakeWordData*)self->impl_data;
    
    pthread_mutex_lock(&pdata->mutex);
    
    // 将数据添加到缓冲区
    for (size_t i = 0; i < size && pdata->buffer_pos < pdata->buffer_size; i++) {
        pdata->audio_buffer[pdata->buffer_pos++] = data[i];
        
        // 当缓冲区有足够数据时，处理一帧
        if (pdata->buffer_pos >= self->feed_size) {
            int32_t keyword_index = -1;
            pv_status_t status = pv_porcupine_process(
                pdata->porcupine,
                pdata->audio_buffer,
                &keyword_index
            );
            
            if (status == PV_STATUS_SUCCESS && keyword_index >= 0) {
                pdata->last_keyword_index = keyword_index;
                
                // 触发回调
                if (self->callback && keyword_index < pdata->config.num_keywords) {
                    // 从路径中提取关键词名称
                    char* keyword_name = basename(pdata->config.keyword_paths[keyword_index]);
                    if (keyword_name) {
                        // 移除.ppn扩展名
                        char* dot = strrchr(keyword_name, '.');
                        if (dot) *dot = '\0';
                        
                        // 更新最后检测到的唤醒词
                        if (self->last_detected_wake_word) {
                            free(self->last_detected_wake_word);
                        }
                        self->last_detected_wake_word = strdup(keyword_name);
                        
                        self->callback(keyword_name, self->callback_user_data);
                    }
                }
            }
            
            // 移动缓冲区数据
            memmove(pdata->audio_buffer, 
                   pdata->audio_buffer + self->feed_size,
                   (pdata->buffer_pos - self->feed_size) * sizeof(int16_t));
            pdata->buffer_pos -= self->feed_size;
        }
    }
    
    pthread_mutex_unlock(&pdata->mutex);
}

static void porcupine_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data) {
    if (!self) return;
    
    self->callback = callback;
    self->callback_user_data = user_data;
}

static void porcupine_start(WakeWordInterface* self) {
    if (!self || !self->impl_data || !self->is_initialized) {
        return;
    }
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    pthread_mutex_lock(&data->mutex);
    if (!data->thread_running) {
        data->should_stop = false;
        if (pthread_create(&data->processing_thread, NULL, porcupine_processing_thread, self) == 0) {
            data->thread_running = true;
            self->is_running = true;
        }
    }
    pthread_mutex_unlock(&data->mutex);
}

static void porcupine_stop(WakeWordInterface* self) {
    if (!self || !self->impl_data) {
        return;
    }
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    pthread_mutex_lock(&data->mutex);
    if (data->thread_running) {
        data->should_stop = true;
        pthread_cond_signal(&data->cond);
    }
    pthread_mutex_unlock(&data->mutex);
    
    if (data->thread_running) {
        pthread_join(data->processing_thread, NULL);
        data->thread_running = false;
    }
    
    self->is_running = false;
}

static size_t porcupine_get_feed_size(WakeWordInterface* self) {
    if (!self) return 0;
    return self->feed_size;
}

static void porcupine_encode_wake_word_data(WakeWordInterface* self) {
    if (!self || !self->impl_data) {
        return;
    }
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    // 这里可以实现Opus编码逻辑
    // 暂时使用简单的数据复制作为占位符
    if (data->audio_buffer && data->buffer_pos > 0) {
        size_t bytes_to_copy = data->buffer_pos * sizeof(int16_t);
        if (bytes_to_copy <= 4096) {
            memcpy(data->opus_encoded_data, data->audio_buffer, bytes_to_copy);
            data->opus_encoded_size = bytes_to_copy;
        }
    }
}

static bool porcupine_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                        size_t buffer_size, size_t* encoded_size) {
    if (!self || !self->impl_data || !opus_data || !encoded_size) {
        return false;
    }
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    if (data->opus_encoded_size > 0 && data->opus_encoded_size <= buffer_size) {
        memcpy(opus_data, data->opus_encoded_data, data->opus_encoded_size);
        *encoded_size = data->opus_encoded_size;
        return true;
    }
    
    return false;
}

static const char* porcupine_get_last_detected_wake_word(WakeWordInterface* self) {
    if (!self) return NULL;
    return self->last_detected_wake_word;
}

static void porcupine_destroy(WakeWordInterface* self) {
    if (!self || !self->impl_data) {
        return;
    }
    
    // 停止处理
    porcupine_stop(self);
    
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    // 清理Porcupine引擎
    if (data->porcupine) {
        pv_porcupine_delete(data->porcupine);
    }
    
    // 清理配置
    porcupine_config_destroy(&data->config);
    
    // 清理缓冲区
    if (data->audio_buffer) {
        free(data->audio_buffer);
    }
    if (data->opus_encoded_data) {
        free(data->opus_encoded_data);
    }
    
    // 清理线程资源
    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->cond);
    
    // 清理检测到的关键词
    for (int i = 0; i < 10; i++) {
        if (data->detected_keywords[i]) {
            free(data->detected_keywords[i]);
        }
    }
    
    if (self->last_detected_wake_word) {
        free(self->last_detected_wake_word);
    }
    
    free(data);
    free(self);
}

static void* porcupine_processing_thread(void* arg) {
    WakeWordInterface* self = (WakeWordInterface*)arg;
    PorcupineWakeWordData* data = (PorcupineWakeWordData*)self->impl_data;
    
    while (!data->should_stop) {
        pthread_mutex_lock(&data->mutex);
        
        // 等待条件或超时
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 100000000; // 100ms
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }
        
        pthread_cond_timedwait(&data->cond, &data->mutex, &timeout);
        pthread_mutex_unlock(&data->mutex);
        
        // 这里可以添加额外的处理逻辑
        usleep(10000); // 10ms
    }
    
    return NULL;
}

// 配置相关函数实现
int porcupine_config_set_default(porcupine_config_t* config, const char* access_key) {
    if (!config || !access_key) {
        return -1;
    }
    
    memset(config, 0, sizeof(porcupine_config_t));
    config->access_key = strdup(access_key);
    config->num_keywords = 0;
    config->keyword_paths = NULL;
    config->sensitivities = NULL;
    config->model_path = porcupine_get_default_model_path();
    
    return 0;
}

int porcupine_config_add_keyword(porcupine_config_t* config, const char* keyword_path, float sensitivity) {
    if (!config || !keyword_path) {
        return -1;
    }
    
    // 重新分配数组
    config->keyword_paths = (char**)realloc(config->keyword_paths, 
                                           (config->num_keywords + 1) * sizeof(char*));
    config->sensitivities = (float*)realloc(config->sensitivities,
                                           (config->num_keywords + 1) * sizeof(float));
    
    if (!config->keyword_paths || !config->sensitivities) {
        return -1;
    }
    
    config->keyword_paths[config->num_keywords] = strdup(keyword_path);
    config->sensitivities[config->num_keywords] = sensitivity;
    config->num_keywords++;
    
    return 0;
}

void porcupine_config_destroy(porcupine_config_t* config) {
    if (!config) return;
    
    if (config->access_key) {
        free(config->access_key);
    }
    
    if (config->model_path) {
        free(config->model_path);
    }
    
    if (config->keyword_paths) {
        for (int i = 0; i < config->num_keywords; i++) {
            if (config->keyword_paths[i]) {
                free(config->keyword_paths[i]);
            }
        }
        free(config->keyword_paths);
    }
    
    if (config->sensitivities) {
        free(config->sensitivities);
    }
    
    memset(config, 0, sizeof(porcupine_config_t));
}

char* porcupine_get_default_keyword_path(const char* keyword_name) {
    if (!keyword_name) return NULL;
    
    // 构建默认关键词路径
    // 假设关键词文件位于 /usr/local/share/porcupine/keywords/
    char* path = (char*)malloc(256);
    if (path) {
        snprintf(path, 256, "/usr/local/share/porcupine/keywords/%s_mac.ppn", keyword_name);
    }
    
    return path;
}

char* porcupine_get_default_model_path(void) {
    // 构建默认模型路径
    char* path = (char*)malloc(256);
    if (path) {
        snprintf(path, 256, "/usr/local/share/porcupine/lib/common/porcupine_params.pv");
    }
    
    return path;
}