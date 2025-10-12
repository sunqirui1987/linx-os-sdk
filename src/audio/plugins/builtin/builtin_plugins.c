/**
 * @file builtin_plugins.c
 * @brief LinxOS Audio内置插件注册和管理实现
 */

#include "builtin_plugins.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内置插件注册表
// ============================================================================

static const linx_builtin_plugin_descriptor_t builtin_plugins[] = {
    {
        .id = LINX_BUILTIN_PLUGIN_GAIN,
        .name = "Gain",
        .description = "Audio gain control plugin",
        .type = LINX_PLUGIN_TYPE_EFFECT,
        .create_func = create_gain_plugin,
        .destroy_func = destroy_gain_plugin,
        .get_metadata_func = get_gain_plugin_metadata
    },
    {
        .id = LINX_BUILTIN_PLUGIN_EQUALIZER,
        .name = "Equalizer",
        .description = "Multi-band audio equalizer",
        .type = LINX_PLUGIN_TYPE_EFFECT,
        .create_func = create_equalizer_plugin,
        .destroy_func = destroy_equalizer_plugin,
        .get_metadata_func = get_equalizer_plugin_metadata
    },
    {
        .id = LINX_BUILTIN_PLUGIN_DELAY,
        .name = "Delay",
        .description = "Audio delay and echo effect",
        .type = LINX_PLUGIN_TYPE_EFFECT,
        .create_func = create_delay_plugin,
        .destroy_func = destroy_delay_plugin,
        .get_metadata_func = get_delay_plugin_metadata
    },
    {
        .id = LINX_BUILTIN_PLUGIN_REVERB,
        .name = "Reverb",
        .description = "Freeverb-based reverb effect",
        .type = LINX_PLUGIN_TYPE_EFFECT,
        .create_func = create_reverb_plugin,
        .destroy_func = destroy_reverb_plugin,
        .get_metadata_func = get_reverb_plugin_metadata
    }
};

static const size_t builtin_plugins_count = sizeof(builtin_plugins) / sizeof(builtin_plugins[0]);

// ============================================================================
// 内部状态
// ============================================================================

static bool builtin_plugins_initialized = false;

// ============================================================================
// 公共接口实现
// ============================================================================

const linx_builtin_plugin_descriptor_t* linx_get_builtin_plugin_descriptor(linx_builtin_plugin_id_t id) {
    if (id < 0 || id >= LINX_BUILTIN_PLUGIN_COUNT) {
        return NULL;
    }
    
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        if (builtin_plugins[i].id == id) {
            return &builtin_plugins[i];
        }
    }
    
    return NULL;
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

linx_plugin_base_t* linx_create_builtin_plugin(linx_builtin_plugin_id_t id, 
                                               const linx_plugin_config_t* config) {
    const linx_builtin_plugin_descriptor_t* descriptor = linx_get_builtin_plugin_descriptor(id);
    if (!descriptor || !descriptor->create_func) {
        return NULL;
    }
    
    return descriptor->create_func(config);
}

void linx_destroy_builtin_plugin(linx_builtin_plugin_id_t id, linx_plugin_base_t* plugin) {
    if (!plugin) {
        return;
    }
    
    const linx_builtin_plugin_descriptor_t* descriptor = linx_get_builtin_plugin_descriptor(id);
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

int linx_get_builtin_plugin_id(const linx_plugin_base_t* plugin) {
    if (!plugin) {
        return -1;
    }
    
    // 类似于linx_is_builtin_plugin的实现
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        linx_plugin_base_t* temp = builtin_plugins[i].create_func(NULL);
        if (temp) {
            bool is_same_type = (plugin->vtable == temp->vtable);
            builtin_plugins[i].destroy_func(temp);
            if (is_same_type) {
                return (int)builtin_plugins[i].id;
            }
        }
    }
    
    return -1;
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
        return LINX_AUDIO_ERROR_BUFFER_TOO_SMALL;
    }
    offset += written;
    
    for (size_t i = 0; i < builtin_plugins_count; i++) {
        const linx_builtin_plugin_descriptor_t* desc = &builtin_plugins[i];
        
        const char* type_str;
        switch (desc->type) {
            case LINX_PLUGIN_TYPE_EFFECT:
                type_str = "Effect";
                break;
            case LINX_PLUGIN_TYPE_GENERATOR:
                type_str = "Generator";
                break;
            case LINX_PLUGIN_TYPE_ANALYZER:
                type_str = "Analyzer";
                break;
            case LINX_PLUGIN_TYPE_UTILITY:
                type_str = "Utility";
                break;
            default:
                type_str = "Unknown";
                break;
        }
        
        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %d. %s (%s) - %s\n",
                          (int)desc->id + 1, desc->name, type_str, desc->description);
        if (written < 0 || (size_t)written >= buffer_size - offset) {
            return LINX_AUDIO_ERROR_BUFFER_TOO_SMALL;
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
            case LINX_PLUGIN_TYPE_EFFECT:
                stats->effect_plugins++;
                break;
            case LINX_PLUGIN_TYPE_GENERATOR:
                stats->generator_plugins++;
                break;
            case LINX_PLUGIN_TYPE_ANALYZER:
                stats->analyzer_plugins++;
                break;
            case LINX_PLUGIN_TYPE_UTILITY:
                stats->utility_plugins++;
                break;
            default:
                break;
        }
    }
    
    return LINX_AUDIO_SUCCESS;
}