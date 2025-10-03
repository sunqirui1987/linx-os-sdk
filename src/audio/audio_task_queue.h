#ifndef AUDIO_TASK_QUEUE_H
#define AUDIO_TASK_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file audio_task_queue.h
 * @brief 音频任务队列管理
 * @details 提供音频任务的队列操作，支持不同类型的音频处理任务
 */

/**
 * @brief 音频任务类型枚举
 * @details 定义音频服务支持的各种任务类型
 */
typedef enum {
    AUDIO_TASK_NONE = 0,            /**< 无任务 */
    AUDIO_TASK_PLAY_SOUND,          /**< 播放声音任务 */
    AUDIO_TASK_STOP_SOUND,          /**< 停止声音任务 */
    AUDIO_TASK_SET_VOLUME,          /**< 设置音量任务 */
    AUDIO_TASK_RESET_DECODER,       /**< 重置解码器任务 */
    AUDIO_TASK_FLUSH_BUFFER,        /**< 刷新缓冲区任务 */
    AUDIO_TASK_CONFIGURE_CODEC,     /**< 配置编解码器任务 */
    AUDIO_TASK_START_RECORDING,     /**< 开始录音任务 */
    AUDIO_TASK_STOP_RECORDING,      /**< 停止录音任务 */
    AUDIO_TASK_PROCESS_AUDIO,       /**< 音频处理任务 */
    AUDIO_TASK_CLEANUP              /**< 清理任务 */
} AudioTaskType;

/**
 * @brief 音频任务结构体
 * @details 封装音频任务的所有信息，包括任务类型、参数等
 */
typedef struct AudioTask {
    AudioTaskType type;             /**< 任务类型 */
    void* data;                     /**< 任务数据指针 */
    size_t data_size;               /**< 任务数据大小 */
    uint32_t timestamp;             /**< 任务时间戳 */
    int priority;                   /**< 任务优先级（0-10，数值越大优先级越高） */
    
    // 任务执行回调
    int (*execute)(struct AudioTask* task);  /**< 任务执行函数指针 */
    void (*cleanup)(struct AudioTask* task); /**< 任务清理函数指针 */
    
    // 任务状态
    bool is_completed;              /**< 任务是否已完成 */
    int result_code;                /**< 任务执行结果代码 */
} AudioTask;

/**
 * @brief 音频任务队列结构体
 * @details 使用循环队列实现的音频任务队列，支持优先级排序
 */
typedef struct AudioTaskQueue {
    AudioTask** tasks;              /**< 任务指针数组 */
    size_t capacity;                /**< 队列容量 */
    size_t size;                    /**< 当前队列大小 */
    size_t head;                    /**< 队列头索引 */
    size_t tail;                    /**< 队列尾索引 */
    bool priority_enabled;          /**< 是否启用优先级排序 */
} AudioTaskQueue;

// 音频任务操作函数

/**
 * @brief 创建音频任务
 * @param type 任务类型
 * @param data 任务数据指针
 * @param data_size 任务数据大小
 * @return 新创建的任务指针，失败返回NULL
 */
AudioTask* audio_task_create(AudioTaskType type, void* data, size_t data_size);

/**
 * @brief 销毁音频任务
 * @param task 要销毁的任务指针
 */
void audio_task_destroy(AudioTask* task);

/**
 * @brief 设置任务执行回调函数
 * @param task 任务指针
 * @param execute 执行函数指针
 * @param cleanup 清理函数指针
 */
void audio_task_set_callbacks(AudioTask* task, 
                             int (*execute)(AudioTask* task),
                             void (*cleanup)(AudioTask* task));

/**
 * @brief 设置任务优先级
 * @param task 任务指针
 * @param priority 优先级（0-10）
 */
void audio_task_set_priority(AudioTask* task, int priority);

/**
 * @brief 执行音频任务
 * @param task 任务指针
 * @return 0成功，负数失败
 */
int audio_task_execute(AudioTask* task);

/**
 * @brief 获取任务类型名称
 * @param type 任务类型
 * @return 任务类型的字符串描述
 */
const char* audio_task_type_to_string(AudioTaskType type);

// 音频任务队列操作函数

/**
 * @brief 初始化音频任务队列
 * @param queue 队列指针
 * @param capacity 队列容量
 * @param priority_enabled 是否启用优先级排序
 * @return 0成功，负数失败
 */
int audio_task_queue_init(AudioTaskQueue* queue, size_t capacity, bool priority_enabled);

/**
 * @brief 销毁音频任务队列
 * @param queue 队列指针
 * @details 会自动销毁队列中剩余的所有任务
 */
void audio_task_queue_destroy(AudioTaskQueue* queue);

/**
 * @brief 向队列中添加任务
 * @param queue 队列指针
 * @param task 要添加的任务
 * @return true成功，false失败（队列已满）
 */
bool audio_task_queue_push(AudioTaskQueue* queue, AudioTask* task);

/**
 * @brief 从队列中取出任务
 * @param queue 队列指针
 * @return 任务指针，队列为空时返回NULL
 * @details 如果启用优先级，会返回优先级最高的任务
 */
AudioTask* audio_task_queue_pop(AudioTaskQueue* queue);

/**
 * @brief 查看队列头部任务但不移除
 * @param queue 队列指针
 * @return 任务指针，队列为空时返回NULL
 */
AudioTask* audio_task_queue_peek(const AudioTaskQueue* queue);

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return true为空，false不为空
 */
bool audio_task_queue_is_empty(const AudioTaskQueue* queue);

/**
 * @brief 检查队列是否已满
 * @param queue 队列指针
 * @return true已满，false未满
 */
bool audio_task_queue_is_full(const AudioTaskQueue* queue);

/**
 * @brief 获取队列当前大小
 * @param queue 队列指针
 * @return 队列中任务的数量
 */
size_t audio_task_queue_size(const AudioTaskQueue* queue);

/**
 * @brief 获取队列容量
 * @param queue 队列指针
 * @return 队列的最大容量
 */
size_t audio_task_queue_capacity(const AudioTaskQueue* queue);

/**
 * @brief 清空队列
 * @param queue 队列指针
 * @details 会销毁队列中的所有任务
 */
void audio_task_queue_clear(AudioTaskQueue* queue);

/**
 * @brief 根据任务类型查找任务
 * @param queue 队列指针
 * @param type 要查找的任务类型
 * @return 找到的任务指针，未找到返回NULL
 * @details 只查找不移除任务
 */
AudioTask* audio_task_queue_find_by_type(const AudioTaskQueue* queue, AudioTaskType type);

/**
 * @brief 移除指定类型的所有任务
 * @param queue 队列指针
 * @param type 要移除的任务类型
 * @return 移除的任务数量
 */
size_t audio_task_queue_remove_by_type(AudioTaskQueue* queue, AudioTaskType type);

/**
 * @brief 获取队列中指定类型任务的数量
 * @param queue 队列指针
 * @param type 任务类型
 * @return 指定类型任务的数量
 */
size_t audio_task_queue_count_by_type(const AudioTaskQueue* queue, AudioTaskType type);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_TASK_QUEUE_H