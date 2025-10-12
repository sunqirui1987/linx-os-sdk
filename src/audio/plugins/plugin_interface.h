#ifndef LINX_AUDIO_PLUGINS_PLUGIN_INTERFACE_H
#define LINX_AUDIO_PLUGINS_PLUGIN_INTERFACE_H

#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 插件版本信息
// ============================================================================

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* build;
} linx_plugin_version_t;

// ============================================================================
// 插件元数据
// ============================================================================

typedef struct {
    const char* name;
    const char* description;
    const char* author;
    const char* license;
    linx_plugin_version_t version;
    linx_plugin_version_t api_version;
    linx_audio_plugin_type_t type;
    uint32_t capabilities;
    const char** supported_formats;
    uint32_t format_count;
} linx_plugin_metadata_t;

// ============================================================================
// 插件配置
// ============================================================================

typedef struct {
    const char* name;
    const char* value;
} linx_plugin_config_param_t;

typedef struct {
    linx_plugin_config_param_t* params;
    uint32_t param_count;
    void* custom_data;
    size_t custom_data_size;
} linx_plugin_config_t;

// ============================================================================
// 插件状态
// ============================================================================

typedef enum {
    PLUGIN_STATE_UNLOADED = 0,
    PLUGIN_STATE_LOADED = 1,
    PLUGIN_STATE_INITIALIZED = 2,
    PLUGIN_STATE_RUNNING = 3,
    PLUGIN_STATE_PAUSED = 4,
    PLUGIN_STATE_ERROR = 5
} linx_plugin_state_t;

// ============================================================================
// 插件能力标志
// ============================================================================

#define PLUGIN_CAP_REALTIME         (1 << 0)   // 支持实时处理
#define PLUGIN_CAP_OFFLINE          (1 << 1)   // 支持离线处理
#define PLUGIN_CAP_INPLACE          (1 << 2)   // 支持原地处理
#define PLUGIN_CAP_VARIABLE_SIZE    (1 << 3)   // 支持可变缓冲区大小
#define PLUGIN_CAP_MULTI_CHANNEL    (1 << 4)   // 支持多声道
#define PLUGIN_CAP_CONFIGURABLE     (1 << 5)   // 支持运行时配置
#define PLUGIN_CAP_STATEFUL         (1 << 6)   // 有状态处理
#define PLUGIN_CAP_THREAD_SAFE      (1 << 7)   // 线程安全
#define PLUGIN_CAP_ZERO_LATENCY     (1 << 8)   // 零延迟
#define PLUGIN_CAP_ADAPTIVE         (1 << 9)   // 自适应处理

// ============================================================================
// 插件基础接口
// ============================================================================

typedef struct linx_plugin_base linx_plugin_base_t;

typedef struct {
    // 生命周期管理
    linx_audio_result_t (*initialize)(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
    linx_audio_result_t (*deinitialize)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*start)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*stop)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*pause)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*resume)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*reset)(linx_plugin_base_t* plugin);
    
    // 数据处理
    linx_audio_result_t (*process)(linx_plugin_base_t* plugin,
                             const linx_audio_buffer_t* input,
                             linx_audio_buffer_t* output);
    
    linx_audio_result_t (*process_inplace)(linx_plugin_base_t* plugin,
                                     linx_audio_buffer_t* buffer);
    
    // 配置管理
    linx_audio_result_t (*set_config)(linx_plugin_base_t* plugin, const linx_plugin_config_t* config);
    linx_audio_result_t (*get_config)(linx_plugin_base_t* plugin, linx_plugin_config_t* config);
    linx_audio_result_t (*set_parameter)(linx_plugin_base_t* plugin, const char* name, const char* value);
    linx_audio_result_t (*get_parameter)(linx_plugin_base_t* plugin, const char* name, char* value, size_t size);
    
    // 格式支持
    linx_audio_result_t (*set_format)(linx_plugin_base_t* plugin, const linx_audio_format_info_t* format);
    linx_audio_result_t (*get_format)(linx_plugin_base_t* plugin, linx_audio_format_info_t* format);
    bool (*supports_format)(linx_plugin_base_t* plugin, const linx_audio_format_info_t* format);
    
    // 延迟信息
    linx_audio_result_t (*get_latency)(linx_plugin_base_t* plugin, uint32_t* latency_frames);
    linx_audio_result_t (*get_tail_time)(linx_plugin_base_t* plugin, uint32_t* tail_frames);
    
    // 状态查询
    linx_plugin_state_t (*get_state)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*get_info)(linx_plugin_base_t* plugin, const char* key, char* value, size_t size);
    
    // 事件处理
    linx_audio_result_t (*on_event)(linx_plugin_base_t* plugin, const linx_audio_event_t* event);
    
    // 清理
    void (*destroy)(linx_plugin_base_t* plugin);
} linx_plugin_vtable_t;

