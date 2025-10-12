#ifndef LINX_AUDIO_PIPELINE_AUDIO_PIPELINE_H
#define LINX_AUDIO_PIPELINE_AUDIO_PIPELINE_H

#include "../core/types.h"
#include "../plugins/plugin_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 管道节点类型
// ============================================================================

typedef enum {
    PIPELINE_NODE_TYPE_SOURCE = 0,     // 音频源节点
    PIPELINE_NODE_TYPE_SINK = 1,       // 音频汇节点
    PIPELINE_NODE_TYPE_PROCESSOR = 2,  // 处理节点
    PIPELINE_NODE_TYPE_MIXER = 3,      // 混音节点
    PIPELINE_NODE_TYPE_SPLITTER = 4,   // 分离节点
    PIPELINE_NODE_TYPE_CONVERTER = 5,  // 格式转换节点
    PIPELINE_NODE_TYPE_PLUGIN = 6      // 插件节点
} pipeline_node_type_t;

// ============================================================================
// 管道节点状态
// ============================================================================

typedef enum {
    PIPELINE_NODE_STATE_IDLE = 0,
    PIPELINE_NODE_STATE_PREPARED = 1,
    PIPELINE_NODE_STATE_RUNNING = 2,
    PIPELINE_NODE_STATE_PAUSED = 3,
    PIPELINE_NODE_STATE_ERROR = 4
} pipeline_node_state_t;

// ============================================================================
// 管道状态
// ============================================================================

typedef enum {
    PIPELINE_STATE_IDLE = 0,
    PIPELINE_STATE_PREPARED = 1,
    PIPELINE_STATE_RUNNING = 2,
    PIPELINE_STATE_PAUSED = 3,
    PIPELINE_STATE_STOPPED = 4,
    PIPELINE_STATE_ERROR = 5
} pipeline_state_t;

// ============================================================================
// 管道节点连接
// ============================================================================

typedef struct pipeline_connection {
    struct pipeline_node* source_node;
    uint32_t source_port;
    struct pipeline_node* sink_node;
    uint32_t sink_port;
    audio_format_info_t format;
    bool active;
    struct pipeline_connection* next;
} pipeline_connection_t;

// ============================================================================
// 管道节点接口
// ============================================================================

typedef struct pipeline_node pipeline_node_t;

typedef struct {
    // 生命周期
    audio_result_t (*initialize)(pipeline_node_t* node, const void* config);
    audio_result_t (*deinitialize)(pipeline_node_t* node);
    audio_result_t (*prepare)(pipeline_node_t* node);
    audio_result_t (*start)(pipeline_node_t* node);
    audio_result_t (*stop)(pipeline_node_t* node);
    audio_result_t (*pause)(pipeline_node_t* node);
    audio_result_t (*resume)(pipeline_node_t* node);
    
    // 数据处理
    audio_result_t (*process)(pipeline_node_t* node);
    audio_result_t (*push_buffer)(pipeline_node_t* node, uint32_t port, const audio_buffer_t* buffer);
    audio_result_t (*pull_buffer)(pipeline_node_t* node, uint32_t port, audio_buffer_t* buffer);
    
    // 连接管理
    audio_result_t (*connect_input)(pipeline_node_t* node, uint32_t port, pipeline_node_t* source);
    audio_result_t (*connect_output)(pipeline_node_t* node, uint32_t port, pipeline_node_t* sink);
    audio_result_t (*disconnect_input)(pipeline_node_t* node, uint32_t port);
    audio_result_t (*disconnect_output)(pipeline_node_t* node, uint32_t port);
    
    // 格式协商
    audio_result_t (*set_input_format)(pipeline_node_t* node, uint32_t port, const audio_format_info_t* format);
    audio_result_t (*set_output_format)(pipeline_node_t* node, uint32_t port, const audio_format_info_t* format);
    audio_result_t (*get_input_format)(pipeline_node_t* node, uint32_t port, audio_format_info_t* format);
    audio_result_t (*get_output_format)(pipeline_node_t* node, uint32_t port, audio_format_info_t* format);
    
    // 状态查询
    pipeline_node_state_t (*get_state)(pipeline_node_t* node);
    audio_result_t (*get_latency)(pipeline_node_t* node, uint32_t* latency_frames);
    
    // 配置
    audio_result_t (*set_property)(pipeline_node_t* node, const char* name, const void* value, size_t size);
    audio_result_t (*get_property)(pipeline_node_t* node, const char* name, void* value, size_t* size);
    
    // 清理
    void (*destroy)(pipeline_node_t* node);
} pipeline_node_vtable_t;

// ============================================================================
// 管道节点结构体
// ============================================================================

struct pipeline_node {
    // 虚函数表
    const pipeline_node_vtable_t* vtable;
    
    // 基本信息
    uint32_t id;
    char* name;
    pipeline_node_type_t type;
    pipeline_node_state_t state;
    
