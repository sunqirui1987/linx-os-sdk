#include "timestamp_queue.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/**
 * @file timestamp_queue.c
 * @brief 时间戳队列管理实现
 * @details 提供时间戳的队列操作实现，支持时间排序和同步管理
 */

#define TIMESTAMP_QUEUE_TAG "TimestampQueue"

// 内部辅助函数声明
static void timestamp_queue_sort_by_time(TimestampQueue* queue);
static int timestamp_entry_compare_time(const TimestampEntry* entry1, const TimestampEntry* entry2);
static void timestamp_queue_update_time_range(TimestampQueue* queue);

// 时间戳条目操作函数实现

TimestampEntry* timestamp_entry_create(uint64_t timestamp, uint32_t sequence_number, uint32_t frame_id) {
    TimestampEntry* entry = (TimestampEntry*)calloc(1, sizeof(TimestampEntry));
    if (!entry) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "创建时间戳条目失败：内存分配失败");
        return NULL;
    }
    
    entry->timestamp = timestamp;
    entry->sequence_number = sequence_number;
    entry->frame_id = frame_id;
    entry->system_time = timestamp_get_current_time_us();
    entry->user_data = NULL;
    entry->user_data_size = 0;
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "创建时间戳条目成功，时间戳：%llu，序列号：%u，帧ID：%u", 
              (unsigned long long)timestamp, sequence_number, frame_id);
    return entry;
}

void timestamp_entry_destroy(TimestampEntry* entry) {
    if (!entry) {
        return;
    }
    
    if (entry->user_data) {
        free(entry->user_data);
        entry->user_data = NULL;
    }
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "销毁时间戳条目完成，序列号：%u", entry->sequence_number);
    free(entry);
}

TimestampEntry* timestamp_entry_copy(const TimestampEntry* src) {
    if (!src) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "复制时间戳条目失败：源条目为空");
        return NULL;
    }
    
    TimestampEntry* copy = timestamp_entry_create(src->timestamp, src->sequence_number, src->frame_id);
    if (!copy) {
        return NULL;
    }
    
    copy->system_time = src->system_time;
    
    // 复制用户数据
    if (src->user_data && src->user_data_size > 0) {
        if (timestamp_entry_set_user_data(copy, src->user_data, src->user_data_size) != 0) {
            timestamp_entry_destroy(copy);
            return NULL;
        }
    }
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "复制时间戳条目成功，序列号：%u", src->sequence_number);
    return copy;
}

int timestamp_entry_set_user_data(TimestampEntry* entry, const void* user_data, size_t data_size) {
    if (!entry) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "设置用户数据失败：条目为空");
        return -1;
    }
    
    // 释放旧的用户数据
    if (entry->user_data) {
        free(entry->user_data);
        entry->user_data = NULL;
        entry->user_data_size = 0;
    }
    
    // 设置新的用户数据
    if (user_data && data_size > 0) {
        entry->user_data = malloc(data_size);
        if (!entry->user_data) {
            LINX_LOGE(TIMESTAMP_QUEUE_TAG, "设置用户数据失败：内存分配失败，大小：%zu", data_size);
            return -1;
        }
        memcpy(entry->user_data, user_data, data_size);
        entry->user_data_size = data_size;
    }
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "设置用户数据成功，大小：%zu", data_size);
    return 0;
}

int64_t timestamp_get_current_time_us(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "获取当前时间失败");
        return 0;
    }
    return (int64_t)tv.tv_sec * 1000000LL + (int64_t)tv.tv_usec;
}

int64_t timestamp_diff_us(uint64_t timestamp1, uint64_t timestamp2) {
    return (int64_t)timestamp1 - (int64_t)timestamp2;
}

// 时间戳队列操作函数实现

