#include "event_bus.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

/**
 * @file event_bus.c
 * @brief LinxOS音频系统事件总线实现
 * @details 实现音频系统内部的事件发布、订阅和分发机制
 */

#define TAG "EventBus"

// ============================================================================
// 内部常量定义
// ============================================================================

#define EVENT_BUS_MAX_QUEUE_SIZE        1000    /**< 最大事件队列大小 */
#define EVENT_BUS_THREAD_TIMEOUT_MS     100     /**< 线程超时时间（毫秒） */
#define EVENT_BUS_STATS_RESET_THRESHOLD 1000000 /**< 统计重置阈值 */

// ============================================================================
// 内部函数声明
// ============================================================================

static void* event_bus_worker_thread(void* arg);
static linx_audio_error_t event_bus_dispatch_event(linx_event_bus_t* bus, 
                                                   const linx_audio_event_t* event);
static bool event_bus_should_deliver_event(const event_subscriber_t* subscriber,
                                           const linx_audio_event_t* event);
static void event_bus_cleanup_subscriber(event_subscriber_t* subscriber);
static linx_audio_error_t event_bus_validate_params(linx_event_bus_t* bus);

// ============================================================================
// 事件总线管理函数实现
// ============================================================================

linx_event_bus_t* event_bus_create(void)
{
    LINX_LOGI(TAG, "Creating event bus");
    
    linx_event_bus_t* bus = (linx_event_bus_t*)calloc(1, sizeof(linx_event_bus_t));
    if (!bus) {
        LINX_LOGE(TAG, "Failed to allocate memory for event bus");
        return NULL;
    }
    
    // 初始化基本状态
    bus->initialized = false;
    bus->async_mode = false;
    bus->thread_running = false;
    bus->next_subscriber_id = 1;
    bus->events_published = 0;
    bus->events_processed = 0;
    bus->events_dropped = 0;
    
    LINX_LOGI(TAG, "Event bus created successfully");
    return bus;
}

