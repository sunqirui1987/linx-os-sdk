/**
 * @file plugin_registry.c
 * @brief 内置插件注册表实现
 */

#include "plugin_registry.h"
#include "../../core/types.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部数据结构
// ============================================================================

static const linx_plugin_descriptor_t* g_registered_plugins[LINX_MAX_BUILTIN_PLUGINS];
static uint32_t g_plugin_count = 0;

// ============================================================================
// 公共API实现
// ============================================================================

linx_audio_result_t linx_register_builtin_plugin(const linx_plugin_descriptor_t* descriptor)
{
    if (!descriptor || !descriptor->metadata.name) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    if (g_plugin_count >= LINX_MAX_BUILTIN_PLUGINS) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }

    // 检查是否已注册同名插件
    for (uint32_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_registered_plugins[i]->metadata.name, descriptor->metadata.name) == 0) {
            return LINX_AUDIO_ERROR_INVALID_STATE;
        }
    }

    // 注册插件
    g_registered_plugins[g_plugin_count++] = descriptor;

    return LINX_AUDIO_SUCCESS;
}

const linx_plugin_descriptor_t* linx_find_plugin_descriptor(const char* name)
{
    if (!name) {
        return NULL;
    }

    for (uint32_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_registered_plugins[i]->metadata.name, name) == 0) {
            return g_registered_plugins[i];
        }
    }

    return NULL;
}

const linx_plugin_descriptor_t** linx_get_all_plugin_descriptors(uint32_t* count)
{
    if (count) {
        *count = g_plugin_count;
    }
    return g_registered_plugins;
}

linx_audio_result_t linx_list_registered_plugins(char* buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    size_t offset = 0;
    int written;

    written = snprintf(buffer + offset, buffer_size - offset,
                      "LinxOS Audio Built-in Plugins:\n");
    if (written < 0 || (size_t)written >= buffer_size - offset) {
        return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
    }
    offset += written;

    for (uint32_t i = 0; i < g_plugin_count; i++) {
        const linx_plugin_descriptor_t* desc = g_registered_plugins[i];

        const char* type_str;
        switch (desc->metadata.type) {
            case LINX_AUDIO_PLUGIN_TYPE_EFFECT:
                type_str = "Effect";
                break;
            case LINX_AUDIO_PLUGIN_TYPE_CODEC:
                type_str = "Codec";
                break;
            case LINX_AUDIO_PLUGIN_TYPE_FILTER:
                type_str = "Filter";
                break;
            case LINX_AUDIO_PLUGIN_TYPE_DRIVER:
                type_str = "Driver";
                break;
            default:
                type_str = "Unknown";
                break;
        }

        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %d. %s (%s) - %s\n",
                          (int)i + 1, desc->metadata.name, type_str,
                          desc->metadata.description ? desc->metadata.description : "No description");
        if (written < 0 || (size_t)written >= buffer_size - offset) {
            return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
        }
        offset += written;
    }

    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 自动注册机制
// ============================================================================

/**
 * @brief 自动注册所有内置插件
 * @details 使用constructor属性在程序启动时自动调用
 */
static void __attribute__((constructor)) register_all_builtin_plugins(void)
{
    // 注册gain插件
    extern const linx_plugin_descriptor_t* linx_gain_plugin_get_descriptor(void);
    const linx_plugin_descriptor_t* gain_desc = linx_gain_plugin_get_descriptor();
    if (gain_desc) {
        linx_register_builtin_plugin(gain_desc);
    }

    // 在这里添加其他内置插件的注册
    // extern const linx_plugin_descriptor_t* linx_other_plugin_get_descriptor(void);
    // linx_register_builtin_plugin(linx_other_plugin_get_descriptor());
}