int timestamp_queue_init(TimestampQueue* queue, size_t capacity, bool time_sorted) {
    if (!queue || capacity == 0) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "初始化时间戳队列失败：无效参数");
        return -1;
    }
    
    queue->entries = (TimestampEntry*)calloc(capacity, sizeof(TimestampEntry));
    if (!queue->entries) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "初始化时间戳队列失败：内存分配失败，容量：%zu", capacity);
        return -1;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->time_sorted = time_sorted;
    queue->oldest_timestamp = 0;
    queue->newest_timestamp = 0;
    
    LINX_LOGI(TIMESTAMP_QUEUE_TAG, "时间戳队列初始化成功，容量：%zu，时间排序：%s", 
              capacity, time_sorted ? "启用" : "禁用");
    return 0;
}

void timestamp_queue_destroy(TimestampQueue* queue) {
    if (!queue) {
        return;
    }
    
    if (queue->entries) {
        // 清理队列中的所有条目
        timestamp_queue_clear(queue);
        free(queue->entries);
        queue->entries = NULL;
    }
    
    queue->capacity = 0;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->time_sorted = false;
    queue->oldest_timestamp = 0;
    queue->newest_timestamp = 0;
    
    LINX_LOGI(TIMESTAMP_QUEUE_TAG, "时间戳队列销毁完成");
}

bool timestamp_queue_push(TimestampQueue* queue, uint64_t timestamp, uint32_t sequence_number, uint32_t frame_id) {
    if (!queue) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "添加时间戳失败：队列为空");
        return false;
    }
    
    if (timestamp_queue_is_full(queue)) {
        LINX_LOGW(TIMESTAMP_QUEUE_TAG, "添加时间戳失败：队列已满，当前大小：%zu", queue->size);
        return false;
    }
    
    TimestampEntry* entry = &queue->entries[queue->tail];
    entry->timestamp = timestamp;
    entry->sequence_number = sequence_number;
    entry->frame_id = frame_id;
    entry->system_time = timestamp_get_current_time_us();
    
    // 清理旧的用户数据
    if (entry->user_data) {
        free(entry->user_data);
        entry->user_data = NULL;
        entry->user_data_size = 0;
    }
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    // 更新时间范围
    timestamp_queue_update_time_range(queue);
    
    // 如果启用时间排序，重新排序队列
    if (queue->time_sorted && queue->size > 1) {
        timestamp_queue_sort_by_time(queue);
    }
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "时间戳添加成功，时间戳：%llu，序列号：%u，队列大小：%zu/%zu", 
              (unsigned long long)timestamp, sequence_number, queue->size, queue->capacity);
    return true;
}

bool timestamp_queue_push_entry(TimestampQueue* queue, const TimestampEntry* entry) {
    if (!queue || !entry) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "添加时间戳条目失败：无效参数");
        return false;
    }
    
    if (timestamp_queue_is_full(queue)) {
        LINX_LOGW(TIMESTAMP_QUEUE_TAG, "添加时间戳条目失败：队列已满，当前大小：%zu", queue->size);
        return false;
    }
    
    TimestampEntry* target = &queue->entries[queue->tail];
    
    // 复制条目数据
    target->timestamp = entry->timestamp;
    target->sequence_number = entry->sequence_number;
    target->frame_id = entry->frame_id;
    target->system_time = entry->system_time;
    
    // 清理旧的用户数据
    if (target->user_data) {
        free(target->user_data);
        target->user_data = NULL;
        target->user_data_size = 0;
    }
    
    // 复制用户数据
    if (entry->user_data && entry->user_data_size > 0) {
        target->user_data = malloc(entry->user_data_size);
        if (target->user_data) {
            memcpy(target->user_data, entry->user_data, entry->user_data_size);
            target->user_data_size = entry->user_data_size;
        } else {
            LINX_LOGW(TIMESTAMP_QUEUE_TAG, "复制用户数据失败：内存分配失败");
        }
    }
    
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    // 更新时间范围
    timestamp_queue_update_time_range(queue);
    
    // 如果启用时间排序，重新排序队列
    if (queue->time_sorted && queue->size > 1) {
        timestamp_queue_sort_by_time(queue);
    }
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "时间戳条目添加成功，序列号：%u，队列大小：%zu/%zu", 
              entry->sequence_number, queue->size, queue->capacity);
    return true;
}

