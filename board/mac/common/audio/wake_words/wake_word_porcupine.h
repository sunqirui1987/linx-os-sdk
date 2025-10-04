#ifndef WAKE_WORD_PORCUPINE_H
#define WAKE_WORD_PORCUPINE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

// 条件包含Porcupine头文件
#ifdef __APPLE__
    // 尝试不同的包含路径
    #if __has_include(<pv_porcupine.h>)
        #include <pv_porcupine.h>
    #elif __has_include(<picovoice/pv_porcupine.h>)
        #include <picovoice/pv_porcupine.h>
    #else
        // 如果找不到头文件，提供基本定义
        typedef struct pv_porcupine pv_porcupine_t;
        typedef enum {
            PV_STATUS_SUCCESS = 0,
            PV_STATUS_OUT_OF_MEMORY = 1,
            PV_STATUS_IO_ERROR = 2,
            PV_STATUS_INVALID_ARGUMENT = 3,
            PV_STATUS_STOP_ITERATION = 4,
            PV_STATUS_KEY_ERROR = 5,
            PV_STATUS_INVALID_STATE = 6,
            PV_STATUS_RUNTIME_ERROR = 7,
            PV_STATUS_ACTIVATION_ERROR = 8,
            PV_STATUS_ACTIVATION_LIMIT_REACHED = 9,
            PV_STATUS_ACTIVATION_THROTTLED = 10,
            PV_STATUS_ACTIVATION_REFUSED = 11
        } pv_status_t;
        
        // 函数声明
        pv_status_t pv_porcupine_init(const char *access_key, const char *model_path, int32_t num_keywords, const char * const *keyword_paths, const float *sensitivities, pv_porcupine_t **object);
        void pv_porcupine_delete(pv_porcupine_t *object);
        pv_status_t pv_porcupine_process(pv_porcupine_t *object, const int16_t *pcm, int32_t *keyword_index);
        int32_t pv_porcupine_frame_length(void);
    #endif
#endif

// 前向声明和基本类型定义
typedef struct WakeWordInterface WakeWordInterface;
typedef struct audio_codec audio_codec_t;
typedef void (*wake_word_callback_t)(const char* wake_word, void* user_data);

// 虚函数表定义
typedef struct {
    int (*initialize)(WakeWordInterface* self, audio_codec_t* codec, void* user_data);
    void (*feed)(WakeWordInterface* self, const int16_t* data, size_t size);
    void (*set_callback)(WakeWordInterface* self, wake_word_callback_t callback, void* user_data);
    void (*start)(WakeWordInterface* self);
    void (*stop)(WakeWordInterface* self);
    size_t (*get_feed_size)(WakeWordInterface* self);
    void (*encode_wake_word_data)(WakeWordInterface* self);
    bool (*get_wake_word_opus)(WakeWordInterface* self, uint8_t* opus_data, size_t buffer_size, size_t* encoded_size);
    const char* (*get_last_detected_wake_word)(WakeWordInterface* self);
    void (*destroy)(WakeWordInterface* self);
} WakeWordInterfaceVTable;

// 基础接口结构体
struct WakeWordInterface {
    const WakeWordInterfaceVTable* vtable;
    void* impl_data;
    
    // 配置信息
    audio_codec_t* codec;
    void* user_data;
    
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file wake_word_porcupine.h
 * @brief Mac版本基于Porcupine的唤醒词实现
 * @details 使用Picovoice Porcupine库实现的高精度唤醒词检测
 */

/**
 * @brief Porcupine唤醒词配置结构体
 */
typedef struct {
    char* access_key;           /**< Picovoice访问密钥 */
    char** keyword_paths;       /**< 关键词文件路径数组 */
    int32_t num_keywords;       /**< 关键词数量 */
    float* sensitivities;       /**< 敏感度数组 */
    char* model_path;           /**< 模型文件路径 */
} porcupine_config_t;

/**
 * @brief Porcupine实现数据结构
 */
typedef struct {
    // Porcupine引擎
    pv_porcupine_t* porcupine;
    
    // 配置信息
    porcupine_config_t config;
    
    // 音频处理
    int16_t* audio_buffer;
    size_t buffer_size;
    size_t buffer_pos;
    
    // 线程控制
    pthread_t processing_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool thread_running;
    bool should_stop;
    
    // 检测结果
    char* detected_keywords[10];  // 最多支持10个关键词
    int32_t last_keyword_index;
    
    // Opus编码相关
    uint8_t* opus_encoded_data;
    size_t opus_encoded_size;
    
} PorcupineWakeWordData;

/**
 * @brief 创建Porcupine唤醒词接口实例
 * @param config Porcupine配置
 * @return 成功返回WakeWordInterface指针，失败返回NULL
 */
WakeWordInterface* wake_word_porcupine_create(const porcupine_config_t* config);

/**
 * @brief 设置默认配置
 * @param config 配置结构体指针
 * @param access_key Picovoice访问密钥
 * @return 成功返回0，失败返回负数
 */
int porcupine_config_set_default(porcupine_config_t* config, const char* access_key);

/**
 * @brief 添加关键词
 * @param config 配置结构体指针
 * @param keyword_path 关键词文件路径
 * @param sensitivity 敏感度 (0.0-1.0)
 * @return 成功返回0，失败返回负数
 */
int porcupine_config_add_keyword(porcupine_config_t* config, const char* keyword_path, float sensitivity);

/**
 * @brief 释放配置资源
 * @param config 配置结构体指针
 */
void porcupine_config_destroy(porcupine_config_t* config);

/**
 * @brief 获取默认关键词路径
 * @param keyword_name 关键词名称 (如 "porcupine", "picovoice" 等)
 * @return 关键词文件路径，需要调用者释放
 */
char* porcupine_get_default_keyword_path(const char* keyword_name);

/**
 * @brief 获取默认模型路径
 * @return 模型文件路径，需要调用者释放
 */
char* porcupine_get_default_model_path(void);

#ifdef __cplusplus
}
#endif

#endif // WAKE_WORD_PORCUPINE_H