// ============================================================================
// 插件基础结构体
// ============================================================================

struct linx_plugin_base {
    // 虚函数表
    const linx_plugin_vtable_t* vtable;
    
    // 元数据
    linx_plugin_metadata_t metadata;
    
    // 状态
    linx_plugin_state_t state;
    
    // 配置
    linx_plugin_config_t config;
    
    // 格式信息
    linx_audio_format_info_t input_format;
    linx_audio_format_info_t output_format;
    
    // 统计信息
    struct {
        uint64_t frames_processed;
        uint64_t processing_time_us;
        uint32_t error_count;
        uint32_t underrun_count;
        uint32_t overrun_count;
    } stats;
    
    // 事件回调
    linx_audio_event_callback_t event_callback;
    void* event_user_data;
    
    // 私有数据
    void* private_data;
    
    // 引用计数
    uint32_t ref_count;
};

// ============================================================================
// 插件工厂函数类型
// ============================================================================

typedef linx_plugin_base_t* (*linx_plugin_create_func_t)(const linx_plugin_config_t* config);
typedef void (*linx_plugin_destroy_func_t)(linx_plugin_base_t* plugin);
typedef linx_audio_result_t (*linx_plugin_get_metadata_func_t)(linx_plugin_metadata_t* metadata);

// ============================================================================
// 插件描述符
// ============================================================================

typedef struct {
    linx_plugin_metadata_t metadata;
    linx_plugin_create_func_t create;
    linx_plugin_destroy_func_t destroy;
    linx_plugin_get_metadata_func_t get_metadata;
} linx_plugin_descriptor_t;

// ============================================================================
// 插件导出宏
// ============================================================================

#define PLUGIN_EXPORT __attribute__((visibility("default")))

#define DECLARE_PLUGIN(name) \
    PLUGIN_EXPORT linx_plugin_base_t* create_##name##_plugin(const linx_plugin_config_t* config); \
    PLUGIN_EXPORT void destroy_##name##_plugin(linx_plugin_base_t* plugin); \
    PLUGIN_EXPORT linx_audio_result_t get_##name##_plugin_metadata(linx_plugin_metadata_t* metadata); \
    PLUGIN_EXPORT const linx_plugin_descriptor_t* get_plugin_descriptor(void)

#define IMPLEMENT_PLUGIN(name) \
    PLUGIN_EXPORT const linx_plugin_descriptor_t* get_plugin_descriptor(void) { \
        static linx_plugin_descriptor_t descriptor = { \
            .create = create_##name##_plugin, \
            .destroy = destroy_##name##_plugin, \
            .get_metadata = get_##name##_plugin_metadata \
        }; \
        get_##name##_plugin_metadata(&descriptor.metadata); \
        return &descriptor; \
    }

// ============================================================================
// 插件基础API
// ============================================================================