bool timestamp_queue_pop(TimestampQueue* queue, TimestampEntry* entry) {
    if (!queue || !entry) {
        LINX_LOGE(TIMESTAMP_QUEUE_TAG, "取出时间戳失败：无效参数");
        return false;
    }
    
    if (timestamp_queue_is_empty(queue)) {
        return false;  // 队列为空，正常情况
    }
    
    TimestampEntry* source = &queue->entries[queue->head];
    
    // 复制条目数据
    entry->timestamp = source->timestamp;
    entry->sequence_number = source->sequence_number;
    entry->frame_id = source->frame_id;
    entry->system_time = source->system_time;
    
    // 复制用户数据
    entry->user_data = NULL;
    entry->user_data_size = 0;
    if (source->user_data && source->user_data_size > 0) {
        entry->user_data = malloc(source->user_data_size);
        if (entry->user_data) {
            memcpy(entry->user_data, source->user_data, source->user_data_size);
            entry->user_data_size = source->user_data_size;
        }
    }
    
    // 清理源条目
    if (source->user_data) {
        free(source->user_data);
        source->user_data = NULL;
        source->user_data_size = 0;
    }
    
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    // 更新时间范围
    timestamp_queue_update_time_range(queue);
    
    LINX_LOGD(TIMESTAMP_QUEUE_TAG, "时间戳取出成功，序列号：%u，队列大小：%zu/%zu", 
              entry->sequence_number, queue->size, queue->capacity);
    return true;
}

bool timestamp_queue_peek(const TimestampQueue* queue, TimestampEntry* entry) {
    if (!queue || !entry || timestamp_queue_is_empty(queue)) {
        return false;
    }
    
    const TimestampEntry* source = &queue->entries[queue->head];
    
    // 复制条目数据（不包括用户数据指针）
    entry->timestamp = source->timestamp;
    entry->sequence_number = source->sequence_number;
    entry->frame_id = source->frame_id;
    entry->system_time = source->system_time;
    entry->user_data = source->user_data;  // 只复制指针，不复制数据
    entry->user_data_size = source->user_data_size;
    
    return true;
}

bool timestamp_queue_is_empty(const TimestampQueue* queue) {
    return !queue || queue->size == 0;
}

bool timestamp_queue_is_full(const TimestampQueue* queue) {
    return queue && queue->size >= queue->capacity;
}

size_t timestamp_queue_size(const TimestampQueue* queue) {
    return queue ? queue->size : 0;
}

size_t timestamp_queue_capacity(const TimestampQueue* queue) {
    return queue ? queue->capacity : 0;
}

void timestamp_queue_clear(TimestampQueue* queue) {
    if (!queue) {
        LINX_LOGW(TIMESTAMP_QUEUE_TAG, "清空队列失败：无效参数");
        return;
    }
    
    size_t cleared_count = 0;
    
    // 清理所有条目的用户数据
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        TimestampEntry* entry = &queue->entries[index];
        if (entry->user_data) {
            free(entry->user_data);
            entry->user_data = NULL;
            entry->user_data_size = 0;
        }
        cleared_count++;
    }
    
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->oldest_timestamp = 0;
    queue->newest_timestamp = 0;
    
    LINX_LOGI(TIMESTAMP_QUEUE_TAG, "队列清空完成，清理了%zu个条目", cleared_count);
}

bool timestamp_queue_find_by_timestamp(const TimestampQueue* queue, uint64_t timestamp, 
                                      uint64_t tolerance, TimestampEntry* entry) {
    if (!queue || !entry || timestamp_queue_is_empty(queue)) {
        return false;
    }
    
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        const TimestampEntry* current = &queue->entries[index];
        
        uint64_t diff = (current->timestamp > timestamp) ? 
                       (current->timestamp - timestamp) : 
                       (timestamp - current->timestamp);
        
        if (diff <= tolerance) {
            // 找到匹配的条目
            entry->timestamp = current->timestamp;
            entry->sequence_number = current->sequence_number;
            entry->frame_id = current->frame_id;
            entry->system_time = current->system_time;
            entry->user_data = current->user_data;
            entry->user_data_size = current->user_data_size;
            
            LINX_LOGD(TIMESTAMP_QUEUE_TAG, "根据时间戳找到条目，时间戳：%llu，序列号：%u", 
                      (unsigned long long)current->timestamp, current->sequence_number);
            return true;
        }
    }
    
    return false;
}

