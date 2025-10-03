#include "audio_task_queue.h"
#include "../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @file audio_task_queue.c
 * @brief 音频任务队列管理实现
 * @details 提供音频任务的队列操作实现，支持优先级排序和任务管理
 */

#define AUDIO_TASK_QUEUE_TAG "AudioTaskQueue"

// 内部辅助函数声明
static void audio_task_queue_sort_by_priority(AudioTaskQueue* queue);
static int audio_task_compare_priority(const AudioTask* task1, const AudioTask* task2);

// 音频任务操作函数实现

AudioTask* audio_task_create(AudioTaskType type, void* data, size_t data_size) {
    AudioTask* task = (AudioTask*)calloc(1, sizeof(AudioTask));
    if (!task) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "创建音频任务失败：内存分配失败");
        return NULL;
    }
    
    task->type = type;
    task->priority = 5;  // 默认中等优先级
    task->timestamp = 0;
    task->is_completed = false;
    task->result_code = 0;
    task->execute = NULL;
    task->cleanup = NULL;
    
    // 复制任务数据
    if (data && data_size > 0) {
        task->data = malloc(data_size);
        if (!task->data) {
            LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "创建音频任务失败：任务数据分配失败，大小：%zu", data_size);
            free(task);
            return NULL;
        }
        memcpy(task->data, data, data_size);
        task->data_size = data_size;
    } else {
        task->data = NULL;
        task->data_size = 0;
    }
    
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "创建音频任务成功，类型：%s，数据大小：%zu", 
              audio_task_type_to_string(type), data_size);
    return task;
}

void audio_task_destroy(AudioTask* task) {
    if (!task) {
        return;
    }
    
    // 调用清理回调
    if (task->cleanup) {
        task->cleanup(task);
    }
    
    // 释放任务数据
    if (task->data) {
        free(task->data);
        task->data = NULL;
    }
    
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "销毁音频任务完成，类型：%s", 
              audio_task_type_to_string(task->type));
    free(task);
}

void audio_task_set_callbacks(AudioTask* task, 
                             int (*execute)(AudioTask* task),
                             void (*cleanup)(AudioTask* task)) {
    if (!task) {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "设置任务回调失败：无效参数");
        return;
    }
    
    task->execute = execute;
    task->cleanup = cleanup;
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "设置任务回调成功，类型：%s", 
              audio_task_type_to_string(task->type));
}

void audio_task_set_priority(AudioTask* task, int priority) {
    if (!task) {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "设置任务优先级失败：无效参数");
        return;
    }
    
    // 限制优先级范围
    if (priority < 0) priority = 0;
    if (priority > 10) priority = 10;
    
    task->priority = priority;
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "设置任务优先级成功，类型：%s，优先级：%d", 
              audio_task_type_to_string(task->type), priority);
}

int audio_task_execute(AudioTask* task) {
    if (!task) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "执行任务失败：无效参数");
        return -1;
    }
    
    if (task->is_completed) {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "任务已完成，跳过执行，类型：%s", 
                  audio_task_type_to_string(task->type));
        return task->result_code;
    }
    
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "开始执行任务，类型：%s", 
              audio_task_type_to_string(task->type));
    
    int result = 0;
    if (task->execute) {
        result = task->execute(task);
    } else {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "任务没有执行函数，类型：%s", 
                  audio_task_type_to_string(task->type));
    }
    
    task->is_completed = true;
    task->result_code = result;
    
    if (result == 0) {
        LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "任务执行成功，类型：%s", 
                  audio_task_type_to_string(task->type));
    } else {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "任务执行失败，类型：%s，错误代码：%d", 
                  audio_task_type_to_string(task->type), result);
    }
    
    return result;
}

const char* audio_task_type_to_string(AudioTaskType type) {
    switch (type) {
        case AUDIO_TASK_NONE:               return "无任务";
        case AUDIO_TASK_PLAY_SOUND:         return "播放声音";
        case AUDIO_TASK_STOP_SOUND:         return "停止声音";
        case AUDIO_TASK_SET_VOLUME:         return "设置音量";
        case AUDIO_TASK_RESET_DECODER:      return "重置解码器";
        case AUDIO_TASK_FLUSH_BUFFER:       return "刷新缓冲区";
        case AUDIO_TASK_CONFIGURE_CODEC:    return "配置编解码器";
        case AUDIO_TASK_START_RECORDING:    return "开始录音";
        case AUDIO_TASK_STOP_RECORDING:     return "停止录音";
        case AUDIO_TASK_PROCESS_AUDIO:      return "音频处理";
        case AUDIO_TASK_CLEANUP:            return "清理任务";
        default:                            return "未知任务";
    }
}

