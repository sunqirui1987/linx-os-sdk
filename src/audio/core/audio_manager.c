/**
 * @file audio_manager.c
 * @brief LinxOS音频系统管理器实现
 * @details 音频系统的核心管理组件实现，负责音频设备、流和插件的统一管理
 */

#include "audio_manager.h"
#include "stream_manager.h"
#include "../plugins/plugin_manager.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

// ============================================================================
// 内部结构定义
// ============================================================================

/**
 * @brief 音频管理器内部结构
 */
struct linx_audio_manager {
    // 基础配置和状态
    linx_audio_manager_config_t config;         /**< 管理器配置 */
    linx_audio_manager_state_t state;           /**< 当前状态 */
    pthread_mutex_t state_mutex;               /**< 状态锁 */
    
    // 核心组件
    linx_event_bus_t* event_bus;               /**< 事件总线 */
    uint32_t subscriber_id;                    /**< 事件订阅者ID */
    linx_stream_manager_t* stream_manager;          /**< 流管理器 */
    linx_plugin_manager_t* plugin_manager;          /**< 插件管理器 */
    
    // 设备管理
    vector_linx_audio_device_info_t_t input_devices;     /**< 输入设备列表 */
    vector_linx_audio_device_info_t_t output_devices;    /**< 输出设备列表 */
    uint32_t default_input_device;             /**< 默认输入设备ID */
    uint32_t default_output_device;            /**< 默认输出设备ID */
    pthread_mutex_t device_mutex;              /**< 设备锁 */
    
    // 流管理
    vector_linx_audio_stream_ptr_t_t active_streams;     /**< 活跃流列表 */
    pthread_mutex_t stream_mutex;              /**< 流锁 */
    
    // 回调函数
    linx_audio_event_callback_t event_callback;    /**< 事件回调 */
    linx_audio_error_callback_t error_callback;    /**< 错误回调 */
    void* callback_user_data;                      /**< 回调用户数据 */
    
    // 统计信息
    linx_audio_manager_stats_t stats;          /**< 统计信息 */
    pthread_mutex_t stats_mutex;               /**< 统计锁 */
    
    // 内部状态
    bool initialized;                          /**< 是否已初始化 */
    bool running;                              /**< 是否正在运行 */
    time_t start_time;                         /**< 启动时间 */
    
    // 日志上下文
    log_context_t* log_ctx;                    /**< 日志上下文 */
};

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t manager_init_components(linx_audio_manager_t* manager);
static void manager_cleanup_components(linx_audio_manager_t* manager);
static linx_audio_result_t manager_init_devices(linx_audio_manager_t* manager);
static void manager_cleanup_devices(linx_audio_manager_t* manager);
static void manager_update_stats(linx_audio_manager_t* manager);
static void manager_event_handler(const linx_audio_event_t* event, void* user_data);


// ============================================================================
// 默认配置
// ============================================================================

static const linx_audio_manager_config_t DEFAULT_CONFIG = {
    .max_devices = 16,
    .max_streams = 32,
    .max_plugins = 64,
    .buffer_size = LINX_AUDIO_DEFAULT_BUFFER_SIZE,
    .sample_rate = 44100,
    .format = LINX_AUDIO_FORMAT_S16LE,
    .enable_event_bus = true,
    .enable_hot_plug = true,
    .enable_auto_routing = false,
    .log_level = "INFO"
};

// ============================================================================
// 核心接口实现
// ============================================================================

