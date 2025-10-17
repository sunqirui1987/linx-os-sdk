/**
 * @file plugin_registry.h
 * @brief 内置插件注册表
 * @details 简化的插件注册系统,用于管理内置插件描述符
 */

#ifndef LINX_AUDIO_PLUGIN_REGISTRY_H
#define LINX_AUDIO_PLUGIN_REGISTRY_H

#include "../plugin_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 插件注册表
// ============================================================================

/**
 * @brief 最大内置插件数量
 */
#define LINX_MAX_BUILTIN_PLUGINS 32

/**
 * @brief 注册内置插件
 * @param descriptor 插件描述符
 * @return 操作结果
 */
linx_audio_result_t linx_register_builtin_plugin(const linx_plugin_descriptor_t* descriptor);

/**
 * @brief 根据名称查找插件描述符
 * @param name 插件名称
 * @return 插件描述符指针,未找到返回NULL
 */
const linx_plugin_descriptor_t* linx_find_plugin_descriptor(const char* name);

/**
 * @brief 获取所有已注册的插件描述符
 * @param count 输出参数,返回插件数量
 * @return 插件描述符数组
 */
const linx_plugin_descriptor_t** linx_get_all_plugin_descriptors(uint32_t* count);

/**
 * @brief 列出所有已注册的插件
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 操作结果
 */
linx_audio_result_t linx_list_registered_plugins(char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_PLUGIN_REGISTRY_H
