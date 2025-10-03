#include "wake_word_stub.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 日志标签
#define TAG "WAKE_WORD_STUB"

// 虚函数表函数的前向声明
static int wake_word_stub_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data);
static void wake_word_stub_feed(WakeWordInterface* self, const int16_t* data, size_t size);
static void wake_word_stub_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);
static void wake_word_stub_start(WakeWordInterface* self);
static void wake_word_stub_stop(WakeWordInterface* self);
static size_t wake_word_stub_get_feed_size(WakeWordInterface* self);
static void wake_word_stub_encode_wake_word_data(WakeWordInterface* self);
static bool wake_word_stub_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                             size_t buffer_size, size_t* encoded_size);
static const char* wake_word_stub_get_last_detected_wake_word(WakeWordInterface* self);
static void wake_word_stub_destroy_impl(WakeWordInterface* self);

// Stub vtable
static const WakeWordInterfaceVTable wake_word_stub_vtable = {
    .initialize = wake_word_stub_initialize,
    .feed = wake_word_stub_feed,
    .set_callback = wake_word_stub_set_callback,
    .start = wake_word_stub_start,
    .stop = wake_word_stub_stop,
    .get_feed_size = wake_word_stub_get_feed_size,
    .encode_wake_word_data = wake_word_stub_encode_wake_word_data,
    .get_wake_word_opus = wake_word_stub_get_wake_word_opus,
    .get_last_detected_wake_word = wake_word_stub_get_last_detected_wake_word,
    .destroy = wake_word_stub_destroy_impl
};

WakeWordInterface* wake_word_stub_create(void) {
    WakeWordInterface* interface = (WakeWordInterface*)malloc(sizeof(WakeWordInterface));
    if (!interface) {
        return NULL;
    }
    
    WakeWordStubData* data = (WakeWordStubData*)malloc(sizeof(WakeWordStubData));
    if (!data) {
        free(interface);
        return NULL;
    }
    
    // Initialize interface
    interface->vtable = &wake_word_stub_vtable;
    interface->impl_data = data;
    interface->codec = NULL;
    interface->user_data = NULL;
    interface->callback = NULL;
    interface->callback_user_data = NULL;
    interface->is_initialized = false;
    interface->is_running = false;
    interface->last_detected_wake_word = NULL;
    interface->feed_size = 1024; // Default feed size
    interface->opus_buffer = NULL;
    interface->opus_buffer_size = 0;
    interface->opus_data_size = 0;
    
    // Initialize stub data
    data->detection_enabled = false;
    data->detection_threshold = 1000;
    data->samples_processed = 0;
    memset(data->detected_word, 0, sizeof(data->detected_word));
    data->audio_buffer = NULL;
    data->buffer_size = 0;
    data->buffer_pos = 0;
    data->opus_data = NULL;
    data->opus_size = 0;
    
    return interface;
}

void wake_word_stub_destroy(WakeWordInterface* interface) {
    if (interface) {
        wake_word_interface_destroy(interface);
    }
}

// Implementation of vtable functions
// 桩实现函数
static int wake_word_stub_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data) {
    LINX_LOGI(TAG, "初始化唤醒词桩实现");
    
    if (!self || !self->impl_data) {
        LINX_LOGE(TAG, "初始化失败：无效的接口指针或数据");
        return -1;
    }
    
    WakeWordStubData* data = (WakeWordStubData*)self->impl_data;
    
    self->codec = codec;
    self->user_data = user_data;
    
    // 初始化音频缓冲区
    data->buffer_size = self->feed_size * 4; // 缓冲4帧数据
    data->audio_buffer = (int16_t*)malloc(data->buffer_size * sizeof(int16_t));
    if (!data->audio_buffer) {
        LINX_LOGE(TAG, "音频缓冲区分配失败");
        return -1;
    }
    data->buffer_pos = 0;
    LINX_LOGI(TAG, "音频缓冲区分配成功，大小：%zu", data->buffer_size);
    
    // 初始化Opus缓冲区
    self->opus_buffer_size = 1024; // 1KB缓冲区
    self->opus_buffer = (uint8_t*)malloc(self->opus_buffer_size);
    if (!self->opus_buffer) {
        LINX_LOGE(TAG, "Opus缓冲区分配失败");
        free(data->audio_buffer);
        data->audio_buffer = NULL;
        return -1;
    }
    
    self->is_initialized = true;
    
    LINX_LOGI(TAG, "唤醒词桩实现初始化成功，用户数据：%p", user_data);
    
    return 0;
}

