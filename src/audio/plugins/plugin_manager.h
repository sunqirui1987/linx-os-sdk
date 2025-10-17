/**
 * @file plugin_manager.h
 * @brief LinxOS音频插件管理器
 * 
 * 插件管理器负责：
 * 1. 插件的动态加载和卸载
 * 2. 插件的注册和发现
 * 3. 插件的生命周期管理
 * 4. 插件间的依赖管理
 * 5. 插件的配置和参数管理
 */

#ifndef LINX_PLUGIN_MANAGER_H
#define LINX_PLUGIN_MANAGER_H

#include "plugin_interface.h"
#include "../core/types.h"
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct linx_plugin_manager linx_plugin_manager_t;
typedef struct linx_plugin_registry linx_plugin_registry_t;
typedef struct linx_plugin_loader linx_plugin_loader_t;

/**
 * @brief 插件加载方式
 */
typedef enum {
    PLUGIN_LOAD_MODE_STATIC = 0,      ///< 静态加载
    PLUGIN_LOAD_MODE_DYNAMIC,         ///< 动态加载
    PLUGIN_LOAD_MODE_LAZY,            ///< 延迟加载
    PLUGIN_LOAD_MODE_ON_DEMAND        ///< 按需加载
} linx_plugin_load_mode_t;

/**
 * @brief 插件状态
 */
typedef enum {
    PLUGIN_STATUS_UNLOADED = 0,       ///< 未加载
    PLUGIN_STATUS_LOADING,            ///< 加载中
    PLUGIN_STATUS_LOADED,             ///< 已加载
    PLUGIN_STATUS_INITIALIZING,       ///< 初始化中
    PLUGIN_STATUS_READY,              ///< 就绪
    PLUGIN_STATUS_RUNNING,            ///< 运行中
    PLUGIN_STATUS_PAUSED,             ///< 暂停
    PLUGIN_STATUS_ERROR,              ///< 错误状态
    PLUGIN_STATUS_UNLOADING           ///< 卸载中
} linx_plugin_status_t;

/**
 * @brief 插件信息
 */
typedef struct {
    // 基本信息
    char name[64];                    ///< 插件名称
    char path[256];                   ///< 插件路径
    linx_audio_plugin_type_t type;          ///< 插件类型
    linx_plugin_status_t status;      ///< 插件状态
    
    // 版本信息
    linx_plugin_version_t version;    ///< 插件版本
    linx_plugin_version_t api_version; ///< API版本
    
    // 元数据
    linx_plugin_metadata_t metadata;  ///< 插件元数据
    
    // 加载信息
    linx_plugin_load_mode_t load_mode; ///< 加载模式
    uint32_t reference_count;         ///< 引用计数
    uint64_t load_time;               ///< 加载时间
    
    // 依赖信息
    char** dependencies;              ///< 依赖列表
    uint32_t dependency_count;        ///< 依赖数量
    
    // 实例信息
    linx_plugin_base_t* instance;     ///< 插件实例
    void* library_handle;             ///< 库句柄
} linx_plugin_info_t;

/**
 * @brief 插件注册表
 */
struct linx_plugin_registry {
    // 插件信息
    linx_plugin_info_t** plugins;     ///< 插件信息数组
    uint32_t plugin_count;            ///< 插件数量
    uint32_t max_plugins;             ///< 最大插件数量
    
    // 索引表
    void* name_index;                 ///< 名称索引（哈希表）
    void* type_index;                 ///< 类型索引
    
    // 同步对象
    pthread_mutex_t mutex;            ///< 互斥锁
};

/**
 * @brief 插件加载器配置
 */
typedef struct {
    // 搜索路径
    char** search_paths;              ///< 搜索路径数组
    uint32_t search_path_count;       ///< 搜索路径数量
    
    // 加载配置
    linx_plugin_load_mode_t default_load_mode;  ///< 默认加载模式
    bool enable_lazy_loading;         ///< 启用延迟加载
    bool enable_dependency_resolution; ///< 启用依赖解析
    
    // 安全配置
    bool enable_signature_verification; ///< 启用签名验证
    bool enable_sandbox;              ///< 启用沙箱
    
    // 调试配置
    bool enable_debug_logging;        ///< 启用调试日志
    bool enable_performance_monitoring; ///< 启用性能监控
} linx_plugin_loader_config_t;

/**
 * @brief 插件加载器
 */
struct linx_plugin_loader {
    // 配置
    linx_plugin_loader_config_t config; ///< 加载器配置
    
    // 搜索路径
    char** search_paths;              ///< 搜索路径
    uint32_t search_path_count;       ///< 搜索路径数量
    
    // 加载缓存
    void* load_cache;                 ///< 加载缓存
    
    // 同步对象
    pthread_mutex_t mutex;            ///< 互斥锁
};

/**
 * @brief 插件管理器配置
 */
