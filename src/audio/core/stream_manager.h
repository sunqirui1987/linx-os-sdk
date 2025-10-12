/**
 * @file stream_manager.h
 * @brief LinxOS音频流管理器
 * 
 * 流管理器负责：
 * 1. 音频流的生命周期管理
 * 2. 流的调度和优先级管理
 * 3. 流间的混音和路由
 * 4. 流的状态监控和统计
 * 5. 流的格式协商和转换
 */

#ifndef LINX_STREAM_MANAGER_H
#define LINX_STREAM_MANAGER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct linx_stream_manager linx_stream_manager_t;
typedef struct linx_audio_stream linx_audio_stream_t;
typedef struct linx_audio_device linx_audio_device_t;



/**
 * @brief 音频流优先级
 */
typedef enum {
    LINX_AUDIO_STREAM_PRIORITY_LOW = 0,     ///< 低优先级
    LINX_AUDIO_STREAM_PRIORITY_NORMAL,      ///< 普通优先级
    LINX_AUDIO_STREAM_PRIORITY_HIGH,        ///< 高优先级
    LINX_AUDIO_STREAM_PRIORITY_REALTIME     ///< 实时优先级
} linx_audio_stream_priority_t;



/**
 * @brief 音频流统计信息
 */
typedef struct {
    // 基本统计
    uint64_t total_frames_processed;   ///< 总处理帧数
    uint64_t total_bytes_processed;    ///< 总处理字节数
    uint32_t buffer_underruns;         ///< 缓冲区欠载次数
    uint32_t buffer_overruns;          ///< 缓冲区溢出次数
    
    // 时间统计
    uint64_t total_processing_time_us; ///< 总处理时间（微秒）
    uint32_t average_latency_frames;   ///< 平均延迟（帧数）
    uint32_t peak_latency_frames;      ///< 峰值延迟（帧数）
    
    // 质量统计
    uint32_t format_conversion_count;  ///< 格式转换次数
    uint32_t resampling_count;        ///< 重采样次数
    uint32_t dropped_frames;          ///< 丢弃帧数
    
    // 错误统计
    uint32_t callback_errors;         ///< 回调错误次数
    uint32_t device_errors;           ///< 设备错误次数
    uint32_t format_errors;           ///< 格式错误次数
} linx_audio_stream_stats_t;

/**
 * @brief 音频流结构
 */
struct linx_audio_stream {
    // 基本信息
    uint32_t id;                      ///< 流ID
    char name[64];                    ///< 流名称
    linx_audio_stream_type_t type;         ///< 流类型
    linx_audio_stream_state_t state;   ///< 流状态
    linx_audio_stream_priority_t priority; ///< 流优先级
    
    // 配置和统计
    linx_audio_stream_config_t config;     ///< 流配置
    linx_audio_stream_stats_t stats;       ///< 统计信息
    
    // 格式信息
    linx_audio_format_info_t format;       ///< 当前格式
    linx_audio_format_info_t native_format; ///< 原生格式
    
    // 设备引用
    linx_audio_device_t* input_device;     ///< 输入设备
    linx_audio_device_t* output_device;    ///< 输出设备
    
    // 缓冲区管理
    linx_audio_buffer_t** buffers;         ///< 缓冲区数组
    uint32_t buffer_count;            ///< 缓冲区数量
    uint32_t current_buffer;          ///< 当前缓冲区索引
    
    // 同步对象
    pthread_mutex_t mutex;            ///< 互斥锁
    pthread_cond_t condition;         ///< 条件变量
    
    // 处理线程
    pthread_t processing_thread;      ///< 处理线程
    bool thread_running;              ///< 线程运行标志
    
    // 管理器引用
    linx_stream_manager_t* manager;        ///< 流管理器引用
    
    // 私有数据
    void* private_data;               ///< 私有数据
};

/**
 * @brief 流管理器配置
 */
