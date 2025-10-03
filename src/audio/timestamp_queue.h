#ifndef TIMESTAMP_QUEUE_H
#define TIMESTAMP_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file timestamp_queue.h
 * @brief 时间戳队列管理
 * @details 提供时间戳的队列操作，用于音频同步和时序管理
 */

/**
 * @brief 时间戳条目结构体
 * @details 封装时间戳信息，包括时间戳值、序列号等
 */
typedef struct TimestampEntry {
    uint64_t timestamp;             /**< 时间戳值（微秒） */
    uint32_t sequence_number;       /**< 序列号 */
    uint32_t frame_id;              /**< 帧ID */
    int64_t system_time;            /**< 系统时间（微秒） */
    void* user_data;                /**< 用户自定义数据指针 */
    size_t user_data_size;          /**< 用户数据大小 */
} TimestampEntry;

/**
 * @brief 时间戳队列结构体
 * @details 使用循环队列实现的时间戳队列，支持时间排序
 */
typedef struct TimestampQueue {
    TimestampEntry* entries;        /**< 时间戳条目数组 */
    size_t capacity;                /**< 队列容量 */
    size_t size;                    /**< 当前队列大小 */
    size_t head;                    /**< 队列头索引 */
    size_t tail;                    /**< 队列尾索引 */
    bool time_sorted;               /**< 是否按时间排序 */
    uint64_t oldest_timestamp;      /**< 最旧的时间戳 */
    uint64_t newest_timestamp;      /**< 最新的时间戳 */
} TimestampQueue;

// 时间戳条目操作函数

/**
 * @brief 创建时间戳条目
 * @param timestamp 时间戳值
 * @param sequence_number 序列号
 * @param frame_id 帧ID
 * @return 新创建的时间戳条目，失败返回NULL
 */
TimestampEntry* timestamp_entry_create(uint64_t timestamp, uint32_t sequence_number, uint32_t frame_id);

/**
 * @brief 销毁时间戳条目
 * @param entry 要销毁的时间戳条目指针
 */
void timestamp_entry_destroy(TimestampEntry* entry);

/**
 * @brief 复制时间戳条目
 * @param src 源时间戳条目
 * @return 新的时间戳条目副本，失败返回NULL
 */
TimestampEntry* timestamp_entry_copy(const TimestampEntry* src);

/**
 * @brief 设置时间戳条目的用户数据
 * @param entry 时间戳条目指针
 * @param user_data 用户数据指针
 * @param data_size 数据大小
 * @return 0成功，负数失败
 */
int timestamp_entry_set_user_data(TimestampEntry* entry, const void* user_data, size_t data_size);

/**
 * @brief 获取当前系统时间（微秒）
 * @return 当前系统时间戳
 */
int64_t timestamp_get_current_time_us(void);

/**
 * @brief 计算两个时间戳之间的差值
 * @param timestamp1 时间戳1
 * @param timestamp2 时间戳2
 * @return 时间差（微秒），timestamp1 - timestamp2
 */
int64_t timestamp_diff_us(uint64_t timestamp1, uint64_t timestamp2);

// 时间戳队列操作函数

/**
 * @brief 初始化时间戳队列
 * @param queue 队列指针
 * @param capacity 队列容量
 * @param time_sorted 是否按时间排序
 * @return 0成功，负数失败
 */
int timestamp_queue_init(TimestampQueue* queue, size_t capacity, bool time_sorted);

/**
 * @brief 销毁时间戳队列
 * @param queue 队列指针
 * @details 会自动清理队列中的所有条目
 */
void timestamp_queue_destroy(TimestampQueue* queue);

/**
 * @brief 向队列中添加时间戳条目
 * @param queue 队列指针
 * @param timestamp 时间戳值
 * @param sequence_number 序列号
 * @param frame_id 帧ID
 * @return true成功，false失败（队列已满）
 */
bool timestamp_queue_push(TimestampQueue* queue, uint64_t timestamp, uint32_t sequence_number, uint32_t frame_id);

/**
 * @brief 向队列中添加时间戳条目（完整版本）
 * @param queue 队列指针
 * @param entry 时间戳条目
 * @return true成功，false失败（队列已满）
 */
bool timestamp_queue_push_entry(TimestampQueue* queue, const TimestampEntry* entry);