typedef struct {
    // 基本配置
    uint32_t max_plugins;             ///< 最大插件数量
    uint32_t max_instances;           ///< 最大实例数量
    
    // 加载器配置
    linx_plugin_loader_config_t loader_config;  ///< 加载器配置
    
    // 性能配置
    bool enable_plugin_pooling;       ///< 启用插件池
    uint32_t plugin_pool_size;        ///< 插件池大小
    
    // 监控配置
    bool enable_health_monitoring;    ///< 启用健康监控
    uint32_t health_check_interval_ms; ///< 健康检查间隔
    
    // 调试配置
    bool enable_debug_logging;        ///< 启用调试日志
    bool enable_performance_profiling; ///< 启用性能分析
} linx_plugin_manager_config_t;

/**
 * @brief 插件管理器统计信息
 */
typedef struct {
    // 基本统计
    uint32_t total_plugins;           ///< 总插件数量
    uint32_t loaded_plugins;          ///< 已加载插件数量
    uint32_t active_instances;        ///< 活跃实例数量
    
    // 类型统计
    uint32_t codec_plugins;           ///< 编解码器插件数量
    uint32_t processor_plugins;       ///< 处理器插件数量
    uint32_t effect_plugins;          ///< 效果插件数量
    uint32_t analyzer_plugins;        ///< 分析器插件数量
    
    // 性能统计
    uint64_t total_load_time_us;      ///< 总加载时间（微秒）
    uint64_t total_processing_time_us; ///< 总处理时间（微秒）
    uint32_t load_operations;         ///< 加载操作次数
    uint32_t unload_operations;       ///< 卸载操作次数
    
    // 错误统计
    uint32_t load_failures;           ///< 加载失败次数
    uint32_t initialization_failures; ///< 初始化失败次数
    uint32_t runtime_errors;          ///< 运行时错误次数
} linx_plugin_manager_stats_t;

/**
 * @brief 插件管理器结构
 */
struct linx_plugin_manager {
    // 基本信息
    uint32_t id;                      ///< 管理器ID
    char name[64];                    ///< 管理器名称
    
    // 配置和统计
    linx_plugin_manager_config_t config; ///< 管理器配置
    linx_plugin_manager_stats_t stats; ///< 统计信息
    
    // 核心组件
    linx_plugin_registry_t* registry; ///< 插件注册表
    linx_plugin_loader_t* loader;     ///< 插件加载器
    
    // 实例管理
    linx_plugin_base_t** instances;   ///< 插件实例数组
    uint32_t instance_count;          ///< 实例数量
    uint32_t next_instance_id;        ///< 下一个实例ID
    
    // 监控线程
    pthread_t monitor_thread;         ///< 监控线程
    bool monitor_running;             ///< 监控运行标志
    
    // 同步对象
    pthread_mutex_t mutex;            ///< 互斥锁
    pthread_cond_t condition;         ///< 条件变量
    
    // 组件引用
    struct audio_manager* manager;    ///< 音频管理器引用
    struct event_bus* event_bus;      ///< 事件总线引用
    
    // 内部数据
    void* private_data;               ///< 私有数据
};

// =============================================================================
// 插件管理器核心API
// =============================================================================

/**
 * @brief 创建插件管理器
 * @param manager 音频管理器
 * @param config 管理器配置
 * @return 插件管理器实例，失败返回NULL
 */
linx_plugin_manager_t* linx_plugin_manager_create(struct audio_manager* manager,
                                                 const linx_plugin_manager_config_t* config);

/**
 * @brief 销毁插件管理器
 * @param manager 插件管理器
 */
void linx_plugin_manager_destroy(linx_plugin_manager_t* manager);

/**
 * @brief 初始化插件管理器
 * @param manager 插件管理器
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_initialize(linx_plugin_manager_t* manager);

/**
 * @brief 反初始化插件管理器
 * @param manager 插件管理器
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_deinitialize(linx_plugin_manager_t* manager);

/**
 * @brief 启动插件管理器
 * @param manager 插件管理器
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_start(linx_plugin_manager_t* manager);

/**
 * @brief 停止插件管理器
 * @param manager 插件管理器
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_stop(linx_plugin_manager_t* manager);

// =============================================================================
// 插件加载管理API
// =============================================================================



// =============================================================================
// 插件实例管理API
// =============================================================================

/**
 * @brief 创建插件实例
 * @param manager 插件管理器
 * @param name 插件名称
 * @param config 插件配置
 * @param instance 输出插件实例
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_create_instance(linx_plugin_manager_t* manager,
                                                       const char* name,
                                                       const linx_plugin_config_t* config,
                                                       linx_plugin_base_t** instance);

/**
 * @brief 销毁插件实例
 * @param manager 插件管理器
 * @param instance 插件实例
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_destroy_instance(linx_plugin_manager_t* manager,
                                                        linx_plugin_base_t* instance);

// =============================================================================
// 配置和统计API
// =============================================================================

/**
 * @brief 获取默认管理器配置
 * @param config 输出配置
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_get_default_config(linx_plugin_manager_config_t* config);

/**
 * @brief 获取管理器统计信息
 * @param manager 插件管理器
 * @param stats 输出统计信息
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_manager_get_stats(linx_plugin_manager_t* manager,
                                                 linx_plugin_manager_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // LINX_PLUGIN_MANAGER_H