typedef struct {
    // 基本配置
    uint32_t max_streams;             ///< 最大流数量
    uint32_t max_concurrent_streams;  ///< 最大并发流数量
    
    // 调度配置
    bool enable_priority_scheduling;  ///< 启用优先级调度
    bool enable_load_balancing;      ///< 启用负载均衡
    uint32_t scheduler_interval_ms;   ///< 调度器间隔（毫秒）
    
    // 混音配置
    bool enable_automatic_mixing;     ///< 启用自动混音
    uint32_t mixer_buffer_size;       ///< 混音器缓冲区大小
    linx_audio_format_t mixer_format;      ///< 混音器格式
    
    // 线程配置
    linx_audio_thread_priority_t thread_priority;  ///< 线程优先级
    bool enable_realtime_scheduling;          ///< 启用实时调度
    
    // 调试配置
    bool enable_debug_logging;        ///< 启用调试日志
    bool enable_performance_monitoring;  ///< 启用性能监控
} linx_stream_manager_config_t;

/**
 * @brief 流管理器统计信息
 */
typedef struct {
    // 流统计
    uint32_t total_streams;           ///< 总流数量
    uint32_t active_streams;          ///< 活跃流数量
    uint32_t paused_streams;          ///< 暂停流数量
    
    // 类型统计
    uint32_t playback_streams;        ///< 播放流数量
    uint32_t capture_streams;         ///< 录制流数量
    uint32_t duplex_streams;          ///< 双工流数量
    
    // 性能统计
    uint64_t total_frames_processed;  ///< 总处理帧数
    uint32_t scheduler_runs;          ///< 调度器运行次数
    uint32_t mixing_operations;       ///< 混音操作次数
    
    // 错误统计
    uint32_t stream_creation_failures; ///< 流创建失败次数
    uint32_t scheduling_errors;        ///< 调度错误次数
    uint32_t mixing_errors;            ///< 混音错误次数
} linx_stream_manager_stats_t;

/**
 * @brief 流管理器结构体
 */
struct linx_stream_manager {
    // 基本信息
    uint32_t id;                      ///< 管理器ID
    char name[64];                    ///< 管理器名称
    
    // 配置和统计
    linx_stream_manager_config_t config;   ///< 管理器配置
    linx_stream_manager_stats_t stats;     ///< 统计信息
    
    // 流管理
    linx_audio_stream_t** streams;         ///< 流数组
    uint32_t stream_count;            ///< 当前流数量
    uint32_t next_stream_id;          ///< 下一个流ID
    
    // 调度器线程
    pthread_t scheduler_thread;       ///< 调度器线程
    bool scheduler_running;           ///< 调度器运行标志
    
    // 同步对象
    pthread_mutex_t mutex;            ///< 互斥锁
    pthread_cond_t condition;         ///< 条件变量
    
    // 管理器引用
    struct linx_audio_manager* manager; ///< 音频管理器引用
    struct event_bus* event_bus;        ///< 事件总线引用
    
    // 私有数据
    void* private_data;               ///< 私有数据
};

// ============================================================================
// 核心接口
// ============================================================================

/**
 * @brief 创建流管理器
 * @param manager 音频管理器
 * @param config 配置参数
 * @return 流管理器实例，失败返回NULL
 */
linx_stream_manager_t* linx_stream_manager_create(struct linx_audio_manager* manager,
                                       const linx_stream_manager_config_t* config);

/**
 * @brief 销毁流管理器
 * @param manager 流管理器
 */
void linx_stream_manager_destroy(linx_stream_manager_t* manager);

