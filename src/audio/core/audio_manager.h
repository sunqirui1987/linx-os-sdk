/**
 * @file audio_manager.h
 * @brief LinxOS音频系统管理器接口
 * @details 音频系统的核心管理组件，负责音频设备、流和插件的统一管理
 */

#ifndef LINX_AUDIO_CORE_AUDIO_MANAGER_H
#define LINX_AUDIO_CORE_AUDIO_MANAGER_H

#include "types.h"
#include "event_bus.h"
#include "../../common/std/vector.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 向量类型声明
// ============================================================================

// 声明设备信息向量类型
VECTOR_DECLARE(linx_audio_device_info_t)

// 声明流指针向量类型
typedef linx_audio_stream_t* linx_audio_stream_ptr_t;
VECTOR_DECLARE(linx_audio_stream_ptr_t)

// ============================================================================
// 前向声明
// ============================================================================
// 前向声明
typedef struct linx_audio_manager linx_audio_manager_t;
typedef struct linx_stream_manager linx_stream_manager_t;
typedef struct linx_plugin_manager linx_plugin_manager_t;

// ============================================================================
// 音频管理器配置
// ============================================================================

/**
 * @brief 音频管理器配置结构
 */
typedef struct {
    uint32_t max_devices;           /**< 最大设备数量 */
    uint32_t max_streams;           /**< 最大流数量 */
    uint32_t max_plugins;           /**< 最大插件数量 */
    uint32_t buffer_size;           /**< 默认缓冲区大小 */
    uint32_t sample_rate;           /**< 默认采样率 */
    linx_audio_format_t format;     /**< 默认音频格式 */
    bool enable_event_bus;          /**< 是否启用事件总线 */
    bool enable_hot_plug;           /**< 是否启用热插拔 */
    bool enable_auto_routing;       /**< 是否启用自动路由 */
    char log_level[16];             /**< 日志级别 */
} linx_audio_manager_config_t;

// linx_audio_manager_state_t 已在 types.h 中定义

/**
 * @brief 音频管理器统计信息
 */
typedef struct {
    uint32_t active_devices;        /**< 活跃设备数量 */
    uint32_t active_streams;        /**< 活跃流数量 */
    uint32_t loaded_plugins;        /**< 已加载插件数量 */
    uint64_t total_frames_processed; /**< 总处理帧数 */
    uint64_t total_bytes_processed;  /**< 总处理字节数 */
    uint32_t buffer_underruns;      /**< 缓冲区下溢次数 */
    uint32_t buffer_overruns;       /**< 缓冲区上溢次数 */
    uint32_t error_count;           /**< 错误计数 */
    double cpu_usage;               /**< CPU使用率 */
    double memory_usage;            /**< 内存使用率 */
    
    // 时间戳字段
    time_t init_time;               /**< 初始化时间 */
    time_t start_time;              /**< 启动时间 */
    time_t stop_time;               /**< 停止时间 */
    
    // 计数字段
    uint32_t init_count;            /**< 初始化次数 */
    uint32_t start_count;           /**< 启动次数 */
    uint32_t stop_count;            /**< 停止次数 */
} linx_audio_manager_stats_t;

// ============================================================================
// 音频管理器核心接口
// ============================================================================

/**
 * @brief 创建音频管理器实例
 * @param config 初始配置，NULL使用默认配置
 * @return 音频管理器指针，失败返回NULL
 */
linx_audio_manager_t* linx_audio_manager_create(const linx_audio_manager_config_t* config);

/**
 * @brief 销毁音频管理器实例
 * @param manager 音频管理器指针
 */
void linx_audio_manager_destroy(linx_audio_manager_t* manager);

/**
 * @brief 初始化音频管理器
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_init(linx_audio_manager_t* manager);

/**
 * @brief 反初始化音频管理器
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_deinit(linx_audio_manager_t* manager);

/**
 * @brief 启动音频系统
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_start(linx_audio_manager_t* manager);

/**
 * @brief 停止音频系统
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_stop(linx_audio_manager_t* manager);

/**
 * @brief 暂停音频系统
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_pause(linx_audio_manager_t* manager);

/**
 * @brief 恢复音频系统
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_resume(linx_audio_manager_t* manager);

/**
 * @brief 获取音频管理器状态
 * @param manager 音频管理器指针
 * @return 当前状态
 */
linx_audio_manager_state_t linx_audio_manager_get_state(const linx_audio_manager_t* manager);

// ============================================================================
// 配置管理接口
// ============================================================================