// 音频任务队列操作函数实现

int audio_task_queue_init(AudioTaskQueue* queue, size_t capacity, bool priority_enabled) {
    if (!queue || capacity == 0) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "初始化任务队列失败：无效参数");
        return -1;
    }
    
    queue->tasks = (AudioTask**)calloc(capacity, sizeof(AudioTask*));
    if (!queue->tasks) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "初始化任务队列失败：内存分配失败，容量：%zu", capacity);
        return -1;
    }
    
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->priority_enabled = priority_enabled;
    
    LINX_LOGI(AUDIO_TASK_QUEUE_TAG, "音频任务队列初始化成功，容量：%zu，优先级排序：%s", 
              capacity, priority_enabled ? "启用" : "禁用");
    return 0;
}

void audio_task_queue_destroy(AudioTaskQueue* queue) {
    if (!queue) {
        return;
    }
    
    if (queue->tasks) {
        // 销毁队列中剩余的所有任务
        size_t destroyed_count = 0;
        while (!audio_task_queue_is_empty(queue)) {
            AudioTask* task = audio_task_queue_pop(queue);
            if (task) {
                audio_task_destroy(task);
                destroyed_count++;
            }
        }
        
        free(queue->tasks);
        queue->tasks = NULL;
        
        LINX_LOGI(AUDIO_TASK_QUEUE_TAG, "销毁了%zu个剩余任务", destroyed_count);
    }
    
    queue->capacity = 0;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->priority_enabled = false;
    
    LINX_LOGI(AUDIO_TASK_QUEUE_TAG, "音频任务队列销毁完成");
}

bool audio_task_queue_push(AudioTaskQueue* queue, AudioTask* task) {
    if (!queue || !task) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "添加任务失败：无效参数");
        return false;
    }
    
    if (audio_task_queue_is_full(queue)) {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "添加任务失败：队列已满，当前大小：%zu", queue->size);
        return false;
    }
    
    queue->tasks[queue->tail] = task;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    
    // 如果启用优先级排序，重新排序队列
    if (queue->priority_enabled && queue->size > 1) {
        audio_task_queue_sort_by_priority(queue);
    }
    
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "任务添加成功，类型：%s，队列大小：%zu/%zu", 
              audio_task_type_to_string(task->type), queue->size, queue->capacity);
    return true;
}

AudioTask* audio_task_queue_pop(AudioTaskQueue* queue) {
    if (!queue) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "取出任务失败：无效参数");
        return NULL;
    }
    
    if (audio_task_queue_is_empty(queue)) {
        return NULL;  // 队列为空，正常情况
    }
    
    AudioTask* task = queue->tasks[queue->head];
    queue->tasks[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    
    LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "任务取出成功，类型：%s，队列大小：%zu/%zu", 
              audio_task_type_to_string(task->type), queue->size, queue->capacity);
    return task;
}

AudioTask* audio_task_queue_peek(const AudioTaskQueue* queue) {
    if (!queue || audio_task_queue_is_empty(queue)) {
        return NULL;
    }
    
    return queue->tasks[queue->head];
}

bool audio_task_queue_is_empty(const AudioTaskQueue* queue) {
    return !queue || queue->size == 0;
}

bool audio_task_queue_is_full(const AudioTaskQueue* queue) {
    return queue && queue->size >= queue->capacity;
}

size_t audio_task_queue_size(const AudioTaskQueue* queue) {
    return queue ? queue->size : 0;
}

size_t audio_task_queue_capacity(const AudioTaskQueue* queue) {
    return queue ? queue->capacity : 0;
}

