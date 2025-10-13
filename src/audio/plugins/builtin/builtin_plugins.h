/**
 * @file builtin_plugins.h
 * @brief LinxOS Audio内置插件注册和管理
 * @details 提供内置插件的统一接口和注册机制
 * @version @PROJECT_VERSION@
 */

#ifndef LINX_AUDIO_BUILTIN_PLUGINS_H
#define LINX_AUDIO_BUILTIN_PLUGINS_H

#include "../plugin_interface.h"
#include "../../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 版本信息
// ============================================================================

#define LINX_BUILTIN_PLUGINS_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define LINX_BUILTIN_PLUGINS_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define LINX_BUILTIN_PLUGINS_VERSION_PATCH @PROJECT_VERSION_PATCH@
#define LINX_BUILTIN_PLUGINS_VERSION_STRING "@PROJECT_VERSION@"

// ============================================================================
// 动态插件注册系统
// ============================================================================

/**
 * @brief 插件注册函数类型
 * @details 每个插件模块应该实现这个函数来注册自己
 */
typedef void (*linx_plugin_register_func_t)(void);

/**
 * @brief 最大支持的内置插件数量
 */
#define LINX_MAX_BUILTIN_PLUGINS 32

// ============================================================================
// 插件注册表
// ============================================================================

/**
 * @brief 内置插件描述符
 */
typedef struct {
    const char* name;
    const char* description;
    linx_audio_plugin_type_t type;
    linx_plugin_base_t* (*create_func)(const linx_plugin_config_t* config);
    void (*destroy_func)(linx_plugin_base_t* plugin);
    linx_audio_result_t (*get_metadata_func)(linx_plugin_metadata_t* metadata);
} linx_builtin_plugin_descriptor_t;

/**
 * @brief 注册内置插件
 * @param descriptor 插件描述符
 * @return 操作结果
 */
linx_audio_result_t linx_register_builtin_plugin(const linx_builtin_plugin_descriptor_t* descriptor);

/**
 * @brief 获取所有内置插件描述符
 * @param count 输出参数，返回插件数量
 * @return 插件描述符数组指针
 */
const linx_builtin_plugin_descriptor_t* linx_get_all_builtin_plugin_descriptors(size_t* count);

/**
 * @brief 根据名称查找内置插件
 * @param name 插件名称
 * @return 插件描述符指针，失败返回NULL
 */
const linx_builtin_plugin_descriptor_t* linx_find_builtin_plugin_by_name(const char* name);

/**
 * @brief 根据名称创建内置插件实例
 * @param name 插件名称
 * @param config 插件配置
 * @return 插件实例指针，失败返回NULL
 */
linx_plugin_base_t* linx_create_builtin_plugin_by_name(const char* name, 
                                                       const linx_plugin_config_t* config);

/**
 * @brief 销毁内置插件实例
 * @param name 插件名称
 * @param plugin 插件实例
 */
void linx_destroy_builtin_plugin_by_name(const char* name, linx_plugin_base_t* plugin);

/**
 * @brief 初始化内置插件系统
 * @return 操作结果
 */
linx_audio_result_t linx_builtin_plugins_init(void);

/**
 * @brief 清理内置插件系统
 */
void linx_builtin_plugins_cleanup(void);

/**
 * @brief 获取内置插件系统版本
 * @return 版本字符串
 */
const char* linx_builtin_plugins_get_version(void);

// ============================================================================
// 插件注册宏
// ============================================================================

/**
 * @brief 注册内置插件的宏
 * @param plugin_name 插件名称（用作标识符）
 * @param name_str 插件名称字符串
 * @param desc 插件描述
 * @param type 插件类型
 * @param create_func 创建函数
 * @param destroy_func 销毁函数
 * @param metadata_func 元数据获取函数
 */
#define LINX_REGISTER_BUILTIN_PLUGIN(plugin_name, name_str, desc, plugin_type, create_fn, destroy_fn, metadata_fn) \
    static const linx_builtin_plugin_descriptor_t builtin_plugin_##plugin_name = { \
        .name = name_str, \
        .description = desc, \
        .type = plugin_type, \
        .create_func = create_fn, \
        .destroy_func = destroy_fn, \
        .get_metadata_func = metadata_fn \
    }; \
    static void register_##plugin_name##_plugin(void) __attribute__((constructor)); \
    static void register_##plugin_name##_plugin(void) { \
        linx_register_builtin_plugin(&builtin_plugin_##plugin_name); \
    }

// ============================================================================
// 实用工具函数
// ============================================================================

/**
 * @brief 检查插件是否为内置插件
 * @param plugin 插件实例
 * @return 如果是内置插件返回true，否则返回false
 */
bool linx_is_builtin_plugin(const linx_plugin_base_t* plugin);

/**
 * @brief 获取内置插件的名称
 * @param plugin 插件实例
 * @return 插件名称，如果不是内置插件返回NULL
 */
const char* linx_get_builtin_plugin_name(const linx_plugin_base_t* plugin);

/**
 * @brief 列出所有可用的内置插件
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 操作结果
 */
linx_audio_result_t linx_list_builtin_plugins(char* buffer, size_t buffer_size);

/**
 * @brief 获取内置插件统计信息
 * @param stats 统计信息结构指针
 * @return 操作结果
 */
typedef struct {
    size_t total_plugins;
    size_t effect_plugins;
    size_t generator_plugins;
    size_t analyzer_plugins;
    size_t utility_plugins;
} linx_builtin_plugin_stats_t;

linx_audio_result_t linx_get_builtin_plugin_stats(linx_builtin_plugin_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_BUILTIN_PLUGINS_H