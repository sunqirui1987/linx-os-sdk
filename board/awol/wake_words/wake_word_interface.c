#include "wake_word_interface.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// 日志标签
#define TAG "WAKE_WORD_INTERFACE"

// 接口实现函数

/**
 * 初始化唤醒词接口
 */
int wake_word_interface_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data) {
    if (!self || !self->vtable || !self->vtable->initialize) {
        LINX_LOGE(TAG, "初始化失败：无效的参数或虚函数表");
        return -1;
    }
    
    LINX_LOGI(TAG, "开始初始化唤醒词接口，用户数据: %p", user_data);
    
    int result = self->vtable->initialize(self, codec, user_data);
    if (result == 0) {
        LINX_LOGI(TAG, "唤醒词接口初始化成功");
    } else {
        LINX_LOGE(TAG, "唤醒词接口初始化失败，错误码: %d", result);
    }
    
    return result;
}

/**
 * 向唤醒词检测器输入音频数据
 */
void wake_word_interface_feed(WakeWordInterface* self, const int16_t* data, size_t size) {
    if (!self || !self->vtable || !self->vtable->feed) {
        LINX_LOGE(TAG, "输入音频数据失败：无效的参数或虚函数表");
        return;
    }
    
    if (!data || size == 0) {
        LINX_LOGW(TAG, "输入音频数据为空或大小为0");
        return;
    }
    
    self->vtable->feed(self, data, size);
}

/**
 * 设置唤醒词检测回调函数
 */
void wake_word_interface_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data) {
    if (!self || !self->vtable || !self->vtable->set_callback) {
        LINX_LOGE(TAG, "设置回调函数失败：无效的参数或虚函数表");
        return;
    }
    
    LINX_LOGI(TAG, "设置唤醒词检测回调函数");
    self->vtable->set_callback(self, callback, user_data);
}

/**
 * 启动唤醒词检测
 */
void wake_word_interface_start(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->start) {
        LINX_LOGE(TAG, "启动唤醒词检测失败：无效的参数或虚函数表");
        return;
    }
    
    LINX_LOGI(TAG, "启动唤醒词检测");
    self->vtable->start(self);
}

/**
 * 停止唤醒词检测
 */
void wake_word_interface_stop(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->stop) {
        LINX_LOGE(TAG, "停止唤醒词检测失败：无效的参数或虚函数表");
        return;
    }
    
    LINX_LOGI(TAG, "停止唤醒词检测");
    self->vtable->stop(self);
}

/**
 * 获取所需的输入大小
 */
size_t wake_word_interface_get_feed_size(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->get_feed_size) {
        LINX_LOGE(TAG, "获取输入大小失败：无效的参数或虚函数表");
        return 0;
    }
    
    size_t feed_size = self->vtable->get_feed_size(self);
    LINX_LOGI(TAG, "获取输入大小: %zu", feed_size);
    return feed_size;
}

/**
 * 编码唤醒词数据
 */
void wake_word_interface_encode_wake_word_data(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->encode_wake_word_data) {
        LINX_LOGE(TAG, "编码唤醒词数据失败：无效的参数或虚函数表");
        return;
    }
    
    LINX_LOGI(TAG, "开始编码唤醒词数据");
    self->vtable->encode_wake_word_data(self);
}

/**
 * 获取唤醒词Opus数据
 */
bool wake_word_interface_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                           size_t buffer_size, size_t* encoded_size) {
    if (!self || !self->vtable || !self->vtable->get_wake_word_opus) {
        LINX_LOGE(TAG, "获取Opus数据失败：无效的参数或虚函数表");
        if (encoded_size) {
            *encoded_size = 0;
        }
        return false;
    }
    
    if (!opus_data || !encoded_size) {
        LINX_LOGE(TAG, "获取Opus数据失败：无效的输出参数");
        if (encoded_size) {
            *encoded_size = 0;
        }
        return false;
    }
    
    bool result = self->vtable->get_wake_word_opus(self, opus_data, buffer_size, encoded_size);
    if (result) {
        LINX_LOGI(TAG, "成功获取Opus数据，大小: %zu字节", *encoded_size);
    } else {
        LINX_LOGW(TAG, "获取Opus数据失败或无数据可用");
    }
    
    return result;
}

/**
 * 获取最后检测到的唤醒词
 */
const char* wake_word_interface_get_last_detected_wake_word(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->get_last_detected_wake_word) {
        LINX_LOGE(TAG, "获取最后检测到的唤醒词失败：无效的参数或虚函数表");
        return NULL;
    }
    
    const char* wake_word = self->vtable->get_last_detected_wake_word(self);
    if (wake_word) {
        LINX_LOGI(TAG, "获取最后检测到的唤醒词: %s", wake_word);
    } else {
        LINX_LOGI(TAG, "暂无检测到的唤醒词");
    }
    
    return wake_word;
}

/**
 * 销毁唤醒词接口
 */
void wake_word_interface_destroy(WakeWordInterface* self) {
    if (!self || !self->vtable || !self->vtable->destroy) {
        LINX_LOGE(TAG, "销毁唤醒词接口失败：无效的参数或虚函数表");
        return;
    }
    
    LINX_LOGI(TAG, "销毁唤醒词接口");
    self->vtable->destroy(self);
}