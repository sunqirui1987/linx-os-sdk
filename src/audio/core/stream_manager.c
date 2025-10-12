/**
 * @file stream_manager.c
 * @brief LinxOS音频流管理器实现
 * 
 * 流管理器负责音频流的生命周期管理、调度和优先级管理、
 * 流间的混音和路由、流的状态监控和统计、流的格式协商和转换
 */

#include "stream_manager.h"
#include "audio_manager.h"
#include "event_bus.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

// ============================================================================
// 内部常量定义
// ============================================================================

#define STREAM_MANAGER_DEFAULT_NAME "LinxOS Stream Manager"
#define STREAM_MANAGER_SCHEDULER_INTERVAL_MS 10
#define STREAM_MANAGER_MAX_STREAMS_DEFAULT 32
#define STREAM_MANAGER_MAX_CONCURRENT_STREAMS_DEFAULT 16

// ============================================================================
// 内部函数声明
// ============================================================================

static audio_result_t linx_stream_manager_init_scheduler(linx_stream_manager_t* manager);
static void linx_stream_manager_cleanup_scheduler(linx_stream_manager_t* manager);
static void* linx_stream_manager_scheduler_thread(void* arg);
static audio_result_t linx_stream_manager_schedule_streams(linx_stream_manager_t* manager);
static audio_result_t linx_stream_manager_process_stream(linx_stream_manager_t* manager, linx_audio_stream_t* stream);
static audio_result_t linx_stream_manager_validate_config(const audio_stream_config_t* config);
static uint32_t linx_stream_manager_generate_stream_id(linx_stream_manager_t* manager);
static void linx_stream_manager_update_stats(linx_stream_manager_t* manager);
static const char* linx_stream_state_to_string(linx_audio_stream_state_t state);

// ============================================================================
// 默认配置
// ============================================================================

static const linx_stream_manager_config_t DEFAULT_STREAM_MANAGER_CONFIG = {
    .max_streams = STREAM_MANAGER_MAX_STREAMS_DEFAULT,
    .max_concurrent_streams = STREAM_MANAGER_MAX_CONCURRENT_STREAMS_DEFAULT,
    .enable_priority_scheduling = true,
    .enable_load_balancing = true,
    .scheduler_interval_ms = STREAM_MANAGER_SCHEDULER_INTERVAL_MS,
    .enable_automatic_mixing = true,
    .mixer_buffer_size = 1024,
    .mixer_format = LINX_AUDIO_FORMAT_S16LE,
    .thread_priority = LINX_AUDIO_THREAD_PRIORITY_HIGH,
    .enable_realtime_scheduling = true,
    .enable_debug_logging = false,
    .enable_performance_monitoring = true
};

// ============================================================================
// 公共API实现
// ============================================================================

linx_stream_manager_t* linx_stream_manager_create(struct linx_audio_manager* manager,
                                       const linx_stream_manager_config_t* config)
{
    if (!manager) {
        LOG_ERROR("音频管理器参数无效");
        return NULL;
    }

    LOG_DEBUG("创建流管理器");

    // 分配内存
    linx_stream_manager_t* stream_manager = calloc(1, sizeof(linx_stream_manager_t));
    if (!stream_manager) {
        LOG_ERROR("分配流管理器内存失败");
        return NULL;
    }

    // 设置基本信息
    stream_manager->id = 1; // TODO: 生成唯一ID
    strncpy(stream_manager->name, STREAM_MANAGER_DEFAULT_NAME, 
            sizeof(stream_manager->name) - 1);

    // 设置配置
    if (config) {
        stream_manager->config = *config;
    } else {
        stream_manager->config = DEFAULT_STREAM_MANAGER_CONFIG;
    }

    // 分配流数组
    stream_manager->streams = calloc(stream_manager->config.max_streams, 
                                   sizeof(linx_audio_stream_t*));
    if (!stream_manager->streams) {
        LOG_ERROR("分配流数组内存失败");
        free(stream_manager);
        return NULL;
    }

    // 初始化同步对象
    if (pthread_mutex_init(&stream_manager->mutex, NULL) != 0) {
        LOG_ERROR("初始化互斥锁失败");
        free(stream_manager->streams);
        free(stream_manager);
        return NULL;
    }

    if (pthread_cond_init(&stream_manager->condition, NULL) != 0) {
        LOG_ERROR("初始化条件变量失败");
        pthread_mutex_destroy(&stream_manager->mutex);
        free(stream_manager->streams);
        free(stream_manager);
        return NULL;
    }

    // 设置引用
    stream_manager->manager = manager;
    stream_manager->next_stream_id = 1;

    LOG_DEBUG("流管理器创建完成");
    return stream_manager;
}