/**
 * @brief 从队列中取出时间戳条目
 * @param queue 队列指针
 * @param entry 输出的时间戳条目
 * @return true成功，false失败（队列为空）
 */
bool timestamp_queue_pop(TimestampQueue* queue, TimestampEntry* entry);

/**
 * @brief 查看队列头部时间戳条目但不移除
 * @param queue 队列指针
 * @param entry 输出的时间戳条目
 * @return true成功，false失败（队列为空）
 */
bool timestamp_queue_peek(const TimestampQueue* queue, TimestampEntry* entry);

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return true为空，false不为空
 */
bool timestamp_queue_is_empty(const TimestampQueue* queue);

/**
 * @brief 检查队列是否已满
 * @param queue 队列指针
 * @return true已满，false未满
 */
bool timestamp_queue_is_full(const TimestampQueue* queue);

/**
 * @brief 获取队列当前大小
 * @param queue 队列指针
 * @return 队列中条目的数量
 */
size_t timestamp_queue_size(const TimestampQueue* queue);

/**
 * @brief 获取队列容量
 * @param queue 队列指针
 * @return 队列的最大容量
 */
size_t timestamp_queue_capacity(const TimestampQueue* queue);

/**
 * @brief 清空队列
 * @param queue 队列指针
 * @details 会清理队列中的所有条目
 */
void timestamp_queue_clear(TimestampQueue* queue);

/**
 * @brief 根据时间戳查找条目
 * @param queue 队列指针
 * @param timestamp 要查找的时间戳
 * @param tolerance 时间容差（微秒）
 * @param entry 输出的时间戳条目
 * @return true找到，false未找到
 */
bool timestamp_queue_find_by_timestamp(const TimestampQueue* queue, uint64_t timestamp, 
                                      uint64_t tolerance, TimestampEntry* entry);

/**
 * @brief 根据序列号查找条目
 * @param queue 队列指针
 * @param sequence_number 要查找的序列号
 * @param entry 输出的时间戳条目
 * @return true找到，false未找到
 */
bool timestamp_queue_find_by_sequence(const TimestampQueue* queue, uint32_t sequence_number, 
                                     TimestampEntry* entry);

/**
 * @brief 根据帧ID查找条目
 * @param queue 队列指针
 * @param frame_id 要查找的帧ID
 * @param entry 输出的时间戳条目
 * @return true找到，false未找到
 */
bool timestamp_queue_find_by_frame_id(const TimestampQueue* queue, uint32_t frame_id, 
                                     TimestampEntry* entry);

/**
 * @brief 移除过期的时间戳条目
 * @param queue 队列指针
 * @param current_time 当前时间戳
 * @param max_age 最大年龄（微秒）
 * @return 移除的条目数量
 */
size_t timestamp_queue_remove_expired(TimestampQueue* queue, uint64_t current_time, uint64_t max_age);

/**
 * @brief 获取队列中最旧的时间戳
 * @param queue 队列指针
 * @return 最旧的时间戳，队列为空时返回0
 */
uint64_t timestamp_queue_get_oldest(const TimestampQueue* queue);

/**
 * @brief 获取队列中最新的时间戳
 * @param queue 队列指针
 * @return 最新的时间戳，队列为空时返回0
 */
uint64_t timestamp_queue_get_newest(const TimestampQueue* queue);

/**
 * @brief 获取队列中时间戳的范围
 * @param queue 队列指针
 * @return 时间戳范围（微秒），队列为空时返回0
 */
uint64_t timestamp_queue_get_time_range(const TimestampQueue* queue);

/**
 * @brief 计算队列中时间戳的平均间隔
 * @param queue 队列指针
 * @return 平均时间间隔（微秒），队列大小小于2时返回0
 */
uint64_t timestamp_queue_get_average_interval(const TimestampQueue* queue);

/**
 * @brief 检查队列中的时间戳是否连续
 * @param queue 队列指针
 * @param max_gap 最大允许间隔（微秒）
 * @return true连续，false不连续
 */
bool timestamp_queue_is_continuous(const TimestampQueue* queue, uint64_t max_gap);

#ifdef __cplusplus
}
#endif

#endif // TIMESTAMP_QUEUE_H