linx_audio_error_t event_bus_init(linx_event_bus_t* bus, bool async_mode)
{
    if (!bus) {
        LINX_LOGE(TAG, "Invalid bus parameter");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (bus->initialized) {
        LINX_LOGW(TAG, "Event bus already initialized");
        return LINX_AUDIO_ERROR_ALREADY_INITIALIZED;
    }
    
    LINX_LOGI(TAG, "Initializing event bus (async_mode=%s)", async_mode ? "true" : "false");
    
    // 初始化互斥锁和条件变量
    int ret = pthread_mutex_init(&bus->mutex, NULL);
    if (ret != 0) {
        LINX_LOGE(TAG, "Failed to initialize mutex: %s", strerror(ret));
        return LINX_AUDIO_ERROR_UNKNOWN;
    }
    
    ret = pthread_cond_init(&bus->condition, NULL);
    if (ret != 0) {
        LINX_LOGE(TAG, "Failed to initialize condition variable: %s", strerror(ret));
        pthread_mutex_destroy(&bus->mutex);
        return LINX_AUDIO_ERROR_UNKNOWN;
    }
    
    // 初始化订阅者列表
    if (vector_event_subscriber_t_init(&bus->subscribers) != 0) {
        LINX_LOGE(TAG, "Failed to initialize subscribers vector");
        pthread_cond_destroy(&bus->condition);
        pthread_mutex_destroy(&bus->mutex);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    // 初始化事件队列
    if (vector_linx_audio_event_t_init(&bus->event_queue) != 0) {
        LINX_LOGE(TAG, "Failed to initialize event queue vector");
        vector_event_subscriber_t_destroy(&bus->subscribers);
        pthread_cond_destroy(&bus->condition);
        pthread_mutex_destroy(&bus->mutex);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    // 预分配队列容量
    if (vector_linx_audio_event_t_reserve(&bus->event_queue, EVENT_BUS_MAX_QUEUE_SIZE) != 0) {
        LINX_LOGW(TAG, "Failed to reserve event queue capacity");
    }
    
    bus->async_mode = async_mode;
    bus->initialized = true;
    
    LINX_LOGI(TAG, "Event bus initialized successfully");
    return LINX_AUDIO_SUCCESS;
}

void event_bus_destroy(linx_event_bus_t* bus)
{
    if (!bus) {
        return;
    }
    
    LINX_LOGI(TAG, "Destroying event bus");
    
    // 停止事件总线
    if (bus->initialized) {
        event_bus_stop(bus);
        
        // 清理订阅者
        pthread_mutex_lock(&bus->mutex);
        
        for (size_t i = 0; i < vector_event_subscriber_t_size(&bus->subscribers); i++) {
            event_subscriber_t* subscriber = vector_event_subscriber_t_at(&bus->subscribers, i);
            if (subscriber) {
                event_bus_cleanup_subscriber(subscriber);
            }
        }
        vector_event_subscriber_t_destroy(&bus->subscribers);
        
        // 清理事件队列
        for (size_t i = 0; i < vector_linx_audio_event_t_size(&bus->event_queue); i++) {
            linx_audio_event_t* event = vector_linx_audio_event_t_at(&bus->event_queue, i);
            if (event) {
                event_bus_free_event(event);
            }
        }
        vector_linx_audio_event_t_destroy(&bus->event_queue);
        
        pthread_mutex_unlock(&bus->mutex);
        
        // 销毁同步对象
        pthread_cond_destroy(&bus->condition);
        pthread_mutex_destroy(&bus->mutex);
    }
    
    // 释放内存
    free(bus);
    
    LINX_LOGI(TAG, "Event bus destroyed");
}

linx_audio_error_t event_bus_start(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        LINX_LOGE(TAG, "Event bus not initialized");
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    if (bus->thread_running) {
        LINX_LOGW(TAG, "Event bus already running");
        pthread_mutex_unlock(&bus->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    if (bus->async_mode) {
        LINX_LOGI(TAG, "Starting event bus worker thread");
        
        bus->thread_running = true;
        int ret = pthread_create(&bus->worker_thread, NULL, event_bus_worker_thread, bus);
        if (ret != 0) {
            LINX_LOGE(TAG, "Failed to create worker thread: %s", strerror(ret));
            bus->thread_running = false;
            pthread_mutex_unlock(&bus->mutex);
            return LINX_AUDIO_ERROR_UNKNOWN;
        }
        
        LINX_LOGI(TAG, "Event bus worker thread started");
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGI(TAG, "Event bus started successfully");
    return LINX_AUDIO_SUCCESS;
}

linx_audio_error_t event_bus_stop(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        LINX_LOGE(TAG, "Event bus not initialized");
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    if (!bus->thread_running) {
        LINX_LOGW(TAG, "Event bus not running");
        pthread_mutex_unlock(&bus->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    LINX_LOGI(TAG, "Stopping event bus");
    
    bus->thread_running = false;
    pthread_cond_broadcast(&bus->condition);
    
    pthread_mutex_unlock(&bus->mutex);
    
    // 等待工作线程结束
    if (bus->async_mode) {
        int ret = pthread_join(bus->worker_thread, NULL);
        if (ret != 0) {
            LINX_LOGE(TAG, "Failed to join worker thread: %s", strerror(ret));
            return LINX_AUDIO_ERROR_UNKNOWN;
        }
        LINX_LOGI(TAG, "Event bus worker thread stopped");
    }
    
    LINX_LOGI(TAG, "Event bus stopped successfully");
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 事件订阅管理函数实现
// ============================================================================

uint32_t event_bus_subscribe(linx_event_bus_t* bus,
                             linx_audio_event_type_t event_type,
                             linx_audio_event_callback_t callback,
                             void* user_data)
{
    if (!bus || !bus->initialized || !callback) {
        LINX_LOGE(TAG, "Invalid parameters for subscription");
        return 0;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    // 检查订阅者数量限制
    if (vector_event_subscriber_t_size(&bus->subscribers) >= LINX_AUDIO_MAX_SUBSCRIBERS) {
        LINX_LOGE(TAG, "Maximum number of subscribers reached");
        pthread_mutex_unlock(&bus->mutex);
        return 0;
    }
    
    // 创建新订阅者
    event_subscriber_t subscriber = {
        .subscriber_id = bus->next_subscriber_id++,
        .event_type = event_type,
        .callback = callback,
        .user_data = user_data,
        .is_active = true,
        .subscribe_time = event_bus_get_timestamp()
    };
    
    // 添加到订阅者列表
    if (vector_event_subscriber_t_push_back(&bus->subscribers, subscriber) != 0) {
        LINX_LOGE(TAG, "Failed to add subscriber to list");
        pthread_mutex_unlock(&bus->mutex);
        return 0;
    }
    
    uint32_t subscriber_id = subscriber.subscriber_id;
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGI(TAG, "Event subscription added: id=%u, type=%d", 
              subscriber_id, event_type);
    
    return subscriber_id;
}

linx_audio_error_t event_bus_unsubscribe(linx_event_bus_t* bus, uint32_t subscriber_id)
{
    if (!bus || !bus->initialized || subscriber_id == 0) {
        LINX_LOGE(TAG, "Invalid parameters for unsubscription");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    bool found = false;
    for (size_t i = 0; i < vector_event_subscriber_t_size(&bus->subscribers); i++) {
        event_subscriber_t* subscriber = vector_event_subscriber_t_at(&bus->subscribers, i);
        if (subscriber && subscriber->subscriber_id == subscriber_id) {
            event_bus_cleanup_subscriber(subscriber);
            vector_event_subscriber_t_erase(&bus->subscribers, i);
            found = true;
            break;
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    if (found) {
        LINX_LOGI(TAG, "Event subscription removed: id=%u", subscriber_id);
        return LINX_AUDIO_SUCCESS;
    } else {
        LINX_LOGW(TAG, "Subscriber not found: id=%u", subscriber_id);
        return LINX_AUDIO_ERROR_NOT_FOUND;
    }
}

linx_audio_error_t event_bus_unsubscribe_all(linx_event_bus_t* bus, 
                                             linx_audio_event_type_t event_type)
{
    if (!bus || !bus->initialized) {
        LINX_LOGE(TAG, "Event bus not initialized");
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t removed_count = 0;
    for (size_t i = 0; i < vector_event_subscriber_t_size(&bus->subscribers); ) {
        event_subscriber_t* subscriber = vector_event_subscriber_t_at(&bus->subscribers, i);
        if (subscriber && (event_type == LINX_AUDIO_EVENT_UNKNOWN || 
                          subscriber->event_type == event_type)) {
            event_bus_cleanup_subscriber(subscriber);
            vector_event_subscriber_t_erase(&bus->subscribers, i);
            removed_count++;
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGI(TAG, "Removed %zu subscribers for event type %d", removed_count, event_type);
    return LINX_AUDIO_SUCCESS;
}

size_t event_bus_get_subscriber_count(linx_event_bus_t* bus, 
                                      linx_audio_event_type_t event_type)
{
    if (!bus || !bus->initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t count = 0;
    for (size_t i = 0; i < vector_event_subscriber_t_size(&bus->subscribers); i++) {
        event_subscriber_t* subscriber = vector_event_subscriber_t_at(&bus->subscribers, i);
        if (subscriber && subscriber->is_active &&
            (event_type == LINX_AUDIO_EVENT_UNKNOWN || subscriber->event_type == event_type)) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    return count;
}

// ============================================================================
// 事件发布函数实现
// ============================================================================

linx_audio_error_t event_bus_publish(linx_event_bus_t* bus, 
                                     const linx_audio_event_t* event)
{
    if (!bus || !bus->initialized || !event) {
        LINX_LOGE(TAG, "Invalid parameters for event publishing");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    bus->events_published++;
    
    if (bus->async_mode && bus->thread_running) {
        // 异步模式：添加到队列
        if (vector_linx_audio_event_t_size(&bus->event_queue) >= EVENT_BUS_MAX_QUEUE_SIZE) {
            LINX_LOGW(TAG, "Event queue full, dropping event");
            bus->events_dropped++;
            pthread_mutex_unlock(&bus->mutex);
            return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
        }
        
        linx_audio_event_t event_copy = event_bus_copy_event(event);
        if (vector_linx_audio_event_t_push_back(&bus->event_queue, event_copy) != 0) {
            LINX_LOGE(TAG, "Failed to add event to queue");
            event_bus_free_event(&event_copy);
            bus->events_dropped++;
            pthread_mutex_unlock(&bus->mutex);
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        
        pthread_cond_signal(&bus->condition);
        pthread_mutex_unlock(&bus->mutex);
    } else {
        // 同步模式：立即分发
        pthread_mutex_unlock(&bus->mutex);
        return event_bus_dispatch_event(bus, event);
    }
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_error_t event_bus_publish_simple(linx_event_bus_t* bus,
                                            linx_audio_event_type_t event_type,
                                            uint32_t source_id,
                                            void* data,
                                            size_t data_size)
{
    linx_audio_event_t event = event_bus_create_event(event_type, source_id, data, data_size);
    linx_audio_error_t result = event_bus_publish(bus, &event);
    event_bus_free_event(&event);
    return result;
}

linx_audio_error_t event_bus_publish_sync(linx_event_bus_t* bus, 
                                          const linx_audio_event_t* event)
{
    if (!bus || !bus->initialized || !event) {
        LINX_LOGE(TAG, "Invalid parameters for sync event publishing");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&bus->mutex);
    bus->events_published++;
    pthread_mutex_unlock(&bus->mutex);
    
    return event_bus_dispatch_event(bus, event);
}

// ============================================================================
// 事件处理函数实现
// ============================================================================

size_t event_bus_process_events(linx_event_bus_t* bus, size_t max_events)
{
    if (!bus || !bus->initialized) {
        return 0;
    }
    
    size_t processed = 0;
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t queue_size = vector_linx_audio_event_t_size(&bus->event_queue);
    size_t events_to_process = (max_events == 0) ? queue_size : 
                              (max_events < queue_size ? max_events : queue_size);
    
    for (size_t i = 0; i < events_to_process; i++) {
        linx_audio_event_t* event = vector_linx_audio_event_t_at(&bus->event_queue, 0);
        if (event) {
            linx_audio_event_t event_copy = *event;
            vector_linx_audio_event_t_erase(&bus->event_queue, 0);
            
            pthread_mutex_unlock(&bus->mutex);
            
            event_bus_dispatch_event(bus, &event_copy);
            event_bus_free_event(&event_copy);
            processed++;
            
            pthread_mutex_lock(&bus->mutex);
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    return processed;
}

linx_audio_error_t event_bus_clear_events(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t cleared_count = vector_linx_audio_event_t_size(&bus->event_queue);
    
    for (size_t i = 0; i < cleared_count; i++) {
        linx_audio_event_t* event = vector_linx_audio_event_t_at(&bus->event_queue, i);
        if (event) {
            event_bus_free_event(event);
        }
    }
    
    vector_linx_audio_event_t_clear(&bus->event_queue);
    bus->events_dropped += cleared_count;
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGI(TAG, "Cleared %zu events from queue", cleared_count);
    return LINX_AUDIO_SUCCESS;
}

size_t event_bus_get_queue_size(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&bus->mutex);
    size_t size = vector_linx_audio_event_t_size(&bus->event_queue);
    pthread_mutex_unlock(&bus->mutex);
    
    return size;
}

// ============================================================================
// 统计和调试函数实现
// ============================================================================

linx_audio_error_t event_bus_get_stats(linx_event_bus_t* bus,
                                       uint64_t* published,
                                       uint64_t* processed,
                                       uint64_t* dropped)
{
    if (!bus || !bus->initialized) {
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    if (published) *published = bus->events_published;
    if (processed) *processed = bus->events_processed;
    if (dropped) *dropped = bus->events_dropped;
    
    pthread_mutex_unlock(&bus->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_error_t event_bus_reset_stats(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    bus->events_published = 0;
    bus->events_processed = 0;
    bus->events_dropped = 0;
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGI(TAG, "Event bus statistics reset");
    return LINX_AUDIO_SUCCESS;
}

bool event_bus_is_healthy(linx_event_bus_t* bus)
{
    if (!bus || !bus->initialized) {
        return false;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    bool healthy = true;
    
    // 检查队列是否过满
    size_t queue_size = vector_linx_audio_event_t_size(&bus->event_queue);
    if (queue_size > EVENT_BUS_MAX_QUEUE_SIZE * 0.9) {
        healthy = false;
    }
    
    // 检查丢弃率
    if (bus->events_published > 0) {
        double drop_rate = (double)bus->events_dropped / bus->events_published;
        if (drop_rate > 0.1) { // 丢弃率超过10%
            healthy = false;
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    return healthy;
}

linx_audio_error_t event_bus_get_status(linx_event_bus_t* bus, 
                                        char* buffer, 
                                        size_t buffer_size)
{
    if (!bus || !buffer || buffer_size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (!bus->initialized) {
        snprintf(buffer, buffer_size, "Event Bus: Not initialized");
        return LINX_AUDIO_SUCCESS;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t queue_size = vector_linx_audio_event_t_size(&bus->event_queue);
    size_t subscriber_count = vector_event_subscriber_t_size(&bus->subscribers);
    
    snprintf(buffer, buffer_size,
             "Event Bus Status:\n"
             "  Mode: %s\n"
             "  Running: %s\n"
             "  Queue Size: %zu/%d\n"
             "  Subscribers: %zu\n"
             "  Published: %llu\n"
             "  Processed: %llu\n"
             "  Dropped: %llu\n"
             "  Health: %s",
             bus->async_mode ? "Async" : "Sync",
             bus->thread_running ? "Yes" : "No",
             queue_size, EVENT_BUS_MAX_QUEUE_SIZE,
             subscriber_count,
             (unsigned long long)bus->events_published,
             (unsigned long long)bus->events_processed,
             (unsigned long long)bus->events_dropped,
             event_bus_is_healthy(bus) ? "Good" : "Poor");
    
    pthread_mutex_unlock(&bus->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 工具函数实现
// ============================================================================

const char* event_bus_get_event_type_name(linx_audio_event_type_t event_type)
{
    switch (event_type) {
        case LINX_AUDIO_EVENT_UNKNOWN: return "UNKNOWN";
        case LINX_AUDIO_EVENT_SYSTEM_INIT: return "SYSTEM_INIT";
        case LINX_AUDIO_EVENT_SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        case LINX_AUDIO_EVENT_DEVICE_ADDED: return "DEVICE_ADDED";
        case LINX_AUDIO_EVENT_DEVICE_REMOVED: return "DEVICE_REMOVED";
        case LINX_AUDIO_EVENT_DEVICE_STATE_CHANGED: return "DEVICE_STATE_CHANGED";
        case LINX_AUDIO_EVENT_STREAM_CREATED: return "STREAM_CREATED";
        case LINX_AUDIO_EVENT_STREAM_DESTROYED: return "STREAM_DESTROYED";
        case LINX_AUDIO_EVENT_STREAM_STATE_CHANGED: return "STREAM_STATE_CHANGED";
        case LINX_AUDIO_EVENT_BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case LINX_AUDIO_EVENT_BUFFER_UNDERFLOW: return "BUFFER_UNDERFLOW";
        case LINX_AUDIO_EVENT_ERROR: return "ERROR";
        default: return "INVALID";
    }
}

linx_audio_event_t event_bus_create_event(linx_audio_event_type_t event_type,
                                          uint32_t source_id,
                                          const void* data,
                                          size_t data_size)
{
    linx_audio_event_t event = {
        .type = event_type,
        .timestamp = event_bus_get_timestamp(),
        .source_id = source_id,
        .data = NULL,
        .data_size = data_size
    };
    
    if (data && data_size > 0) {
        event.data = malloc(data_size);
        if (event.data) {
            memcpy(event.data, data, data_size);
        } else {
            event.data_size = 0;
        }
    }
    
    return event;
}

linx_audio_event_t event_bus_copy_event(const linx_audio_event_t* src)
{
    if (!src) {
        linx_audio_event_t empty = {0};
        return empty;
    }
    
    return event_bus_create_event(src->type, src->source_id, src->data, src->data_size);
}

void event_bus_free_event(linx_audio_event_t* event)
{
    if (event && event->data) {
        free(event->data);
        event->data = NULL;
        event->data_size = 0;
    }
}

uint64_t event_bus_get_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// ============================================================================
// 内部函数实现
// ============================================================================

static void* event_bus_worker_thread(void* arg)
{
    linx_event_bus_t* bus = (linx_event_bus_t*)arg;
    if (!bus) {
        LINX_LOGE(TAG, "Invalid bus parameter in worker thread");
        return NULL;
    }
    
    LINX_LOGI(TAG, "Event bus worker thread started");
    
    while (true) {
        pthread_mutex_lock(&bus->mutex);
        
        // 检查是否应该停止
        if (!bus->thread_running) {
            pthread_mutex_unlock(&bus->mutex);
            break;
        }
        
        // 等待事件或超时
        if (vector_linx_audio_event_t_empty(&bus->event_queue)) {
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_nsec += EVENT_BUS_THREAD_TIMEOUT_MS * 1000000;
            if (timeout.tv_nsec >= 1000000000) {
                timeout.tv_sec++;
                timeout.tv_nsec -= 1000000000;
            }
            
            pthread_cond_timedwait(&bus->condition, &bus->mutex, &timeout);
        }
        
        // 处理事件
        if (!vector_linx_audio_event_t_empty(&bus->event_queue)) {
            linx_audio_event_t* event = vector_linx_audio_event_t_at(&bus->event_queue, 0);
            if (event) {
                linx_audio_event_t event_copy = *event;
                vector_linx_audio_event_t_erase(&bus->event_queue, 0);
                
                pthread_mutex_unlock(&bus->mutex);
                
                event_bus_dispatch_event(bus, &event_copy);
                event_bus_free_event(&event_copy);
                
                pthread_mutex_lock(&bus->mutex);
                bus->events_processed++;
            }
        }
        
        pthread_mutex_unlock(&bus->mutex);
    }
    
    LINX_LOGI(TAG, "Event bus worker thread stopped");
    return NULL;
}

static linx_audio_error_t event_bus_dispatch_event(linx_event_bus_t* bus, 
                                                   const linx_audio_event_t* event)
{
    if (!bus || !event) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&bus->mutex);
    
    size_t delivered_count = 0;
    for (size_t i = 0; i < vector_event_subscriber_t_size(&bus->subscribers); i++) {
        event_subscriber_t* subscriber = vector_event_subscriber_t_at(&bus->subscribers, i);
        if (subscriber && event_bus_should_deliver_event(subscriber, event)) {
            pthread_mutex_unlock(&bus->mutex);
            
            // 调用回调函数（在锁外执行以避免死锁）
            subscriber->callback(event, subscriber->user_data);
            delivered_count++;
            
            pthread_mutex_lock(&bus->mutex);
        }
    }
    
    pthread_mutex_unlock(&bus->mutex);
    
    LINX_LOGD(TAG, "Event dispatched: type=%s, delivered to %zu subscribers",
              event_bus_get_event_type_name(event->type), delivered_count);
    
    return LINX_AUDIO_SUCCESS;
}

static bool event_bus_should_deliver_event(const event_subscriber_t* subscriber,
                                           const linx_audio_event_t* event)
{
    if (!subscriber || !event || !subscriber->is_active) {
        return false;
    }
    
    // 检查事件类型匹配
    return (subscriber->event_type == LINX_AUDIO_EVENT_UNKNOWN || 
            subscriber->event_type == event->type);
}

static void event_bus_cleanup_subscriber(event_subscriber_t* subscriber)
{
    if (subscriber) {
        subscriber->is_active = false;
        subscriber->callback = NULL;
        subscriber->user_data = NULL;
    }
}

static linx_audio_error_t event_bus_validate_params(linx_event_bus_t* bus)
{
    if (!bus) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (!bus->initialized) {
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    return LINX_AUDIO_SUCCESS;
}