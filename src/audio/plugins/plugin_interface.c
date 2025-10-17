#include "plugin_interface.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// ============================================================================
// Plugin Base Functions
// ============================================================================

linx_audio_result_t linx_plugin_base_init(linx_plugin_base_t* plugin,
                                          const linx_plugin_vtable_t* vtable,
                                          const linx_plugin_metadata_t* metadata) {
    if (!plugin || !metadata) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }

    memset(plugin, 0, sizeof(linx_plugin_base_t));

    plugin->vtable = vtable;
    plugin->metadata = *metadata;
    plugin->state = PLUGIN_STATE_LOADED;
    plugin->ref_count = 1;

    // 初始化引用计数互斥锁
    if (pthread_mutex_init(&plugin->ref_count_mutex, NULL) != 0) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }

    return LINX_AUDIO_SUCCESS;
}

uint32_t linx_plugin_base_ref(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return 0;
    }

    pthread_mutex_lock(&plugin->ref_count_mutex);
    uint32_t new_count = ++plugin->ref_count;
    pthread_mutex_unlock(&plugin->ref_count_mutex);

    return new_count;
}

uint32_t linx_plugin_base_unref(linx_plugin_base_t* plugin) {
    if (!plugin) {
        return 0;
    }

    pthread_mutex_lock(&plugin->ref_count_mutex);

    if (plugin->ref_count == 0) {
        pthread_mutex_unlock(&plugin->ref_count_mutex);
        return 0;
    }

    uint32_t new_count = --plugin->ref_count;
    pthread_mutex_unlock(&plugin->ref_count_mutex);

    // 如果引用计数归零,销毁互斥锁
    if (new_count == 0) {
        pthread_mutex_destroy(&plugin->ref_count_mutex);
    }

    return new_count;
}

// ============================================================================
// Plugin Config Functions
// ============================================================================

linx_audio_result_t linx_plugin_config_create(linx_plugin_config_t* config) {
    if (!config) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    memset(config, 0, sizeof(linx_plugin_config_t));
    config->param_count = 0;
    config->params = NULL;
    
    return LINX_AUDIO_SUCCESS;
}

void linx_plugin_config_destroy(linx_plugin_config_t* config) {
    if (!config) {
        return;
    }

    // Free parameter names and values
    if (config->params) {
        for (size_t i = 0; i < config->param_count; i++) {
            free((void*)config->params[i].name);
            free((void*)config->params[i].value);
        }
        free(config->params);
    }

    // Free custom data
    if (config->custom_data) {
        free(config->custom_data);
    }

    memset(config, 0, sizeof(linx_plugin_config_t));
}

linx_audio_result_t linx_plugin_config_add_param(linx_plugin_config_t* config,
                                                 const char* name,
                                                 const char* value) {
    if (!config || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    // Reallocate parameters array
    linx_plugin_config_param_t* new_params = realloc(config->params, 
        (config->param_count + 1) * sizeof(linx_plugin_config_param_t));
    if (!new_params) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    config->params = new_params;
    
    // Copy name and value
    config->params[config->param_count].name = strdup(name);
    config->params[config->param_count].value = strdup(value);
    
    if (!config->params[config->param_count].name || 
        !config->params[config->param_count].value) {
        free((void*)config->params[config->param_count].name);
        free((void*)config->params[config->param_count].value);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    config->param_count++;
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_plugin_config_get_param(const linx_plugin_config_t* config,
                                                 const char* name,
                                                 const char** value) {
    if (!config || !name || !value) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    for (size_t i = 0; i < config->param_count; i++) {
        if (strcmp(config->params[i].name, name) == 0) {
            *value = config->params[i].value;
            return LINX_AUDIO_SUCCESS;
        }
    }
    
    return LINX_AUDIO_ERROR_NOT_FOUND;
}

// ============================================================================
// Plugin Version Functions
// ============================================================================

int linx_plugin_version_compare(const linx_plugin_version_t* v1, const linx_plugin_version_t* v2) {
    if (!v1 && !v2) {
        return 0;
    }
    if (!v1) {
        return -1;
    }
    if (!v2) {
        return 1;
    }
    
    if (v1->major != v2->major) {
        return (v1->major < v2->major) ? -1 : 1;
    }
    
    if (v1->minor != v2->minor) {
        return (v1->minor < v2->minor) ? -1 : 1;
    }
    
    if (v1->patch != v2->patch) {
        return (v1->patch < v2->patch) ? -1 : 1;
    }
    
    return 0;
}

linx_audio_result_t linx_plugin_version_to_string(const linx_plugin_version_t* version,
                                                  char* buffer,
                                                  size_t size) {
    if (!version || !buffer || size == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    int result;
    if (version->build && strlen(version->build) > 0) {
        result = snprintf(buffer, size, "%u.%u.%u-%s",
                         version->major, version->minor, version->patch, version->build);
    } else {
        result = snprintf(buffer, size, "%u.%u.%u",
                         version->major, version->minor, version->patch);
    }
    
    if (result < 0 || (size_t)result >= size) {
        return LINX_AUDIO_ERROR_BUFFER_OVERFLOW;
    }
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_plugin_version_from_string(const char* str, linx_plugin_version_t* version) {
    if (!str || !version) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    memset(version, 0, sizeof(linx_plugin_version_t));
    
    char* str_copy = strdup(str);
    if (!str_copy) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    char* build_part = strchr(str_copy, '-');
    if (build_part) {
        *build_part = '\0';
        build_part++;
        version->build = strdup(build_part);
    }
    
    int parsed = sscanf(str_copy, "%u.%u.%u", &version->major, &version->minor, &version->patch);
    
    free(str_copy);
    
    if (parsed < 3) {
        free((void*)version->build);
        memset(version, 0, sizeof(linx_plugin_version_t));
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_plugin_config_set_custom_data(linx_plugin_config_t* config,
                                                       const void* data,
                                                       size_t size) {
    if (!config || !data) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    // Free existing custom data if any
    if (config->custom_data) {
        free(config->custom_data);
    }
    
    // Allocate and copy new data
    config->custom_data = malloc(size);
    if (!config->custom_data) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }

    memcpy(config->custom_data, data, size);
    config->custom_data_size = size;

    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_plugin_metadata_create(linx_plugin_metadata_t* metadata,
                                                const char* name,
                                                const linx_plugin_version_t* version,
                                                linx_audio_plugin_type_t type) {
    if (!metadata || !name || !version) {
        return LINX_AUDIO_ERROR_INVALID_PARAMETER;
    }
    
    memset(metadata, 0, sizeof(linx_plugin_metadata_t));
    
    metadata->name = strdup(name);
    if (!metadata->name) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    metadata->version = *version;
    metadata->type = type;
    
    return LINX_AUDIO_SUCCESS;
}

void linx_plugin_metadata_destroy(linx_plugin_metadata_t* metadata) {
    if (!metadata) {
        return;
    }
    
    free((void*)metadata->name);
    free((void*)metadata->version.build);
    
    memset(metadata, 0, sizeof(linx_plugin_metadata_t));
}