/**
 * @brief 获取默认系统配置
 * @param config 配置结构体指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_default_config(linx_audio_manager_config_t* config);

/**
 * @brief 设置系统配置
 * @param manager 音频管理器指针
 * @param config 新的配置
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_set_config(linx_audio_manager_t* manager, 
                                                  const linx_audio_manager_config_t* config);

/**
 * @brief 获取当前系统配置
 * @param manager 音频管理器指针
 * @param config 配置结构体指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_config(const linx_audio_manager_t* manager, 
                                                  linx_audio_manager_config_t* config);

// ============================================================================
// 设备管理接口
// ============================================================================

/**
 * @brief 枚举音频设备
 * @param manager 音频管理器指针
 * @param device_type 设备类型
 * @param devices 设备信息数组指针
 * @param count 设备数量指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_enumerate_devices(const linx_audio_manager_t* manager,
                                                        linx_audio_device_type_t device_type,
                                                        linx_audio_device_info_t** devices,
                                                        uint32_t* count);

/**
 * @brief 获取默认设备
 * @param manager 音频管理器指针
 * @param device_type 设备类型
 * @param device_info 设备信息指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_default_device(const linx_audio_manager_t* manager,
                                                         linx_audio_device_type_t device_type,
                                                         linx_audio_device_info_t* device_info);

/**
 * @brief 设置默认设备
 * @param manager 音频管理器指针
 * @param device_type 设备类型
 * @param device_id 设备ID
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_set_default_device(linx_audio_manager_t* manager,
                                                         linx_audio_device_type_t device_type,
                                                         uint32_t device_id);

/**
 * @brief 检查设备是否可用
 * @param manager 音频管理器指针
 * @param device_id 设备ID
 * @param available 可用性指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_is_device_available(const linx_audio_manager_t* manager,
                                                          uint32_t device_id,
                                                          bool* available);

/**
 * @brief 获取设备信息
 * @param manager 音频管理器指针
 * @param device_id 设备ID
 * @param device_info 设备信息指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_device_info(const linx_audio_manager_t* manager,
                                                      uint32_t device_id,
                                                      linx_audio_device_info_t* device_info);

// ============================================================================
// 流管理接口
// ============================================================================

/**
 * @brief 创建音频流
 * @param manager 音频管理器指针
 * @param config 流配置
 * @param stream 流指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_create_stream(linx_audio_manager_t* manager,
                                                    const linx_audio_stream_config_t* config,
                                                    linx_audio_stream_t** stream);

/**
 * @brief 销毁音频流
 * @param manager 音频管理器指针
 * @param stream 流指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_destroy_stream(linx_audio_manager_t* manager,
                                                     linx_audio_stream_t* stream);

/**
 * @brief 获取活跃流数量
 * @param manager 音频管理器指针
 * @param count 流数量指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_active_stream_count(const linx_audio_manager_t* manager,
                                                              uint32_t* count);

/**
 * @brief 枚举活跃流
 * @param manager 音频管理器指针
 * @param streams 流数组指针
 * @param count 流数量指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_enumerate_streams(const linx_audio_manager_t* manager,
                                                        linx_audio_stream_t*** streams,
                                                        uint32_t* count);

// ============================================================================
// 事件管理接口
// ============================================================================

/**
 * @brief 获取事件总线
 * @param manager 音频管理器指针
 * @return 事件总线指针，失败返回NULL
 */
linx_event_bus_t* linx_audio_manager_get_event_bus(linx_audio_manager_t* manager);

/**
 * @brief 设置全局事件回调
 * @param manager 音频管理器指针
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_set_event_callback(linx_audio_manager_t* manager,
                                                         linx_audio_event_callback_t callback,
                                                         void* user_data);

/**
 * @brief 设置错误回调
 * @param manager 音频管理器指针
 * @param callback 错误回调函数
 * @param user_data 用户数据
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_set_error_callback(linx_audio_manager_t* manager,
                                                         linx_audio_error_callback_t callback,
                                                         void* user_data);

/**
 * @brief 发送事件
 * @param manager 音频管理器指针
 * @param event 事件指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_send_event(linx_audio_manager_t* manager,
                                                  const linx_audio_event_t* event);

// ============================================================================
// 统计信息接口
// ============================================================================

/**
 * @brief 获取系统统计信息
 * @param manager 音频管理器指针
 * @param stats 统计信息结构体指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_stats(const linx_audio_manager_t* manager,
                                                 linx_audio_manager_stats_t* stats);

/**
 * @brief 重置统计信息
 * @param manager 音频管理器指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_reset_stats(linx_audio_manager_t* manager);

// ============================================================================
// 状态查询接口
// ============================================================================

/**
 * @brief 检查管理器是否已初始化
 * @param manager 音频管理器指针
 * @return 是否已初始化
 */
bool linx_audio_manager_is_initialized(const linx_audio_manager_t* manager);

/**
 * @brief 检查管理器是否正在运行
 * @param manager 音频管理器指针
 * @return 是否正在运行
 */
bool linx_audio_manager_is_running(const linx_audio_manager_t* manager);

/**
 * @brief 检查系统健康状态
 * @param manager 音频管理器指针
 * @return 是否健康
 */
bool linx_audio_manager_is_healthy(const linx_audio_manager_t* manager);

// ============================================================================
// 版本信息接口
// ============================================================================

/**
 * @brief 获取管理器版本信息
 * @param major 主版本号指针
 * @param minor 次版本号指针
 * @param patch 补丁版本号指针
 * @return 操作结果
 */
linx_audio_result_t linx_audio_manager_get_version(uint32_t* major, uint32_t* minor, uint32_t* patch);

/**
 * @brief 获取管理器版本字符串
 * @return 版本字符串
 */
const char* linx_audio_manager_get_version_string(void);



#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_CORE_AUDIO_MANAGER_H