linx_audio_manager_t* linx_audio_manager_create(const linx_audio_manager_config_t* config)
{
    LOG_INFO("创建音频管理器");
    
    // 分配内存
    linx_audio_manager_t* manager = calloc(1, sizeof(linx_audio_manager_t));
    if (!manager) {
        LOG_ERROR("分配音频管理器内存失败");
        return NULL;
    }
    
    // 设置配置
    if (config) {
        manager->config = *config;
    } else {
        manager->config = DEFAULT_CONFIG;
    }
    
    // 初始化状态
    manager->state = LINX_AUDIO_MANAGER_STATE_UNINITIALIZED;
    manager->initialized = false;
    manager->running = false;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&manager->state_mutex, NULL) != 0) {
        LOG_ERROR("初始化状态锁失败: %s", strerror(errno));
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->device_mutex, NULL) != 0) {
        LOG_ERROR("初始化设备锁失败: %s", strerror(errno));
        pthread_mutex_destroy(&manager->state_mutex);
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->stream_mutex, NULL) != 0) {
        LOG_ERROR("初始化流锁失败: %s", strerror(errno));
        pthread_mutex_destroy(&manager->device_mutex);
        pthread_mutex_destroy(&manager->state_mutex);
        free(manager);
        return NULL;
    }
    
    if (pthread_mutex_init(&manager->stats_mutex, NULL) != 0) {
        LOG_ERROR("初始化统计锁失败: %s", strerror(errno));
        pthread_mutex_destroy(&manager->stream_mutex);
        pthread_mutex_destroy(&manager->device_mutex);
        pthread_mutex_destroy(&manager->state_mutex);
        free(manager);
        return NULL;
    }
    
    // 初始化日志上下文
    manager->log_ctx = calloc(1, sizeof(log_context_t));
    if (manager->log_ctx) {
        log_config_t config = LOG_DEFAULT_CONFIG;
        manager->log_ctx->config = config;
        manager->log_ctx->initialized = true;
    } else {
        LOG_WARN("创建日志上下文失败，使用默认日志");
    }
    
    // 初始化向量容器
    if (vector_linx_audio_device_info_t_init(&manager->input_devices) != 0 ||
        vector_linx_audio_device_info_t_init(&manager->output_devices) != 0 ||
        vector_linx_audio_stream_ptr_t_init(&manager->active_streams) != 0) {
        LOG_ERROR("初始化向量容器失败");
        linx_audio_manager_destroy(manager);
        return NULL;
    }
    
    // 初始化统计信息
    memset(&manager->stats, 0, sizeof(manager->stats));
    manager->start_time = time(NULL);
    
    LOG_INFO("音频管理器创建成功");
    return manager;
}

void linx_audio_manager_destroy(linx_audio_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    LOG_INFO("销毁音频管理器");
    
    // 停止管理器
    if (manager->running) {
        linx_audio_manager_stop(manager);
    }
    
    // 反初始化
    if (manager->initialized) {
        linx_audio_manager_deinit(manager);
    }
    
    // 清理组件
    manager_cleanup_components(manager);
    
    // 清理设备
    manager_cleanup_devices(manager);
    
    // 销毁向量容器
    vector_linx_audio_device_info_t_destroy(&manager->input_devices);
    vector_linx_audio_device_info_t_destroy(&manager->output_devices);
    vector_linx_audio_stream_ptr_t_destroy(&manager->active_streams);
    
    // 销毁日志上下文
    if (manager->log_ctx) {
        free(manager->log_ctx);
    }
    
    // 销毁互斥锁
    pthread_mutex_destroy(&manager->stats_mutex);
    pthread_mutex_destroy(&manager->stream_mutex);
    pthread_mutex_destroy(&manager->device_mutex);
    pthread_mutex_destroy(&manager->state_mutex);
    
    // 释放内存
    free(manager);
    
    LOG_INFO("音频管理器销毁完成");
}