/**
 * @brief 初始化流管理器
 * @param manager 流管理器
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_initialize(linx_stream_manager_t* manager);

/**
 * @brief 反初始化流管理器
 * @param manager 流管理器
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_deinitialize(linx_stream_manager_t* manager);

/**
 * @brief 启动流管理器
 * @param manager 流管理器
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_start(linx_stream_manager_t* manager);

/**
 * @brief 停止流管理器
 * @param manager 流管理器
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_stop(linx_stream_manager_t* manager);

// ============================================================================
// 流管理接口
// ============================================================================

/**
 * @brief 创建音频流
 * @param manager 流管理器
 * @param config 流配置
 * @param stream 输出流指针
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_create_stream(linx_stream_manager_t* manager,
                                           const linx_audio_stream_config_t* config,
                                           linx_audio_stream_t** stream);

/**
 * @brief 销毁音频流
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_destroy_stream(linx_stream_manager_t* manager,
                                            linx_audio_stream_t* stream);

/**
 * @brief 启动音频流
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_start_stream(linx_stream_manager_t* manager,
                                          linx_audio_stream_t* stream);

/**
 * @brief 停止音频流
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_stop_stream(linx_stream_manager_t* manager,
                                         linx_audio_stream_t* stream);

/**
 * @brief 暂停音频流
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_pause_stream(linx_stream_manager_t* manager,
                                          linx_audio_stream_t* stream);

/**
 * @brief 恢复音频流
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_resume_stream(linx_stream_manager_t* manager,
                                           linx_audio_stream_t* stream);

// ============================================================================
// 查询接口
// ============================================================================

/**
 * @brief 获取所有流
 * @param manager 流管理器
 * @param streams 输出流数组
 * @param count 输出流数量
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_streams(linx_stream_manager_t* manager,
                                         linx_audio_stream_t*** streams,
                                         uint32_t* count);

/**
 * @brief 根据ID查找流
 * @param manager 流管理器
 * @param stream_id 流ID
 * @return 音频流，未找到返回NULL
 */
linx_audio_stream_t* linx_stream_manager_find_stream(linx_stream_manager_t* manager,
                                          uint32_t stream_id);

/**
 * @brief 根据名称查找流
 * @param manager 流管理器
 * @param name 流名称
 * @return 音频流，未找到返回NULL
 */
linx_audio_stream_t* linx_stream_manager_find_stream_by_name(linx_stream_manager_t* manager,
                                                  const char* name);

// ============================================================================
// 配置接口
// ============================================================================

/**
 * @brief 获取默认流配置
 * @param type 流类型
 * @param config 输出配置
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_default_stream_config(linx_audio_stream_type_t type,
                                                       linx_audio_stream_config_t* config);

/**
 * @brief 设置流配置
 * @param manager 流管理器
 * @param stream 音频流
 * @param config 流配置
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_set_stream_config(linx_stream_manager_t* manager,
                                               linx_audio_stream_t* stream,
                                               const linx_audio_stream_config_t* config);

/**
 * @brief 获取流配置
 * @param manager 流管理器
 * @param stream 音频流
 * @param config 流配置（输出）
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_stream_config(linx_stream_manager_t* manager,
                                               linx_audio_stream_t* stream,
                                               linx_audio_stream_config_t* config);

// ============================================================================
// 统计接口
// ============================================================================

/**
 * @brief 获取管理器统计信息
 * @param manager 流管理器
 * @param stats 输出统计信息
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_stats(linx_stream_manager_t* manager,
                                       linx_stream_manager_stats_t* stats);

/**
 * @brief 重置管理器统计信息
 * @param manager 流管理器
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_reset_stats(linx_stream_manager_t* manager);

/**
 * @brief 获取流统计信息
 * @param manager 流管理器
 * @param stream 音频流
 * @param stats 输出统计信息
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_stream_stats(linx_stream_manager_t* manager,
                                              linx_audio_stream_t* stream,
                                              linx_audio_stream_stats_t* stats);

/**
 * @brief 重置流统计信息
 * @param manager 流管理器
 * @param stream 音频流
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_reset_stream_stats(linx_stream_manager_t* manager,
                                                linx_audio_stream_t* stream);

// =============================================================================
// 实用工具函数
// =============================================================================

/**
 * @brief 流类型转字符串
 * @param type 流类型
 * @return 类型字符串
 */
const char* linx_audio_stream_type_to_string(linx_audio_stream_type_t type);

/**
 * @brief 流优先级转字符串
 * @param priority 流优先级
 * @return 优先级字符串
 */
const char* linx_audio_stream_priority_to_string(linx_audio_stream_priority_t priority);

/**
 * @brief 获取默认管理器配置
 * @param config 输出配置
 * @return 操作结果
 */
linx_audio_result_t linx_stream_manager_get_default_config(linx_stream_manager_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // LINX_STREAM_MANAGER_H