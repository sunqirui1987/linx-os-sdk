#ifndef LINX_AUDIO_EVENT_BUS_H
#define LINX_AUDIO_EVENT_BUS_H

#include "types.h"
#include "../../common/std/vector.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file event_bus.h
 * @brief LinxOS音频系统事件总线
 * @details 提供音频系统内部的事件发布、订阅和分发机制
 */

// ============================================================================
// 事件订阅者定义
// ============================================================================

/**
 * @brief 事件订阅者结构体
 */
typedef struct {
    uint32_t subscriber_id;                     /**< 订阅者ID */
    linx_audio_event_type_t event_type;        /**< 订阅的事件类型 */
    linx_audio_event_callback_t callback;      /**< 回调函数 */
    void* user_data;                           /**< 用户数据 */
    bool is_active;                            /**< 是否活跃 */
    uint64_t subscribe_time;                   /**< 订阅时间 */
} event_subscriber_t;

// 声明vector类型
VECTOR_DECLARE(event_subscriber_t)
VECTOR_DECLARE(linx_audio_event_t)

/**
 * @brief 事件总线结构体
 */
struct linx_event_bus {
    bool initialized;                          /**< 是否已初始化 */
    pthread_mutex_t mutex;                     /**< 互斥锁 */
    pthread_cond_t condition;                  /**< 条件变量 */
    
    vector_event_subscriber_t_t subscribers;   /**< 订阅者列表 */
    uint32_t next_subscriber_id;               /**< 下一个订阅者ID */
    
    // 事件队列
    vector_linx_audio_event_t_t event_queue;   /**< 事件队列 */
    bool async_mode;                           /**< 异步模式 */
    pthread_t worker_thread;                   /**< 工作线程 */
    bool thread_running;                       /**< 线程运行状态 */
    
    // 统计信息
    uint64_t events_published;                 /**< 已发布事件数 */
    uint64_t events_processed;                 /**< 已处理事件数 */
    uint64_t events_dropped;                   /**< 丢弃事件数 */
};

// ============================================================================
// 事件总线管理函数
// ============================================================================

/**
 * @brief 创建事件总线
 * @return 事件总线实例，失败返回NULL
 */
linx_event_bus_t* event_bus_create(void);

/**
 * @brief 初始化事件总线
 * @param bus 事件总线实例
 * @param async_mode 是否启用异步模式
 * @return 错误码
 */
linx_audio_error_t event_bus_init(linx_event_bus_t* bus, bool async_mode);

/**
 * @brief 销毁事件总线
 * @param bus 事件总线实例
 */
void event_bus_destroy(linx_event_bus_t* bus);

/**
 * @brief 启动事件总线
 * @param bus 事件总线实例
 * @return 错误码
 */
linx_audio_error_t event_bus_start(linx_event_bus_t* bus);

/**
 * @brief 停止事件总线
 * @param bus 事件总线实例
 * @return 错误码
 */
linx_audio_error_t event_bus_stop(linx_event_bus_t* bus);

// ============================================================================
// 事件订阅管理函数
// ============================================================================

/**
 * @brief 订阅事件
 * @param bus 事件总线实例
 * @param event_type 事件类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 订阅者ID，失败返回0
 */
uint32_t event_bus_subscribe(linx_event_bus_t* bus,
                            linx_audio_event_type_t event_type,
                            linx_audio_event_callback_t callback,
                            void* user_data);

/**
 * @brief 取消订阅事件
 * @param bus 事件总线实例
 * @param subscriber_id 订阅者ID
 * @return 错误码
 */
linx_audio_error_t event_bus_unsubscribe(linx_event_bus_t* bus, uint32_t subscriber_id);

/**
 * @brief 取消所有订阅
 * @param bus 事件总线实例
 * @param event_type 事件类型，如果为LINX_AUDIO_EVENT_UNKNOWN则取消所有类型
 * @return 错误码
 */
linx_audio_error_t event_bus_unsubscribe_all(linx_event_bus_t* bus, 
                                             linx_audio_event_type_t event_type);

/**
 * @brief 获取订阅者数量
 * @param bus 事件总线实例
 * @param event_type 事件类型，如果为LINX_AUDIO_EVENT_UNKNOWN则返回总数
 * @return 订阅者数量
 */