    // 端口信息
    uint32_t input_port_count;
    uint32_t output_port_count;
    audio_format_info_t* input_formats;
    audio_format_info_t* output_formats;
    
    // 连接信息
    pipeline_node_t** input_nodes;
    pipeline_node_t** output_nodes;
    uint32_t* input_ports;
    uint32_t* output_ports;
    
    // 缓冲区
    audio_buffer_t** input_buffers;
    audio_buffer_t** output_buffers;
    
    // 插件引用（如果是插件节点）
    linx_plugin_base_t* plugin;
    
    // 配置数据
    void* config_data;
    size_t config_size;
    
    // 统计信息
    struct {
        uint64_t buffers_processed;
        uint64_t processing_time_us;
        uint32_t underrun_count;
        uint32_t overrun_count;
        uint32_t error_count;
    } stats;
    
    // 事件回调
    audio_event_callback_t event_callback;
    void* event_user_data;
    
    // 私有数据
    void* private_data;
    
    // 管道引用
    struct audio_pipeline* pipeline;
};

// ============================================================================
// 音频管道结构体
// ============================================================================

struct audio_pipeline {
    // 基本信息
    uint32_t id;
    char* name;
    pipeline_state_t state;
    
    // 节点管理
    pipeline_node_t** nodes;
    uint32_t node_count;
    uint32_t node_capacity;
    uint32_t next_node_id;
    
    // 连接管理
    pipeline_connection_t* connections;
    uint32_t connection_count;
    
    // 处理线程
    void* thread;
    bool thread_running;
    bool thread_should_stop;
    
    // 同步对象
    void* mutex;
    void* condition;
    
    // 配置
    struct {
        uint32_t buffer_size;
        uint32_t buffer_count;
        audio_thread_priority_t thread_priority;
        bool realtime_processing;
        uint32_t processing_interval_us;
    } config;
    
    // 统计信息
    struct {
        uint64_t cycles_processed;
        uint64_t total_processing_time_us;
        uint64_t max_processing_time_us;
        uint32_t underrun_count;
        uint32_t overrun_count;
        uint32_t error_count;
    } stats;
    
    // 事件回调
    audio_event_callback_t event_callback;
    void* event_user_data;
    
    // 管理器引用
    struct audio_manager* manager;
    
    // 私有数据
    void* private_data;
};

// ============================================================================
// 管道配置
// ============================================================================

typedef struct {
    const char* name;
    uint32_t buffer_size;
    uint32_t buffer_count;
    audio_thread_priority_t thread_priority;
    bool realtime_processing;
    uint32_t processing_interval_us;
} audio_pipeline_config_t;

// ============================================================================
// 节点配置
// ============================================================================

typedef struct {
    const char* name;
    pipeline_node_type_t type;
    uint32_t input_port_count;
    uint32_t output_port_count;
    const void* config_data;
    size_t config_size;
} pipeline_node_config_t;

// ============================================================================
// 音频管道API
// ============================================================================

/**
 * @brief 创建音频管道
 * @param manager 音频管理器
 * @param config 管道配置
 * @return 管道指针，失败返回NULL
 */
audio_pipeline_t* audio_pipeline_create(struct audio_manager* manager,
                                       const audio_pipeline_config_t* config);

/**
 * @brief 销毁音频管道
 * @param pipeline 管道指针
 */
void audio_pipeline_destroy(audio_pipeline_t* pipeline);

/**
 * @brief 准备管道
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_prepare(audio_pipeline_t* pipeline);

/**
 * @brief 启动管道
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_start(audio_pipeline_t* pipeline);

/**
 * @brief 停止管道
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_stop(audio_pipeline_t* pipeline);

/**
 * @brief 暂停管道
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_pause(audio_pipeline_t* pipeline);

/**
 * @brief 恢复管道
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_resume(audio_pipeline_t* pipeline);

// ============================================================================
// 节点管理
// ============================================================================

/**
 * @brief 添加节点到管道
 * @param pipeline 管道指针
 * @param config 节点配置
 * @param node 节点指针的指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_node(audio_pipeline_t* pipeline,
                                      const pipeline_node_config_t* config,
                                      pipeline_node_t** node);

/**
 * @brief 从管道移除节点
 * @param pipeline 管道指针
 * @param node 节点指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_remove_node(audio_pipeline_t* pipeline,
                                         pipeline_node_t* node);

/**
 * @brief 根据名称查找节点
 * @param pipeline 管道指针
 * @param name 节点名称
 * @return 节点指针，未找到返回NULL
 */
pipeline_node_t* audio_pipeline_find_node(audio_pipeline_t* pipeline,
                                         const char* name);

/**
 * @brief 根据ID查找节点
 * @param pipeline 管道指针
 * @param id 节点ID
 * @return 节点指针，未找到返回NULL
 */
pipeline_node_t* audio_pipeline_find_node_by_id(audio_pipeline_t* pipeline,
                                               uint32_t id);