static void wake_word_stub_feed(WakeWordInterface* self, const int16_t* data, size_t size) {
    if (!self || !self->impl_data || !data || !self->is_running) {
        LINX_LOGE(TAG, "音频数据输入失败：无效的参数或未运行状态");
        return;
    }
    
    WakeWordStubData* stub_data = (WakeWordStubData*)self->impl_data;
    
    // 模拟处理音频数据
    stub_data->samples_processed += size;
    
    // 将数据复制到内部缓冲区
    for (size_t i = 0; i < size && stub_data->buffer_pos < stub_data->buffer_size; i++) {
        stub_data->audio_buffer[stub_data->buffer_pos++] = data[i];
    }
    
    // 简单的唤醒词检测模拟
    // 每10000个采样点检测一次唤醒词（在16kHz采样率下大约每0.625秒）
    if (stub_data->samples_processed % 10000 == 0 && stub_data->detection_enabled) {
        strcpy(stub_data->detected_word, "hello");
        LINX_LOGI(TAG, "模拟检测到唤醒词：%s，已处理采样点：%zu", stub_data->detected_word, stub_data->samples_processed);
        
        // 更新最后检测到的唤醒词
        if (self->last_detected_wake_word) {
            free(self->last_detected_wake_word);
        }
        self->last_detected_wake_word = (char*)malloc(strlen(stub_data->detected_word) + 1);
        if (self->last_detected_wake_word) {
            strcpy(self->last_detected_wake_word, stub_data->detected_word);
        }
        
        // 如果设置了回调函数则调用
        if (self->callback) {
            self->callback(stub_data->detected_word, self->callback_user_data);
            LINX_LOGI(TAG, "已调用唤醒词检测回调函数");
        } else {
            LINX_LOGW(TAG, "回调函数未设置，无法通知唤醒词检测结果");
        }
        
        printf("Wake word detected: %s\n", stub_data->detected_word);
    }
    
    // 每处理1000个采样点记录一次日志
    if (stub_data->samples_processed % 1000 == 0) {
        LINX_LOGI(TAG, "已处理音频采样点：%zu，缓冲区位置：%zu", stub_data->samples_processed, stub_data->buffer_pos);
    }
}

static void wake_word_stub_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data) {
    if (!self) return;
    
    self->callback = callback;
    self->callback_user_data = user_data;
}

static void wake_word_stub_start(WakeWordInterface* self) {
    if (!self || !self->impl_data) return;
    
    WakeWordStubData* data = (WakeWordStubData*)self->impl_data;
    
    self->is_running = true;
    data->detection_enabled = true;
    data->samples_processed = 0;
    
    printf("Wake word detection started\n");
}

static void wake_word_stub_stop(WakeWordInterface* self) {
    if (!self || !self->impl_data) return;
    
    WakeWordStubData* data = (WakeWordStubData*)self->impl_data;
    
    self->is_running = false;
    data->detection_enabled = false;
    
    printf("Wake word detection stopped\n");
}

static size_t wake_word_stub_get_feed_size(WakeWordInterface* self) {
    if (!self) return 0;
    
    return self->feed_size;
}

static void wake_word_stub_encode_wake_word_data(WakeWordInterface* self) {
    if (!self || !self->impl_data || !self->opus_buffer) return;
    
    WakeWordStubData* data = (WakeWordStubData*)self->impl_data;
    
    // Simulate Opus encoding
    // In a real implementation, this would encode the audio data using Opus codec
    const char* dummy_opus = "OPUS_ENCODED_DATA";
    size_t dummy_size = strlen(dummy_opus);
    
    if (dummy_size <= self->opus_buffer_size) {
        memcpy(self->opus_buffer, dummy_opus, dummy_size);
        self->opus_data_size = dummy_size;
        
        printf("Wake word data encoded to Opus format (%zu bytes)\n", dummy_size);
    }
}

static bool wake_word_stub_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                             size_t buffer_size, size_t* encoded_size) {
    if (!self || !opus_data || !encoded_size) {
        if (encoded_size) *encoded_size = 0;
        return false;
    }
    
    if (self->opus_data_size == 0 || !self->opus_buffer) {
        *encoded_size = 0;
        return false;
    }
    
    if (buffer_size < self->opus_data_size) {
        *encoded_size = self->opus_data_size;
        return false;
    }
    
    memcpy(opus_data, self->opus_buffer, self->opus_data_size);
    *encoded_size = self->opus_data_size;
    
    return true;
}

static const char* wake_word_stub_get_last_detected_wake_word(WakeWordInterface* self) {
    if (!self) return NULL;
    
    return self->last_detected_wake_word;
}

static void wake_word_stub_destroy_impl(WakeWordInterface* self) {
    if (!self) return;
    
    WakeWordStubData* data = (WakeWordStubData*)self->impl_data;
    
    if (data) {
        if (data->audio_buffer) {
            free(data->audio_buffer);
        }
        if (data->opus_data) {
            free(data->opus_data);
        }
        free(data);
    }
    
    if (self->opus_buffer) {
        free(self->opus_buffer);
    }
    
    if (self->last_detected_wake_word) {
        free(self->last_detected_wake_word);
    }
    
    free(self);
}