bool timestamp_queue_find_by_sequence(const TimestampQueue* queue, uint32_t sequence_number, 
                                     TimestampEntry* entry) {
    if (!queue || !entry || timestamp_queue_is_empty(queue)) {
        return false;
    }
    
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        const TimestampEntry* current = &queue->entries[index];
        
        if (current->sequence_number == sequence_number) {
            // 找到匹配的条目
            entry->timestamp = current->timestamp;
            entry->sequence_number = current->sequence_number;
            entry->frame_id = current->frame_id;
            entry->system_time = current->system_time;
            entry->user_data = current->user_data;
            entry->user_data_size = current->user_data_size;
            
            LINX_LOGD(TIMESTAMP_QUEUE_TAG, "根据序列号找到条目，序列号：%u，时间戳：%llu", 
                      sequence_number, (unsigned long long)current->timestamp);
            return true;
        }
    }
    
    return false;
}

bool timestamp_queue_find_by_frame_id(const TimestampQueue* queue, uint32_t frame_id, 
                                     TimestampEntry* entry) {
    if (!queue || !entry || timestamp_queue_is_empty(queue)) {
        return false;
    }
    
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        const TimestampEntry* current = &queue->entries[index];
        
        if (current->frame_id == frame_id) {
            // 找到匹配的条目
            entry->timestamp = current->timestamp;
            entry->sequence_number = current->sequence_number;
            entry->frame_id = current->frame_id;
            entry->system_time = current->system_time;
            entry->user_data = current->user_data;
            entry->user_data_size = current->user_data_size;
            
            LINX_LOGD(TIMESTAMP_QUEUE_TAG, "根据帧ID找到条目，帧ID：%u，时间戳：%llu", 
                      frame_id, (unsigned long long)current->timestamp);
            return true;
        }
    }
    
    return false;
}

size_t timestamp_queue_remove_expired(TimestampQueue* queue, uint64_t current_time, uint64_t max_age) {
    if (!queue || timestamp_queue_is_empty(queue)) {
        return 0;
    }
    
    size_t removed_count = 0;
    uint64_t expire_time = current_time - max_age;
    
    // 从队列头开始移除过期条目
    while (!timestamp_queue_is_empty(queue)) {
        const TimestampEntry* head_entry = &queue->entries[queue->head];
        
        if (head_entry->timestamp >= expire_time) {
            break;  // 头部条目未过期，停止移除
        }
        
        // 移除过期条目
        TimestampEntry temp_entry;
        if (timestamp_queue_pop(queue, &temp_entry)) {
            if (temp_entry.user_data) {
                free(temp_entry.user_data);
            }
            removed_count++;
        }
    }
    
    if (removed_count > 0) {
        LINX_LOGI(TIMESTAMP_QUEUE_TAG, "移除过期条目完成，移除数量：%zu，当前队列大小：%zu", 
                  removed_count, queue->size);
    }
    
    return removed_count;
}

uint64_t timestamp_queue_get_oldest(const TimestampQueue* queue) {
    return (queue && !timestamp_queue_is_empty(queue)) ? queue->oldest_timestamp : 0;
}

uint64_t timestamp_queue_get_newest(const TimestampQueue* queue) {
    return (queue && !timestamp_queue_is_empty(queue)) ? queue->newest_timestamp : 0;
}

uint64_t timestamp_queue_get_time_range(const TimestampQueue* queue) {
    if (!queue || timestamp_queue_is_empty(queue)) {
        return 0;
    }
    return queue->newest_timestamp - queue->oldest_timestamp;
}

