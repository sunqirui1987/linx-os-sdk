#ifndef WAKE_WORD_INTERFACE_H
#define WAKE_WORD_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../codecs/audio_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct WakeWordInterface WakeWordInterface;

/**
 * 唤醒词检测回调函数类型
 * @param wake_word 检测到的唤醒词字符串
 * @param user_data 传递给回调函数的用户自定义数据
 */
typedef void (*wake_word_callback_t)(const char* wake_word, void* user_data);

/**
 * 唤醒词接口函数指针表
 */
typedef struct {
    /**
     * 初始化唤醒词检测器
     * @param self 唤醒词接口指针
     * @param codec 音频编解码器指针
     * @param user_data 用户自定义数据（替代原来的models_list）
     * @return 成功返回0，失败返回负数
     */
    int (*initialize)(WakeWordInterface* self, audio_codec_t* codec, void* user_data);
    
    /**
     * 向唤醒词检测器输入音频数据
     * @param self 唤醒词接口指针
     * @param data 音频数据缓冲区指针（16位有符号整数）
     * @param size 数据缓冲区中的采样点数量
     */
    void (*feed)(WakeWordInterface* self, const int16_t* data, size_t size);
    
    /**
     * 设置唤醒词检测回调函数
     * @param self 唤醒词接口指针
     * @param callback 检测到唤醒词时调用的回调函数
     * @param user_data 传递给回调函数的用户自定义数据
     */
    void (*set_callback)(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);
    
    /**
     * 启动唤醒词检测
     * @param self 唤醒词接口指针
     */
    void (*start)(WakeWordInterface* self);
    
    /**
     * 停止唤醒词检测
     * @param self 唤醒词接口指针
     */
    void (*stop)(WakeWordInterface* self);
    
    /**
     * 获取音频数据所需的输入大小
     * @param self 唤醒词接口指针
     * @return 所需的输入大小（采样点数）
     */
    size_t (*get_feed_size)(WakeWordInterface* self);
    
    /**
     * 编码唤醒词数据用于传输
     * @param self 唤醒词接口指针
     */
    void (*encode_wake_word_data)(WakeWordInterface* self);
    
    /**
     * 获取Opus格式的编码唤醒词数据
     * @param self 唤醒词接口指针
     * @param opus_data 存储Opus数据的缓冲区指针
     * @param buffer_size 输出缓冲区大小
     * @param encoded_size 存储实际编码大小的指针
     * @return 成功返回true，失败返回false
     */
    bool (*get_wake_word_opus)(WakeWordInterface* self, uint8_t* opus_data, 
                              size_t buffer_size, size_t* encoded_size);
    
    /**
     * 获取最后检测到的唤醒词
     * @param self 唤醒词接口指针
     * @return 最后检测到的唤醒词字符串指针（可能为NULL）
     */
    const char* (*get_last_detected_wake_word)(WakeWordInterface* self);
    
    /**
     * 销毁唤醒词接口并释放资源
     * @param self 唤醒词接口指针
     */
    void (*destroy)(WakeWordInterface* self);
    
} WakeWordInterfaceVTable;

/**
 * 基础唤醒词接口结构体
 */
struct WakeWordInterface {
    const WakeWordInterfaceVTable* vtable;
    void* impl_data;  // 实现特定数据
    
    // 配置信息
    audio_codec_t* codec;
    void* user_data;  // 用户自定义数据（替代原来的models_list）
    
    // 回调信息
    wake_word_callback_t callback;
    void* callback_user_data;
    
    // 状态信息
    bool is_initialized;
    bool is_running;
    char* last_detected_wake_word;
    size_t feed_size;
    
    // Opus编码缓冲区
    uint8_t* opus_buffer;
    size_t opus_buffer_size;
    size_t opus_data_size;
};

/**
 * 初始化唤醒词接口
 */
int wake_word_interface_initialize(WakeWordInterface* self, audio_codec_t* codec, void* user_data);

/**
 * 向唤醒词检测器输入音频数据
 */
void wake_word_interface_feed(WakeWordInterface* self, const int16_t* data, size_t size);

/**
 * 设置唤醒词检测回调函数
 */
void wake_word_interface_set_callback(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);

/**
 * 启动唤醒词检测
 */
void wake_word_interface_start(WakeWordInterface* self);

/**
 * 停止唤醒词检测
 */
void wake_word_interface_stop(WakeWordInterface* self);

/**
 * 获取所需的输入大小
 */
size_t wake_word_interface_get_feed_size(WakeWordInterface* self);

/**
 * 编码唤醒词数据
 */
void wake_word_interface_encode_wake_word_data(WakeWordInterface* self);

/**
 * 获取唤醒词Opus数据
 */
bool wake_word_interface_get_wake_word_opus(WakeWordInterface* self, uint8_t* opus_data, 
                                           size_t buffer_size, size_t* encoded_size);

/**
 * 获取最后检测到的唤醒词
 */
const char* wake_word_interface_get_last_detected_wake_word(WakeWordInterface* self);

/**
 * 销毁唤醒词接口
 */
void wake_word_interface_destroy(WakeWordInterface* self);

#ifdef __cplusplus
}
#endif

#endif // WAKE_WORD_INTERFACE_H