size_t event_bus_get_subscriber_count(linx_event_bus_t* bus, 
                                      linx_audio_event_type_t event_type);

// ============================================================================
// 事件发布函数
// ============================================================================

/**
 * @brief 发布事件
 * @param bus 事件总线实例
 * @param event 事件数据
 * @return 错误码
 */
linx_audio_error_t event_bus_publish(linx_event_bus_t* bus, 
                                     const linx_audio_event_t* event);

/**
 * @brief 发布简单事件
 * @param bus 事件总线实例
 * @param event_type 事件类型
 * @param source_id 事件源ID
 * @param data 事件数据
 * @param data_size 数据大小
 * @return 错误码
 */
linx_audio_error_t event_bus_publish_simple(linx_event_bus_t* bus,
                                            linx_audio_event_type_t event_type,
                                            uint32_t source_id,
                                            void* data,
                                            size_t data_size);

/**
 * @brief 同步发布事件（立即处理）
 * @param bus 事件总线实例
 * @param event 事件数据
 * @return 错误码
 */
linx_audio_error_t event_bus_publish_sync(linx_event_bus_t* bus, 
                                          const linx_audio_event_t* event);

// ============================================================================
// 事件处理函数
// ============================================================================

/**
 * @brief 处理事件队列（手动模式）
 * @param bus 事件总线实例
 * @param max_events 最大处理事件数，0表示处理所有
 * @return 实际处理的事件数
 */
size_t event_bus_process_events(linx_event_bus_t* bus, size_t max_events);

/**
 * @brief 清空事件队列
 * @param bus 事件总线实例
 * @return 错误码
 */
linx_audio_error_t event_bus_clear_events(linx_event_bus_t* bus);

/**
 * @brief 获取事件队列大小
 * @param bus 事件总线实例
 * @return 队列中的事件数量
 */
size_t event_bus_get_queue_size(linx_event_bus_t* bus);

// ============================================================================
// 统计和调试函数
// ============================================================================

/**
 * @brief 获取事件总线统计信息
 * @param bus 事件总线实例
 * @param published 已发布事件数（输出参数）
 * @param processed 已处理事件数（输出参数）
 * @param dropped 丢弃事件数（输出参数）
 * @return 错误码
 */
linx_audio_error_t event_bus_get_stats(linx_event_bus_t* bus,
                                       uint64_t* published,
                                       uint64_t* processed,
                                       uint64_t* dropped);

/**
 * @brief 重置统计信息
 * @param bus 事件总线实例
 * @return 错误码
 */
linx_audio_error_t event_bus_reset_stats(linx_event_bus_t* bus);

/**
 * @brief 检查事件总线是否健康
 * @param bus 事件总线实例
 * @return true表示健康，false表示有问题
 */
bool event_bus_is_healthy(linx_event_bus_t* bus);

/**
 * @brief 获取事件总线状态信息
 * @param bus 事件总线实例
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 错误码
 */
linx_audio_error_t event_bus_get_status(linx_event_bus_t* bus, 
                                        char* buffer, 
                                        size_t buffer_size);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 获取事件类型名称
 * @param event_type 事件类型
 * @return 事件类型名称字符串
 */
const char* event_bus_get_event_type_name(linx_audio_event_type_t event_type);

/**
 * @brief 创建事件
 * @param event_type 事件类型
 * @param source_id 事件源ID
 * @param data 事件数据
 * @param data_size 数据大小
 * @return 创建的事件，需要调用者释放data内存
 */
linx_audio_event_t event_bus_create_event(linx_audio_event_type_t event_type,
                                          uint32_t source_id,
                                          const void* data,
                                          size_t data_size);

/**
 * @brief 复制事件
 * @param src 源事件
 * @return 复制的事件，需要调用者释放data内存
 */
linx_audio_event_t event_bus_copy_event(const linx_audio_event_t* src);

/**
 * @brief 释放事件数据
 * @param event 事件
 */
void event_bus_free_event(linx_audio_event_t* event);

/**
 * @brief 获取当前时间戳（微秒）
 * @return 时间戳
 */
uint64_t event_bus_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* LINX_AUDIO_EVENT_BUS_H */