/**
 * @brief 初始化插件基础结构
 * @param plugin 插件指针
 * @param vtable 虚函数表
 * @param metadata 元数据
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_base_init(linx_plugin_base_t* plugin,
                               const linx_plugin_vtable_t* vtable,
                               const linx_plugin_metadata_t* metadata);

/**
 * @brief 增加插件引用计数
 * @param plugin 插件指针
 * @return 新的引用计数
 */
uint32_t linx_plugin_base_ref(linx_plugin_base_t* plugin);

/**
 * @brief 减少插件引用计数
 * @param plugin 插件指针
 * @return 新的引用计数
 */
uint32_t linx_plugin_base_unref(linx_plugin_base_t* plugin);

/**
 * @brief 设置插件事件回调
 * @param plugin 插件指针
 * @param callback 事件回调函数
 * @param user_data 用户数据
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_base_set_event_callback(linx_plugin_base_t* plugin,
                                             linx_audio_event_callback_t callback,
                                             void* user_data);

/**
 * @brief 发送插件事件
 * @param plugin 插件指针
 * @param event 事件指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_base_send_event(linx_plugin_base_t* plugin, const linx_audio_event_t* event);

/**
 * @brief 获取插件统计信息
 * @param plugin 插件指针
 * @param stats 统计信息指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_base_get_stats(const linx_plugin_base_t* plugin, void* stats);

/**
 * @brief 重置插件统计信息
 * @param plugin 插件指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_base_reset_stats(linx_plugin_base_t* plugin);

// ============================================================================
// 插件配置工具
// ============================================================================

/**
 * @brief 创建插件配置
 * @param config 配置指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_config_create(linx_plugin_config_t* config);

/**
 * @brief 销毁插件配置
 * @param config 配置指针
 */
void linx_plugin_config_destroy(linx_plugin_config_t* config);

/**
 * @brief 添加配置参数
 * @param config 配置指针
 * @param name 参数名
 * @param value 参数值
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_config_add_param(linx_plugin_config_t* config,
                                      const char* name,
                                      const char* value);

/**
 * @brief 获取配置参数
 * @param config 配置指针
 * @param name 参数名
 * @param value 参数值指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_config_get_param(const linx_plugin_config_t* config,
                                      const char* name,
                                      const char** value);

/**
 * @brief 设置自定义数据
 * @param config 配置指针
 * @param data 数据指针
 * @param size 数据大小
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_config_set_custom_data(linx_plugin_config_t* config,
                                            const void* data,
                                            size_t size);

// ============================================================================
// 插件元数据工具
// ============================================================================

/**
 * @brief 创建插件元数据
 * @param metadata 元数据指针
 * @param name 插件名称
 * @param version 版本信息
 * @param type 插件类型
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_metadata_create(linx_plugin_metadata_t* metadata,
                                     const char* name,
                                     const linx_plugin_version_t* version,
                                     linx_audio_plugin_type_t type);

/**
 * @brief 销毁插件元数据
 * @param metadata 元数据指针
 */
void linx_plugin_metadata_destroy(linx_plugin_metadata_t* metadata);

/**
 * @brief 比较插件版本
 * @param v1 版本1
 * @param v2 版本2
 * @return 比较结果 (-1: v1 < v2, 0: v1 == v2, 1: v1 > v2)
 */
int linx_plugin_version_compare(const linx_plugin_version_t* v1, const linx_plugin_version_t* v2);

/**
 * @brief 版本转字符串
 * @param version 版本信息
 * @param buffer 字符串缓冲区
 * @param size 缓冲区大小
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_version_to_string(const linx_plugin_version_t* version,
                                       char* buffer,
                                       size_t size);

/**
 * @brief 字符串转版本
 * @param str 版本字符串
 * @param version 版本信息指针
 * @return 操作结果
 */
linx_audio_result_t linx_plugin_version_from_string(const char* str, linx_plugin_version_t* version);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_PLUGINS_PLUGIN_INTERFACE_H