linx_audio_result_t linx_audio_manager_init(linx_audio_manager_t* manager)
{
    if (!manager) {
        LOG_ERROR("音频管理器指针为空");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->state_mutex);
    
    if (manager->initialized) {
        LOG_WARN("音频管理器已经初始化");
        pthread_mutex_unlock(&manager->state_mutex);
        return LINX_AUDIO_ERROR_ALREADY_INITIALIZED;
    }
    
    LOG_INFO("初始化音频管理器");
    
    // 更新状态
    manager->state = LINX_AUDIO_MANAGER_STATE_INITIALIZING;
    
    // 初始化组件
    linx_audio_result_t result = manager_init_components(manager);
    if (result != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("初始化组件失败: %d", result);
        manager->state = LINX_AUDIO_MANAGER_STATE_ERROR;
        pthread_mutex_unlock(&manager->state_mutex);
        return result;
    }
    
    // 初始化设备
    result = manager_init_devices(manager);
    if (result != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("初始化设备失败: %d", result);
        manager_cleanup_components(manager);
        manager->state = LINX_AUDIO_MANAGER_STATE_ERROR;
        pthread_mutex_unlock(&manager->state_mutex);
        return result;
    }
    
    // 注册事件处理器
    if (manager->event_bus && manager->config.enable_event_bus) {
        uint32_t subscriber_id = linx_event_bus_subscribe(manager->event_bus, 
                                                     LINX_AUDIO_EVENT_TYPE_ALL,
                                                     manager_event_handler,
                                                     manager);
        if (subscriber_id == 0) {
            LOG_ERROR("注册事件处理器失败");
            manager_cleanup_components(manager);
            manager->state = LINX_AUDIO_MANAGER_STATE_ERROR;
            pthread_mutex_unlock(&manager->state_mutex);
            return LINX_AUDIO_ERROR_UNKNOWN;
        }
    }
    
    // 设置状态
    manager->initialized = true;
    manager->state = LINX_AUDIO_MANAGER_STATE_INITIALIZED;
    
    // 更新统计信息
    manager->stats.init_count++;
    
    pthread_mutex_unlock(&manager->state_mutex);
    
    LOG_INFO("音频管理器初始化完成");
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_manager_start(linx_audio_manager_t* manager)
{
    if (!manager) {
        LOG_ERROR("音频管理器指针为空");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->state_mutex);
    
    if (!manager->initialized) {
        LOG_ERROR("音频管理器未初始化");
        pthread_mutex_unlock(&manager->state_mutex);
        return LINX_AUDIO_ERROR_NOT_INITIALIZED;
    }
    
    if (manager->running) {
        LOG_INFO("音频管理器已经在运行");
        pthread_mutex_unlock(&manager->state_mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    LOG_INFO("启动音频管理器");
    
    // 更新状态
    manager->state = LINX_AUDIO_MANAGER_STATE_STARTING;
    
    // 启动事件总线
    if (manager->event_bus && manager->config.enable_event_bus) {
        linx_audio_result_t result = linx_event_bus_start(manager->event_bus);
        if (result != LINX_AUDIO_SUCCESS) {
            LOG_ERROR("启动事件总线失败: %d", result);
            manager->state = LINX_AUDIO_MANAGER_STATE_ERROR;
            pthread_mutex_unlock(&manager->state_mutex);
            return LINX_AUDIO_ERROR_UNKNOWN;
        }
    }
    
    // 启动音频处理
    LOG_DEBUG("音频处理启动成功");
    
    // 启动流管理器
    if (manager->stream_manager) {
        if (linx_stream_manager_start(manager->stream_manager) != LINX_AUDIO_SUCCESS) {
            LOG_ERROR("启动流管理器失败");
            pthread_mutex_unlock(&manager->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        LOG_DEBUG("流管理器启动成功");
    }
    
    // 启动插件管理器
    if (manager->plugin_manager) {
        if (linx_plugin_manager_start(manager->plugin_manager) != LINX_AUDIO_SUCCESS) {
            LOG_ERROR("启动插件管理器失败");
            pthread_mutex_unlock(&manager->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        LOG_DEBUG("插件管理器启动成功");
    }
    
    // 设置状态
    manager->running = true;
    manager->state = LINX_AUDIO_MANAGER_STATE_RUNNING;
    
    // 更新统计信息
    manager->stats.start_count++;
    
    pthread_mutex_unlock(&manager->state_mutex);
    
    LOG_INFO("音频管理器启动完成");
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_manager_stop(linx_audio_manager_t* manager)
{
    if (!manager) {
        LOG_ERROR("音频管理器指针为空");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->state_mutex);
    
    if (!manager->running) {
        LOG_INFO("音频管理器已经停止");
        pthread_mutex_unlock(&manager->state_mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    LOG_INFO("停止音频管理器");
    
    // 更新状态
    manager->state = LINX_AUDIO_MANAGER_STATE_STOPPING;
    
    // 停止插件管理器
    if (manager->plugin_manager) {
        if (linx_plugin_manager_stop(manager->plugin_manager) != LINX_AUDIO_SUCCESS) {
            LOG_WARN("停止插件管理器失败");
        } else {
            LOG_DEBUG("插件管理器停止成功");
        }
    }
    
    // 停止流管理器
    if (manager->stream_manager) {
        if (linx_stream_manager_stop(manager->stream_manager) != LINX_AUDIO_SUCCESS) {
            LOG_WARN("停止流管理器失败");
        } else {
            LOG_DEBUG("流管理器停止成功");
        }
    }
    
    // 停止音频处理
    LOG_DEBUG("音频处理停止成功");
    
    // 停止事件总线
    if (manager->event_bus && manager->config.enable_event_bus) {
        linx_audio_result_t result = linx_event_bus_stop(manager->event_bus);
        if (result != LINX_AUDIO_SUCCESS) {
            LOG_WARN("停止事件总线失败: %d", result);
        }
    }
    
    // 设置状态
    manager->running = false;
    manager->state = LINX_AUDIO_MANAGER_STATE_STOPPED;
    
    // 更新统计信息
    manager->stats.stop_count++;
    
    pthread_mutex_unlock(&manager->state_mutex);
    
    LOG_INFO("音频管理器停止完成");
    return LINX_AUDIO_SUCCESS;
}

// 反初始化音频管理器
linx_audio_result_t linx_audio_manager_deinit(linx_audio_manager_t* manager)
{
    if (!manager) {
        LOG_ERROR("音频管理器指针为空");
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&manager->state_mutex);
    
    if (!manager->initialized) {
        LOG_INFO("音频管理器已经反初始化");
        pthread_mutex_unlock(&manager->state_mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    LOG_INFO("反初始化音频管理器");
    
    // 确保已停止
    if (manager->running) {
        pthread_mutex_unlock(&manager->state_mutex);
        linx_audio_manager_stop(manager);
        pthread_mutex_lock(&manager->state_mutex);
    }
    
    // 更新状态
    manager->state = LINX_AUDIO_MANAGER_STATE_DEINITIALIZING;
    
    // 清理组件
    manager_cleanup_components(manager);
    
    // 清理设备
    manager_cleanup_devices(manager);
    
    // 取消事件订阅
    if (manager->event_bus && manager->config.enable_event_bus) {
        // TODO: 需要保存subscriber_id以便取消订阅
        // event_bus_unsubscribe(manager->event_bus, subscriber_id);
    }
    
    // 更新状态和标志
    manager->initialized = false;
    manager->state = LINX_AUDIO_MANAGER_STATE_UNINITIALIZED;
    
    // 更新统计信息
    manager->stats.init_count--;
    
    pthread_mutex_unlock(&manager->state_mutex);
    
    LOG_INFO("音频管理器反初始化完成");
    return LINX_AUDIO_SUCCESS;
}

// 获取默认配置
linx_audio_result_t linx_audio_manager_get_default_config(linx_audio_manager_config_t* config)
{
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    *config = DEFAULT_CONFIG;
    return LINX_AUDIO_SUCCESS;
}

// 获取管理器状态
linx_audio_manager_state_t linx_audio_manager_get_state(const linx_audio_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_MANAGER_STATE_ERROR;
    }
    
    return manager->state;
}

// 检查是否已初始化
bool linx_audio_manager_is_initialized(const linx_audio_manager_t* manager)
{
    return manager ? manager->initialized : false;
}

// 检查是否正在运行
bool linx_audio_manager_is_running(const linx_audio_manager_t* manager)
{
    return manager ? manager->running : false;
}

// 获取统计信息
linx_audio_result_t linx_audio_manager_get_stats(const linx_audio_manager_t* manager,
                                                  linx_audio_manager_stats_t* stats)
{
    if (!manager || !stats) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock((pthread_mutex_t*)&manager->stats_mutex);
    *stats = manager->stats;
    pthread_mutex_unlock((pthread_mutex_t*)&manager->stats_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// 内部辅助函数实现

static linx_audio_result_t manager_init_components(linx_audio_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    LOG_DEBUG("初始化音频管理器组件");
    
    // 创建事件总线
    if (manager->config.enable_event_bus) {
        manager->event_bus = linx_event_bus_create();
        if (!manager->event_bus) {
            LOG_ERROR("创建事件总线失败");
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        
        // 初始化事件总线
        linx_audio_result_t result = linx_event_bus_init(manager->event_bus, true);
        if (result != LINX_AUDIO_SUCCESS) {
            LOG_ERROR("初始化事件总线失败: %d", result);
            linx_event_bus_destroy(manager->event_bus);
            manager->event_bus = NULL;
            return result;
        }
        
        // 注册事件处理器
        manager->subscriber_id = linx_event_bus_subscribe(manager->event_bus, LINX_AUDIO_EVENT_TYPE_ALL, 
                                                   manager_event_handler, manager);
        if (manager->subscriber_id == 0) {
            LOG_WARN("注册事件处理器失败");
        }
    }
    
    // 音频处理功能已集成到管理器中
    LOG_DEBUG("音频处理功能初始化完成");
    
    // 创建流管理器
    linx_stream_manager_config_t stream_config;
    linx_stream_manager_get_default_config(&stream_config);
    stream_config.max_streams = manager->config.max_streams;
    stream_config.mixer_buffer_size = manager->config.buffer_size;
    
    manager->stream_manager = linx_stream_manager_create(manager, &stream_config);
    if (!manager->stream_manager) {
        LOG_ERROR("创建流管理器失败");
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    if (linx_stream_manager_initialize(manager->stream_manager) != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("初始化流管理器失败");
        linx_stream_manager_destroy(manager->stream_manager);
        manager->stream_manager = NULL;
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    // 创建插件管理器
    linx_plugin_manager_config_t plugin_config;
    linx_plugin_manager_get_default_config(&plugin_config);
    plugin_config.max_plugins = manager->config.max_plugins;
    
    manager->plugin_manager = linx_plugin_manager_create((struct audio_manager*)manager, &plugin_config);
    if (!manager->plugin_manager) {
        LOG_ERROR("创建插件管理器失败");
        linx_stream_manager_deinitialize(manager->stream_manager);
        linx_stream_manager_destroy(manager->stream_manager);
        manager->stream_manager = NULL;
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    if (linx_plugin_manager_initialize(manager->plugin_manager) != LINX_AUDIO_SUCCESS) {
        LOG_ERROR("初始化插件管理器失败");
        linx_plugin_manager_destroy(manager->plugin_manager);
        manager->plugin_manager = NULL;
        linx_stream_manager_deinitialize(manager->stream_manager);
        linx_stream_manager_destroy(manager->stream_manager);
        manager->stream_manager = NULL;
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    LOG_DEBUG("流管理器创建和初始化完成");
    
    LOG_DEBUG("音频管理器组件初始化完成");
    return LINX_AUDIO_SUCCESS;
}

static void manager_cleanup_components(linx_audio_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    LOG_DEBUG("清理音频管理器组件");
    
    // 销毁插件管理器
    if (manager->plugin_manager) {
        linx_plugin_manager_deinitialize(manager->plugin_manager);
        linx_plugin_manager_destroy(manager->plugin_manager);
        manager->plugin_manager = NULL;
        LOG_DEBUG("插件管理器已销毁");
    }
    
    // 销毁流管理器
    if (manager->stream_manager) {
        linx_stream_manager_deinitialize(manager->stream_manager);
        linx_stream_manager_destroy(manager->stream_manager);
        manager->stream_manager = NULL;
        LOG_DEBUG("流管理器已销毁");
    }
    
    // 音频处理功能已集成到管理器中
    LOG_DEBUG("音频处理功能已清理");
    
    // 取消事件订阅
    if (manager->event_bus && manager->subscriber_id != 0) {
        linx_event_bus_unsubscribe(manager->event_bus, manager->subscriber_id);
        manager->subscriber_id = 0;
    }
    
    // 销毁事件总线
    if (manager->event_bus) {
        linx_event_bus_destroy(manager->event_bus);
        manager->event_bus = NULL;
        LOG_DEBUG("事件总线已销毁");
    }
}

static linx_audio_result_t manager_init_devices(linx_audio_manager_t* manager)
{
    if (!manager) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    LOG_DEBUG("初始化音频设备");
    
    // 初始化设备列表
    vector_linx_audio_device_info_t_init(&manager->input_devices);
    vector_linx_audio_device_info_t_init(&manager->output_devices);
    
    // 设备发现和初始化逻辑
    // 这里需要调用平台特定的驱动接口
    // 暂时创建一些默认设备作为示例
    
    // 创建默认输出设备
    linx_audio_device_info_t default_output = {
        .device_id = 1,
        .type = LINX_AUDIO_DEVICE_TYPE_PLAYBACK,
        .state = LINX_AUDIO_DEVICE_STATE_IDLE,
        .is_default = true,
        .default_params = {
            .format = LINX_AUDIO_FORMAT_S16LE,
            .sample_rate = 44100,
            .channels = 2,
            .bits_per_sample = 16,
            .frame_size = 4,
            .buffer_size = LINX_AUDIO_DEFAULT_BUFFER_SIZE
        },
        .driver_data = NULL
    };
    strncpy(default_output.name, "默认输出设备", sizeof(default_output.name) - 1);
    
    vector_linx_audio_device_info_t_push_back(&manager->output_devices, default_output);
    manager->default_output_device = 1;
    
    // 创建默认输入设备
    linx_audio_device_info_t default_input = {
        .device_id = 2,
        .type = LINX_AUDIO_DEVICE_TYPE_CAPTURE,
        .state = LINX_AUDIO_DEVICE_STATE_IDLE,
        .is_default = true,
        .default_params = {
            .format = LINX_AUDIO_FORMAT_S16LE,
            .sample_rate = 44100,
            .channels = 2,
            .bits_per_sample = 16,
            .frame_size = 4,
            .buffer_size = LINX_AUDIO_DEFAULT_BUFFER_SIZE
        },
        .driver_data = NULL
    };
    strncpy(default_input.name, "默认输入设备", sizeof(default_input.name) - 1);
    
    vector_linx_audio_device_info_t_push_back(&manager->input_devices, default_input);
    manager->default_input_device = 2;
    
    LOG_INFO("设备初始化完成，输出设备: %zu, 输入设备: %zu", 
             vector_linx_audio_device_info_t_size(&manager->output_devices),
             vector_linx_audio_device_info_t_size(&manager->input_devices));
    
    return LINX_AUDIO_SUCCESS;
}

static void manager_cleanup_devices(linx_audio_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    LOG_DEBUG("清理音频设备");
    
    // 清理输出设备
    if (vector_linx_audio_device_info_t_size(&manager->output_devices) > 0) {
        // 停止所有输出设备
        for (size_t i = 0; i < vector_linx_audio_device_info_t_size(&manager->output_devices); i++) {
            linx_audio_device_info_t* device = vector_linx_audio_device_info_t_at(&manager->output_devices, i);
            if (device && device->state == LINX_AUDIO_DEVICE_STATE_RUNNING) {
                device->state = LINX_AUDIO_DEVICE_STATE_IDLE;
                LOG_DEBUG("停止输出设备: %s (ID: %u)", device->name, device->device_id);
            }
        }
        vector_linx_audio_device_info_t_clear(&manager->output_devices);
    }
    
    // 清理输入设备
    if (vector_linx_audio_device_info_t_size(&manager->input_devices) > 0) {
        // 停止所有输入设备
        for (size_t i = 0; i < vector_linx_audio_device_info_t_size(&manager->input_devices); i++) {
            linx_audio_device_info_t* device = vector_linx_audio_device_info_t_at(&manager->input_devices, i);
            if (device && device->state == LINX_AUDIO_DEVICE_STATE_RUNNING) {
                device->state = LINX_AUDIO_DEVICE_STATE_IDLE;
                LOG_DEBUG("停止输入设备: %s (ID: %u)", device->name, device->device_id);
            }
        }
        vector_linx_audio_device_info_t_clear(&manager->input_devices);
    }
    
    // 清理输入设备列表
    vector_linx_audio_device_info_t_destroy(&manager->input_devices);
    
    // 清理输出设备列表
    vector_linx_audio_device_info_t_destroy(&manager->output_devices);
    
    // 重置默认设备
    manager->default_input_device = 0;
    manager->default_output_device = 0;
    
    LOG_DEBUG("设备清理完成");
}

static void manager_update_stats(linx_audio_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    pthread_mutex_lock(&manager->stats_mutex);
    
    // 更新设备统计
    manager->stats.active_devices = vector_linx_audio_device_info_t_size(&manager->input_devices) + 
                                   vector_linx_audio_device_info_t_size(&manager->output_devices);
    
    // 更新流统计
    manager->stats.active_streams = vector_linx_audio_stream_ptr_t_size(&manager->active_streams);
    
    // 更新插件统计信息
    if (manager->plugin_manager) {
        linx_plugin_manager_stats_t plugin_stats;
        if (linx_plugin_manager_get_stats(manager->plugin_manager, &plugin_stats) == LINX_AUDIO_SUCCESS) {
            manager->stats.loaded_plugins = plugin_stats.loaded_plugins;
        }
    }
    
    pthread_mutex_unlock(&manager->stats_mutex);
}

static void manager_event_handler(const linx_audio_event_t* event, void* user_data)
{
    linx_audio_manager_t* manager = (linx_audio_manager_t*)user_data;
    
    if (!manager || !event) {
        return;
    }
    
    LOG_DEBUG("处理音频事件: type=%d", event->type);
    
    // 根据事件类型进行处理
    switch (event->type) {
        case LINX_AUDIO_EVENT_DEVICE_ADDED:
            LOG_INFO("设备添加事件");
            manager_update_stats(manager);
            break;
            
        case LINX_AUDIO_EVENT_DEVICE_REMOVED:
            LOG_INFO("设备移除事件");
            manager_update_stats(manager);
            break;
            
        case LINX_AUDIO_EVENT_STREAM_CREATED:
            LOG_DEBUG("流启动事件");
            manager->stats.start_count++;
            break;
            
        case LINX_AUDIO_EVENT_STREAM_DESTROYED:
            LOG_DEBUG("流停止事件");
            manager->stats.stop_count++;
            break;
            
        case LINX_AUDIO_EVENT_ERROR:
            LOG_WARN("音频错误事件");
            manager->stats.error_count++;
            break;
            
        default:
            LOG_DEBUG("未处理的事件类型: %d", event->type);
            break;
    }
}