void linx_stream_manager_destroy(linx_stream_manager_t* manager)
{
    if (!manager) {
        return;
    }

    LOG_DEBUG("销毁流管理器");

    // 停止调度器
    linx_stream_manager_cleanup_scheduler(manager);

    // 销毁所有流
    if (manager->streams) {
        for (uint32_t i = 0; i < manager->stream_count; i++) {
            if (manager->streams[i]) {
                // TODO: 销毁流
                free(manager->streams[i]);
            }
        }
        free(manager->streams);
    }

    // 销毁同步对象
    pthread_cond_destroy(&manager->condition);
    pthread_mutex_destroy(&manager->mutex);

    // 释放内存
    free(manager);

    LOG_DEBUG("流管理器销毁完成");
}

audio_result_t linx_stream_manager_initialize(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("初始化流管理器");

    // 初始化调度器
    audio_result_t result = linx_stream_manager_init_scheduler(manager);
    if (result != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("初始化调度器失败: %d", result);
        return result;
    }

    LOG_DEBUG("流管理器初始化完成");
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_deinitialize(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("反初始化流管理器");

    // 停止所有流
    for (uint32_t i = 0; i < manager->stream_count; i++) {
        if (manager->streams[i]) {
            linx_stream_manager_stop_stream(manager, manager->streams[i]);
        }
    }

    // 清理调度器
    linx_stream_manager_cleanup_scheduler(manager);

    LOG_DEBUG("流管理器反初始化完成");
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_start(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("启动流管理器");

    // 启动调度器线程
    manager->scheduler_running = true;
    if (pthread_create(&manager->scheduler_thread, NULL, 
                      linx_stream_manager_scheduler_thread, manager) != 0) {
        LOG_ERROR("创建调度器线程失败");
        manager->scheduler_running = false;
        return LINX_AUDIO_ERROR_THREAD_CREATE;
    }

    LOG_DEBUG("流管理器启动完成");
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_stop(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("停止流管理器");

    // 停止调度器线程
    if (manager->scheduler_running) {
        manager->scheduler_running = false;
        pthread_cond_signal(&manager->condition);
        pthread_join(manager->scheduler_thread, NULL);
    }

    LOG_DEBUG("流管理器停止完成");
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 流管理接口实现
// ============================================================================

audio_result_t linx_stream_manager_create_stream(linx_stream_manager_t* manager,
                                           const audio_stream_config_t* config,
                                           linx_audio_stream_t** stream)
{
    if (!manager || !config || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("创建音频流");

    // 验证配置
    audio_result_t result = linx_stream_manager_validate_config(config);
    if (result != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("流配置验证失败: %d", result);
        return result;
    }

    pthread_mutex_lock(&manager->mutex);

    // 检查流数量限制
    if (manager->stream_count >= manager->config.max_streams) {
        LOG_ERROR("已达到最大流数量限制: %d", manager->config.max_streams);
        pthread_mutex_unlock(&manager->mutex);
        return LINX_AUDIO_ERROR_RESOURCE_LIMIT;
    }

    // 分配流内存
    linx_audio_stream_t* new_stream = calloc(1, sizeof(linx_audio_stream_t));
    if (!new_stream) {
        LOG_ERROR("分配流内存失败");
        pthread_mutex_unlock(&manager->mutex);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }

    // 设置流基本信息
    new_stream->id = linx_stream_manager_generate_stream_id(manager);
    strncpy(new_stream->name, config->name, sizeof(new_stream->name) - 1);
    new_stream->type = config->type;
    new_stream->state = LINX_AUDIO_STREAM_STATE_CREATED;
    new_stream->priority = config->priority;
    new_stream->config = *config;
    new_stream->manager = manager;

    // 初始化同步对象
    if (pthread_mutex_init(&new_stream->mutex, NULL) != 0) {
        LOG_ERROR("初始化流互斥锁失败");
        free(new_stream);
        pthread_mutex_unlock(&manager->mutex);
        return LINX_AUDIO_ERROR_MUTEX_INIT;
    }

    if (pthread_cond_init(&new_stream->condition, NULL) != 0) {
        LOG_ERROR("初始化流条件变量失败");
        pthread_mutex_destroy(&new_stream->mutex);
        free(new_stream);
        pthread_mutex_unlock(&manager->mutex);
        return LINX_AUDIO_ERROR_CONDITION_INIT;
    }

    // 添加到流数组
    manager->streams[manager->stream_count] = new_stream;
    manager->stream_count++;

    // 更新统计信息
    linx_stream_manager_update_stats(manager);

    *stream = new_stream;

    pthread_mutex_unlock(&manager->mutex);

    LOG_DEBUG("音频流创建完成，ID: %d", new_stream->id);
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_destroy_stream(linx_stream_manager_t* manager,
                                            linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("销毁音频流，ID: %d", stream->id);

    pthread_mutex_lock(&manager->mutex);

    // 查找并移除流
    bool found = false;
    for (uint32_t i = 0; i < manager->stream_count; i++) {
        if (manager->streams[i] == stream) {
            // 移动后续元素
            for (uint32_t j = i; j < manager->stream_count - 1; j++) {
                manager->streams[j] = manager->streams[j + 1];
            }
            manager->stream_count--;
            found = true;
            break;
        }
    }

    if (!found) {
        LOG_ERROR("未找到要销毁的流，ID: %d", stream->id);
        pthread_mutex_unlock(&manager->mutex);
        return LINX_AUDIO_ERROR_NOT_FOUND;
    }

    // 销毁同步对象
    pthread_cond_destroy(&stream->condition);
    pthread_mutex_destroy(&stream->mutex);

    // 释放内存
    free(stream);

    // 更新统计信息
    linx_stream_manager_update_stats(manager);

    pthread_mutex_unlock(&manager->mutex);

    LOG_DEBUG("音频流销毁完成");
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_start_stream(linx_stream_manager_t* manager,
                                          linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("启动音频流，ID: %d", stream->id);

    pthread_mutex_lock(&stream->mutex);

    if (stream->state != LINX_AUDIO_STREAM_STATE_CREATED && 
        stream->state != LINX_AUDIO_STREAM_STATE_STOPPED) {
        LOG_ERROR("流状态不允许启动，当前状态: %s", 
                 linx_stream_state_to_string(stream->state));
        pthread_mutex_unlock(&stream->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }

    // 设置流状态
    stream->state = LINX_AUDIO_STREAM_STATE_RUNNING;

    pthread_mutex_unlock(&stream->mutex);

    LOG_DEBUG("音频流启动完成，ID: %d", stream->id);
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_stop_stream(linx_stream_manager_t* manager,
                                         linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("停止音频流，ID: %d", stream->id);

    pthread_mutex_lock(&stream->mutex);

    if (stream->state != LINX_AUDIO_STREAM_STATE_RUNNING && 
        stream->state != LINX_AUDIO_STREAM_STATE_PAUSED) {
        LOG_ERROR("流状态不允许停止，当前状态: %s", 
                 linx_stream_state_to_string(stream->state));
        pthread_mutex_unlock(&stream->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }

    // 设置流状态
    stream->state = LINX_AUDIO_STREAM_STATE_STOPPED;

    pthread_mutex_unlock(&stream->mutex);

    LOG_DEBUG("音频流停止完成，ID: %d", stream->id);
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_pause_stream(linx_stream_manager_t* manager,
                                          linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("暂停音频流，ID: %d", stream->id);

    pthread_mutex_lock(&stream->mutex);

    if (stream->state != LINX_AUDIO_STREAM_STATE_RUNNING) {
        LOG_ERROR("流状态不允许暂停，当前状态: %s", 
                 linx_stream_state_to_string(stream->state));
        pthread_mutex_unlock(&stream->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }

    // 设置流状态
    stream->state = LINX_AUDIO_STREAM_STATE_PAUSED;

    pthread_mutex_unlock(&stream->mutex);

    LOG_DEBUG("音频流暂停完成，ID: %d", stream->id);
    return LINX_AUDIO_SUCCESS;
}

audio_result_t linx_stream_manager_resume_stream(linx_stream_manager_t* manager,
                                           linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("恢复音频流，ID: %d", stream->id);

    pthread_mutex_lock(&stream->mutex);

    if (stream->state != LINX_AUDIO_STREAM_STATE_PAUSED) {
        LOG_ERROR("流状态不允许恢复，当前状态: %s", 
                 linx_stream_state_to_string(stream->state));
        pthread_mutex_unlock(&stream->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }

    // 设置流状态
    stream->state = LINX_AUDIO_STREAM_STATE_RUNNING;

    pthread_mutex_unlock(&stream->mutex);

    LOG_DEBUG("音频流恢复完成，ID: %d", stream->id);
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 内部函数实现
// ============================================================================

static audio_result_t linx_stream_manager_init_scheduler(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    LOG_DEBUG("初始化流调度器");

    // 设置调度器参数
    manager->scheduler_running = false;

    LOG_DEBUG("流调度器初始化完成");
    return LINX_AUDIO_SUCCESS;
}

static void linx_stream_manager_cleanup_scheduler(linx_stream_manager_t* manager)
{
    if (!manager) {
        return;
    }

    LOG_DEBUG("清理流调度器");

    // 停止调度器线程
    if (manager->scheduler_running) {
        manager->scheduler_running = false;
        pthread_cond_signal(&manager->condition);
        pthread_join(manager->scheduler_thread, NULL);
    }

    LOG_DEBUG("流调度器清理完成");
}

static void* linx_stream_manager_scheduler_thread(void* arg)
{
    linx_stream_manager_t* manager = (linx_stream_manager_t*)arg;
    if (!manager) {
        return NULL;
    }

    LOG_DEBUG("调度器线程启动");

    struct timespec sleep_time = {
        .tv_sec = 0,
        .tv_nsec = manager->config.scheduler_interval_ms * 1000000
    };

    while (manager->scheduler_running) {
        // 执行调度
        linx_stream_manager_schedule_streams(manager);

        // 休眠
        nanosleep(&sleep_time, NULL);
    }

    LOG_DEBUG("调度器线程退出");
    return NULL;
}

static audio_result_t linx_stream_manager_schedule_streams(linx_stream_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&manager->mutex);

    // 遍历所有活跃流
    for (uint32_t i = 0; i < manager->stream_count; i++) {
        linx_audio_stream_t* stream = manager->streams[i];
        if (stream && stream->state == LINX_AUDIO_STREAM_STATE_RUNNING) {
            linx_stream_manager_process_stream(manager, stream);
        }
    }

    manager->stats.scheduler_runs++;
    pthread_mutex_unlock(&manager->mutex);

    return LINX_AUDIO_SUCCESS;
}

static audio_result_t linx_stream_manager_process_stream(linx_stream_manager_t* manager, 
                                                   linx_audio_stream_t* stream)
{
    if (!manager || !stream) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    // 这里实现具体的流处理逻辑
    // 包括数据回调、格式转换、缓冲区管理等
    
    return LINX_AUDIO_SUCCESS;
}

static audio_result_t linx_stream_manager_validate_config(const audio_stream_config_t* config)
{
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    // 验证流类型
    if (config->type >= LINX_AUDIO_STREAM_TYPE_VIRTUAL + 1) {
        LOG_ERROR("无效的流类型: %d", config->type);
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    // 验证优先级
    if (config->priority >= LINX_AUDIO_STREAM_PRIORITY_REALTIME + 1) {
        LOG_ERROR("无效的流优先级: %d", config->priority);
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    return LINX_AUDIO_SUCCESS;
}

static uint32_t linx_stream_manager_generate_stream_id(linx_stream_manager_t* manager)
{
    if (!manager) {
        return 0;
    }

    return manager->next_stream_id++;
}

static void linx_stream_manager_update_stats(linx_stream_manager_t* manager)
{
    if (!manager) {
        return;
    }

    // 更新统计信息
    manager->stats.total_streams = manager->stream_count;
    
    uint32_t active_count = 0;
    uint32_t paused_count = 0;
    uint32_t playback_count = 0;
    uint32_t capture_count = 0;
    uint32_t duplex_count = 0;

    for (uint32_t i = 0; i < manager->stream_count; i++) {
        linx_audio_stream_t* stream = manager->streams[i];
        if (stream) {
            switch (stream->state) {
                case LINX_AUDIO_STREAM_STATE_RUNNING:
                    active_count++;
                    break;
                case LINX_AUDIO_STREAM_STATE_PAUSED:
                    paused_count++;
                    break;
                default:
                    break;
            }

            switch (stream->type) {
                case LINX_AUDIO_STREAM_TYPE_PLAYBACK:
                    playback_count++;
                    break;
                case LINX_AUDIO_STREAM_TYPE_CAPTURE:
                    capture_count++;
                    break;
                case LINX_AUDIO_STREAM_TYPE_DUPLEX:
                    duplex_count++;
                    break;
                default:
                    break;
            }
        }
    }

    manager->stats.active_streams = active_count;
    manager->stats.paused_streams = paused_count;
    manager->stats.playback_streams = playback_count;
    manager->stats.capture_streams = capture_count;
    manager->stats.duplex_streams = duplex_count;
}

static const char* linx_stream_state_to_string(linx_audio_stream_state_t state)
{
    switch (state) {
        case LINX_AUDIO_STREAM_STATE_CREATED:
            return "CREATED";
        case LINX_AUDIO_STREAM_STATE_INITIALIZED:
            return "INITIALIZED";
        case LINX_AUDIO_STREAM_STATE_RUNNING:
            return "RUNNING";
        case LINX_AUDIO_STREAM_STATE_PAUSED:
            return "PAUSED";
        case LINX_AUDIO_STREAM_STATE_STOPPED:
            return "STOPPED";
        case LINX_AUDIO_STREAM_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

// ============================================================================
// 实用工具函数实现
// ============================================================================

const char* linx_audio_stream_type_to_string(linx_audio_stream_type_t type)
{
    switch (type) {
        case LINX_AUDIO_STREAM_TYPE_PLAYBACK:
            return "PLAYBACK";
        case LINX_AUDIO_STREAM_TYPE_CAPTURE:
            return "CAPTURE";
        case LINX_AUDIO_STREAM_TYPE_DUPLEX:
            return "DUPLEX";
        case LINX_AUDIO_STREAM_TYPE_LOOPBACK:
            return "LOOPBACK";
        case LINX_AUDIO_STREAM_TYPE_VIRTUAL:
            return "VIRTUAL";
        default:
            return "UNKNOWN";
    }
}

const char* linx_audio_stream_priority_to_string(linx_audio_stream_priority_t priority)
{
    switch (priority) {
        case LINX_AUDIO_STREAM_PRIORITY_LOW:
            return "LOW";
        case LINX_AUDIO_STREAM_PRIORITY_NORMAL:
            return "NORMAL";
        case LINX_AUDIO_STREAM_PRIORITY_HIGH:
            return "HIGH";
        case LINX_AUDIO_STREAM_PRIORITY_REALTIME:
            return "REALTIME";
        default:
            return "UNKNOWN";
    }
}

audio_result_t linx_stream_manager_get_default_config(linx_stream_manager_config_t* config)
{
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    *config = DEFAULT_STREAM_MANAGER_CONFIG;
    return LINX_AUDIO_SUCCESS;
}