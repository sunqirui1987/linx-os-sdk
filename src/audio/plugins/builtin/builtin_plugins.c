/**
 * @file builtin_plugins.c
 * @brief LinxOS Audio内置插件注册和管理实现
 */

#include "builtin_plugins.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 动态插件注册表
// ============================================================================

static linx_builtin_plugin_descriptor_t builtin_plugins[LINX_MAX_BUILTIN_PLUGINS];
static size_t builtin_plugins_count = 0;

// ============================================================================
// 内部状态
// ============================================================================

static bool builtin_plugins_initialized = false;

// ============================================================================
// 公共接口实现
// ============================================================================

linx_audio_result_t linx_register_builtin_plugin(const linx_builtin_plugin_descriptor_t* descriptor) {
    if (!descriptor || !descriptor->name || !descriptor->create_func || 
        !descriptor->destroy_func || !descriptor->get_metadata_func) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    if (builtin_plugins_count >= LINX_MAX_BUILTIN_PLUGINS) {
        return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
    }
    
    // 检查是否已经注册了同名插件
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        if (strcmp(builtin_plugins[i].name, descriptor->name) == 0) {
            return LINX_AUDIO_ERROR_INVALID_STATE; // 插件已存在
        }
    }
    
    // 复制插件描述符
    builtin_plugins[builtin_plugins_count] = *descriptor;
    builtin_plugins_count++;
    
    return LINX_AUDIO_SUCCESS;
}

const linx_builtin_plugin_descriptor_t* linx_get_all_builtin_plugin_descriptors(size_t* count) {
    if (count) {
        *count = builtin_plugins_count;
    }
    return builtin_plugins;
}

const linx_builtin_plugin_descriptor_t* linx_find_builtin_plugin_by_name(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        if (strcmp(builtin_plugins[i].name, name) == 0) {
            return &builtin_plugins[i];
        }
    }
    
    return NULL;
}

linx_plugin_base_t* linx_create_builtin_plugin_by_name(const char* name, 
                                                       const linx_plugin_config_t* config) {
    const linx_builtin_plugin_descriptor_t* descriptor = linx_find_builtin_plugin_by_name(name);
    if (!descriptor || !descriptor->create_func) {
        return NULL;
    }
    
    return descriptor->create_func(config);
}

void linx_destroy_builtin_plugin_by_name(const char* name, linx_plugin_base_t* plugin) {
    if (!plugin || !name) {
        return;
    }
    
    const linx_builtin_plugin_descriptor_t* descriptor = linx_find_builtin_plugin_by_name(name);
    if (descriptor && descriptor->destroy_func) {
        descriptor->destroy_func(plugin);
    }
}

linx_audio_result_t linx_builtin_plugins_init(void) {
    if (builtin_plugins_initialized) {
        return LINX_AUDIO_SUCCESS;
    }
    
    // 验证所有插件描述符
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        const linx_builtin_plugin_descriptor_t* desc = &builtin_plugins[i];
        
        if (!desc->name || !desc->description || 
            !desc->create_func || !desc->destroy_func || 
            !desc->get_metadata_func) {
            return LINX_AUDIO_ERROR_INVALID_STATE;
        }
        
        // 验证插件元数据
        linx_plugin_metadata_t metadata;
        linx_audio_result_t result = desc->get_metadata_func(&metadata);
        if (result != LINX_AUDIO_SUCCESS) {
            return result;
        }
    }
    
    builtin_plugins_initialized = true;
    return LINX_AUDIO_SUCCESS;
}

void linx_builtin_plugins_cleanup(void) {
    builtin_plugins_initialized = false;
}

const char* linx_builtin_plugins_get_version(void) {
    return LINX_BUILTIN_PLUGINS_VERSION_STRING;
}

bool linx_is_builtin_plugin(const linx_plugin_base_t* plugin) {
    if (!plugin) {
        return false;
    }
    
    // 检查插件是否在内置插件列表中
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        // 这里需要一个更好的方法来识别插件类型
        // 可以通过插件的vtable或者其他标识符来判断
        // 暂时通过创建临时插件来比较vtable指针
        linx_plugin_base_t* temp = builtin_plugins[i].create_func(NULL);
        if (temp) {
            bool is_same_type = (plugin->vtable == temp->vtable);
            builtin_plugins[i].destroy_func(temp);
            if (is_same_type) {
                return true;
            }
        }
    }
    
    return false;
}

const char* linx_get_builtin_plugin_name(const linx_plugin_base_t* plugin) {
    if (!plugin) {
        return NULL;
    }
    
    // 类似于linx_is_builtin_plugin的实现
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        linx_plugin_base_t* temp = builtin_plugins[i].create_func(NULL);
        if (temp) {
            bool is_same_type = (plugin->vtable == temp->vtable);
            builtin_plugins[i].destroy_func(temp);
            if (is_same_type) {
                return builtin_plugins[i].name;
            }
        }
    }
    
    return NULL;
}

linx_audio_result_t linx_list_builtin_plugins(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    size_t offset = 0;
    int written;
    
    written = snprintf(buffer + offset, buffer_size - offset, 
                      "LinxOS Audio Built-in Plugins (v%s):\n", 
                      LINX_BUILTIN_PLUGINS_VERSION_STRING);
    if (written < 0 || (size_t)written >= buffer_size - offset) {
        return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
    }
    offset += written;
    
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        const linx_builtin_plugin_descriptor_t* desc = &builtin_plugins[i];
        
        const char* type_str;
        switch (desc->type) {
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
                          (int)i + 1, desc->name, type_str, desc->description);
        if (written < 0 || (size_t)written >= buffer_size - offset) {
            return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
        }
        offset += written;
    }
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_get_builtin_plugin_stats(linx_builtin_plugin_stats_t* stats) {
    if (!stats) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    memset(stats, 0, sizeof(linx_builtin_plugin_stats_t));
    
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        const linx_builtin_plugin_descriptor_t* desc = &builtin_plugins[i];
        
        stats->total_plugins++;
        
        switch (desc->type) {
            case LINX_AUDIO_PLUGIN_TYPE_EFFECT:
                stats->effect_plugins++;
                break;
            case LINX_AUDIO_PLUGIN_TYPE_CODEC:
                stats->generator_plugins++;
                break;
            case LINX_AUDIO_PLUGIN_TYPE_FILTER:
                stats->analyzer_plugins++;
                break;
            case LINX_AUDIO_PLUGIN_TYPE_DRIVER:
                stats->utility_plugins++;
                break;
            default:
                break;
        }
    }
    
    return LINX_AUDIO_SUCCESS;
}