uint64_t timestamp_queue_get_average_interval(const TimestampQueue* queue) {
    if (!queue || queue->size < 2) {
        return 0;
    }
    
    uint64_t total_interval = 0;
    size_t interval_count = 0;
    
    for (size_t i = 1; i < queue->size; i++) {
        size_t prev_index = (queue->head + i - 1) % queue->capacity;
        size_t curr_index = (queue->head + i) % queue->capacity;
        
        const TimestampEntry* prev_entry = &queue->entries[prev_index];
        const TimestampEntry* curr_entry = &queue->entries[curr_index];
        
        if (curr_entry->timestamp > prev_entry->timestamp) {
            total_interval += curr_entry->timestamp - prev_entry->timestamp;
            interval_count++;
        }
    }
    
    return interval_count > 0 ? total_interval / interval_count : 0;
}

bool timestamp_queue_is_continuous(const TimestampQueue* queue, uint64_t max_gap) {
    if (!queue || queue->size < 2) {
        return true;  // 空队列或单个条目认为是连续的
    }
    
    for (size_t i = 1; i < queue->size; i++) {
        size_t prev_index = (queue->head + i - 1) % queue->capacity;
        size_t curr_index = (queue->head + i) % queue->capacity;
        
        const TimestampEntry* prev_entry = &queue->entries[prev_index];
        const TimestampEntry* curr_entry = &queue->entries[curr_index];
        
        if (curr_entry->timestamp > prev_entry->timestamp) {
            uint64_t gap = curr_entry->timestamp - prev_entry->timestamp;
            if (gap > max_gap) {
                LINX_LOGD(TIMESTAMP_QUEUE_TAG, "发现时间戳间隔过大：%llu > %llu", 
                          (unsigned long long)gap, (unsigned long long)max_gap);
                return false;
            }
        }
    }
    
    return true;
}

// 内部辅助函数实现

static void timestamp_queue_sort_by_time(TimestampQueue* queue) {
    if (!queue || queue->size <= 1) {
        return;
    }
    
    // 简单的冒泡排序，按时间戳升序排列
    for (size_t i = 0; i < queue->size - 1; i++) {
        for (size_t j = 0; j < queue->size - 1 - i; j++) {
            size_t index1 = (queue->head + j) % queue->capacity;
            size_t index2 = (queue->head + j + 1) % queue->capacity;
            
            TimestampEntry* entry1 = &queue->entries[index1];
            TimestampEntry* entry2 = &queue->entries[index2];
            
            if (timestamp_entry_compare_time(entry1, entry2) > 0) {
                // 交换条目
                TimestampEntry temp = *entry1;
                *entry1 = *entry2;
                *entry2 = temp;
            }
        }
    }
}

static int timestamp_entry_compare_time(const TimestampEntry* entry1, const TimestampEntry* entry2) {
    if (!entry1 || !entry2) {
        return 0;
    }
    
    if (entry1->timestamp < entry2->timestamp) {
        return -1;
    } else if (entry1->timestamp > entry2->timestamp) {
        return 1;
    } else {
        // 时间戳相同时，按序列号排序
        if (entry1->sequence_number < entry2->sequence_number) {
            return -1;
        } else if (entry1->sequence_number > entry2->sequence_number) {
            return 1;
        } else {
            return 0;
        }
    }
}

static void timestamp_queue_update_time_range(TimestampQueue* queue) {
    if (!queue || timestamp_queue_is_empty(queue)) {
        queue->oldest_timestamp = 0;
        queue->newest_timestamp = 0;
        return;
    }
    
    uint64_t oldest = UINT64_MAX;
    uint64_t newest = 0;
    
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        const TimestampEntry* entry = &queue->entries[index];
        
        if (entry->timestamp < oldest) {
            oldest = entry->timestamp;
        }
        if (entry->timestamp > newest) {
            newest = entry->timestamp;
        }
    }
    
    queue->oldest_timestamp = oldest;
    queue->newest_timestamp = newest;
}