/**
 * @brief 获取所有节点
 * @param pipeline 管道指针
 * @param nodes 节点数组指针
 * @param count 节点数量指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_nodes(audio_pipeline_t* pipeline,
                                       pipeline_node_t*** nodes,
                                       uint32_t* count);

// ============================================================================
// 连接管理
// ============================================================================

/**
 * @brief 连接两个节点
 * @param pipeline 管道指针
 * @param source_node 源节点
 * @param source_port 源端口
 * @param sink_node 汇节点
 * @param sink_port 汇端口
 * @return 操作结果
 */
audio_result_t audio_pipeline_connect_nodes(audio_pipeline_t* pipeline,
                                           pipeline_node_t* source_node,
                                           uint32_t source_port,
                                           pipeline_node_t* sink_node,
                                           uint32_t sink_port);

/**
 * @brief 断开节点连接
 * @param pipeline 管道指针
 * @param source_node 源节点
 * @param source_port 源端口
 * @param sink_node 汇节点
 * @param sink_port 汇端口
 * @return 操作结果
 */
audio_result_t audio_pipeline_disconnect_nodes(audio_pipeline_t* pipeline,
                                              pipeline_node_t* source_node,
                                              uint32_t source_port,
                                              pipeline_node_t* sink_node,
                                              uint32_t sink_port);

/**
 * @brief 获取所有连接
 * @param pipeline 管道指针
 * @param connections 连接数组指针
 * @param count 连接数量指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_connections(audio_pipeline_t* pipeline,
                                             pipeline_connection_t*** connections,
                                             uint32_t* count);

// ============================================================================
// 便捷节点创建函数
// ============================================================================

/**
 * @brief 添加输入节点
 * @param pipeline 管道指针
 * @param device_name 设备名称
 * @param node 节点指针的指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_input_node(audio_pipeline_t* pipeline,
                                            const char* device_name,
                                            pipeline_node_t** node);

/**
 * @brief 添加输出节点
 * @param pipeline 管道指针
 * @param device_name 设备名称
 * @param node 节点指针的指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_output_node(audio_pipeline_t* pipeline,
                                             const char* device_name,
                                             pipeline_node_t** node);

/**
 * @brief 添加插件节点
 * @param pipeline 管道指针
 * @param plugin_name 插件名称
 * @param config 插件配置
 * @param node 节点指针的指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_plugin_node(audio_pipeline_t* pipeline,
                                             const char* plugin_name,
                                             const linx_plugin_config_t* config,
                                             pipeline_node_t** node);

/**
 * @brief 添加混音节点
 * @param pipeline 管道指针
 * @param input_count 输入数量
 * @param node 节点指针的指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_add_mixer_node(audio_pipeline_t* pipeline,
                                            uint32_t input_count,
                                            pipeline_node_t** node);

// ============================================================================
// 状态和统计
// ============================================================================

/**
 * @brief 获取管道状态
 * @param pipeline 管道指针
 * @return 管道状态
 */
pipeline_state_t audio_pipeline_get_state(const audio_pipeline_t* pipeline);

/**
 * @brief 获取管道统计信息
 * @param pipeline 管道指针
 * @param stats 统计信息指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_stats(const audio_pipeline_t* pipeline, void* stats);

/**
 * @brief 重置管道统计信息
 * @param pipeline 管道指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_reset_stats(audio_pipeline_t* pipeline);

/**
 * @brief 获取管道延迟
 * @param pipeline 管道指针
 * @param latency_frames 延迟帧数指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_latency(const audio_pipeline_t* pipeline,
                                         uint32_t* latency_frames);

// ============================================================================
// 事件处理
// ============================================================================

/**
 * @brief 设置管道事件回调
 * @param pipeline 管道指针
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return 操作结果
 */
audio_result_t audio_pipeline_set_event_callback(audio_pipeline_t* pipeline,
                                                audio_event_callback_t callback,
                                                void* user_data);

/**
 * @brief 发送管道事件
 * @param pipeline 管道指针
 * @param event 事件指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_send_event(audio_pipeline_t* pipeline,
                                        const audio_event_t* event);

// ============================================================================
// 配置管理
// ============================================================================

/**
 * @brief 获取默认管道配置
 * @param config 配置指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_default_config(audio_pipeline_config_t* config);

/**
 * @brief 设置管道配置
 * @param pipeline 管道指针
 * @param config 新配置
 * @return 操作结果
 */
audio_result_t audio_pipeline_set_config(audio_pipeline_t* pipeline,
                                        const audio_pipeline_config_t* config);

/**
 * @brief 获取管道配置
 * @param pipeline 管道指针
 * @param config 配置指针
 * @return 操作结果
 */
audio_result_t audio_pipeline_get_config(const audio_pipeline_t* pipeline,
                                        audio_pipeline_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_PIPELINE_AUDIO_PIPELINE_H