void audio_task_queue_clear(AudioTaskQueue* queue) {
    if (!queue) {
        LINX_LOGW(AUDIO_TASK_QUEUE_TAG, "清空队列失败：无效参数");
        return;
    }
    
    size_t cleared_count = 0;
    while (!audio_task_queue_is_empty(queue)) {
        AudioTask* task = audio_task_queue_pop(queue);
        if (task) {
            audio_task_destroy(task);
            cleared_count++;
        }
    }
    
    LINX_LOGI(AUDIO_TASK_QUEUE_TAG, "队列清空完成，销毁了%zu个任务", cleared_count);
}

AudioTask* audio_task_queue_find_by_type(const AudioTaskQueue* queue, AudioTaskType type) {
    if (!queue || audio_task_queue_is_empty(queue)) {
        return NULL;
    }
    
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        AudioTask* task = queue->tasks[index];
        if (task && task->type == type) {
            LINX_LOGD(AUDIO_TASK_QUEUE_TAG, "找到指定类型任务：%s", 
                      audio_task_type_to_string(type));
            return task;
        }
    }
    
    return NULL;
}

size_t audio_task_queue_remove_by_type(AudioTaskQueue* queue, AudioTaskType type) {
    if (!queue || audio_task_queue_is_empty(queue)) {
        return 0;
    }
    
    size_t removed_count = 0;
    size_t original_size = queue->size;
    
    // 创建临时数组存储保留的任务
    AudioTask** temp_tasks = (AudioTask**)malloc(queue->capacity * sizeof(AudioTask*));
    if (!temp_tasks) {
        LINX_LOGE(AUDIO_TASK_QUEUE_TAG, "移除任务失败：临时数组分配失败");
        return 0;
    }
    
    size_t temp_count = 0;
    
    // 遍历队列，保留非目标类型的任务
    for (size_t i = 0; i < original_size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        AudioTask* task = queue->tasks[index];
        
        if (task && task->type == type) {
            audio_task_destroy(task);
            removed_count++;
        } else if (task) {
            temp_tasks[temp_count++] = task;
        }
    }
    
    // 重新构建队列
    queue->head = 0;
    queue->tail = temp_count;
    queue->size = temp_count;
    
    for (size_t i = 0; i < temp_count; i++) {
        queue->tasks[i] = temp_tasks[i];
    }
    
    // 清空剩余位置
    for (size_t i = temp_count; i < queue->capacity; i++) {
        queue->tasks[i] = NULL;
    }
    
    free(temp_tasks);
    
    LINX_LOGI(AUDIO_TASK_QUEUE_TAG, "移除指定类型任务完成，类型：%s，移除数量：%zu", 
              audio_task_type_to_string(type), removed_count);
    return removed_count;
}

size_t audio_task_queue_count_by_type(const AudioTaskQueue* queue, AudioTaskType type) {
    if (!queue || audio_task_queue_is_empty(queue)) {
        return 0;
    }
    
    size_t count = 0;
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->capacity;
        AudioTask* task = queue->tasks[index];
        if (task && task->type == type) {
            count++;
        }
    }
    
    return count;
}

// 内部辅助函数实现

static void audio_task_queue_sort_by_priority(AudioTaskQueue* queue) {
    if (!queue || queue->size <= 1) {
        return;
    }
    
    // 简单的冒泡排序，按优先级降序排列
    for (size_t i = 0; i < queue->size - 1; i++) {
        for (size_t j = 0; j < queue->size - 1 - i; j++) {
            size_t index1 = (queue->head + j) % queue->capacity;
            size_t index2 = (queue->head + j + 1) % queue->capacity;
            
            AudioTask* task1 = queue->tasks[index1];
            AudioTask* task2 = queue->tasks[index2];
            
            if (task1 && task2 && audio_task_compare_priority(task1, task2) < 0) {
                // 交换任务
                queue->tasks[index1] = task2;
                queue->tasks[index2] = task1;
            }
        }
    }
}

static int audio_task_compare_priority(const AudioTask* task1, const AudioTask* task2) {
    if (!task1 || !task2) {
        return 0;
    }
    
    // 优先级高的排在前面
    if (task1->priority > task2->priority) {
        return 1;
    } else if (task1->priority < task2->priority) {
        return -1;
    } else {
        // 优先级相同时，按时间戳排序（早的在前）
        if (task1->timestamp < task2->timestamp) {
            return 1;
        } else if (task1->timestamp > task2->timestamp) {
            return -1;
        } else {
            return 0;
        }
    }
}