#include "settings.h"
#include "log/linx_log.h"
#include "cjson/cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TAG "Settings"
#define MAX_NAMESPACE_LEN 64
#define MAX_FILENAME_LEN 128

/**
 * Settings implementation structure
 */
struct Settings {
    char namespace[MAX_NAMESPACE_LEN];
    char filename[MAX_FILENAME_LEN];
    bool auto_save;
    cJSON* json_root;
};

/**
 * Generate settings filename based on namespace
 */
static void generate_filename(const char* namespace, char* filename, size_t size) {
    snprintf(filename, size, "/tmp/linx_settings_%s.json", namespace);
}

/**
 * Load JSON from file
 */
static cJSON* load_json_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        LINX_LOGW(TAG, "Settings file not found: %s", filename);
        return cJSON_CreateObject();
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return cJSON_CreateObject();
    }
    
    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        LINX_LOGE(TAG, "Failed to allocate memory for settings file");
        return cJSON_CreateObject();
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    cJSON* json = cJSON_Parse(buffer);
    free(buffer);
    
    if (!json) {
        LINX_LOGE(TAG, "Failed to parse settings JSON");
        return cJSON_CreateObject();
    }
    
    return json;
}

/**
 * Save JSON to file
 */
static bool save_json_to_file(const char* filename, cJSON* json) {
    char* json_string = cJSON_Print(json);
    if (!json_string) {
        LINX_LOGE(TAG, "Failed to serialize JSON");
        return false;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        LINX_LOGE(TAG, "Failed to open settings file for writing: %s", filename);
        free(json_string);
        return false;
    }
    
    size_t len = strlen(json_string);
    size_t written = fwrite(json_string, 1, len, file);
    fclose(file);
    free(json_string);
    
    if (written != len) {
        LINX_LOGE(TAG, "Failed to write complete settings file");
        return false;
    }
    
    return true;
}

Settings* settings_create(const char* namespace, bool auto_save) {
    if (!namespace) {
        LINX_LOGE(TAG, "Namespace cannot be NULL");
        return NULL;
    }
    
    Settings* settings = malloc(sizeof(Settings));
    if (!settings) {
        LINX_LOGE(TAG, "Failed to allocate memory for settings");
        return NULL;
    }
    
    strncpy(settings->namespace, namespace, MAX_NAMESPACE_LEN - 1);
    settings->namespace[MAX_NAMESPACE_LEN - 1] = '\0';
    settings->auto_save = auto_save;
    
    generate_filename(namespace, settings->filename, MAX_FILENAME_LEN);
    settings->json_root = load_json_from_file(settings->filename);
    
    if (!settings->json_root) {
        free(settings);
        return NULL;
    }
    
    LINX_LOGI(TAG, "Settings created for namespace: %s", namespace);
    return settings;
}

void settings_destroy(Settings* settings) {
    if (!settings) return;
    
    if (settings->auto_save) {
        settings_save(settings);
    }
    
    if (settings->json_root) {
        cJSON_Delete(settings->json_root);
    }
    
    free(settings);
}

const char* settings_get_string(Settings* settings, const char* key) {
    if (!settings || !key) return NULL;
    
    cJSON* item = cJSON_GetObjectItem(settings->json_root, key);
    if (!item || !cJSON_IsString(item)) {
        return NULL;
    }
    
    return cJSON_GetStringValue(item);
}

bool settings_set_string(Settings* settings, const char* key, const char* value) {
    if (!settings || !key || !value) return false;
    
    cJSON* item = cJSON_CreateString(value);
    if (!item) return false;
    
    if (!cJSON_ReplaceItemInObject(settings->json_root, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}

int settings_get_int(Settings* settings, const char* key, int default_value) {
    if (!settings || !key) return default_value;
    
    cJSON* item = cJSON_GetObjectItem(settings->json_root, key);
    if (!item || !cJSON_IsNumber(item)) {
        return default_value;
    }
    
    return (int)cJSON_GetNumberValue(item);
}

bool settings_set_int(Settings* settings, const char* key, int value) {
    if (!settings || !key) return false;
    
    cJSON* item = cJSON_CreateNumber(value);
    if (!item) return false;
    
    if (!cJSON_ReplaceItemInObject(settings->json_root, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}

bool settings_get_bool(Settings* settings, const char* key, bool default_value) {
    if (!settings || !key) return default_value;
    
    cJSON* item = cJSON_GetObjectItem(settings->json_root, key);
    if (!item || !cJSON_IsBool(item)) {
        return default_value;
    }
    
    return cJSON_IsTrue(item);
}

bool settings_set_bool(Settings* settings, const char* key, bool value) {
    if (!settings || !key) return false;
    
    cJSON* item = cJSON_CreateBool(value);
    if (!item) return false;
    
    if (!cJSON_ReplaceItemInObject(settings->json_root, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}

float settings_get_float(Settings* settings, const char* key, float default_value) {
    if (!settings || !key) return default_value;
    
    cJSON* item = cJSON_GetObjectItem(settings->json_root, key);
    if (!item || !cJSON_IsNumber(item)) {
        return default_value;
    }
    
    return (float)cJSON_GetNumberValue(item);
}

bool settings_set_float(Settings* settings, const char* key, float value) {
    if (!settings || !key) return false;
    
    cJSON* item = cJSON_CreateNumber(value);
    if (!item) return false;
    
    if (!cJSON_ReplaceItemInObject(settings->json_root, key, item)) {
        cJSON_Delete(item);
        return false;
    }
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}

bool settings_has_key(Settings* settings, const char* key) {
    if (!settings || !key) return false;
    
    return cJSON_HasObjectItem(settings->json_root, key);
}

bool settings_remove_key(Settings* settings, const char* key) {
    if (!settings || !key) return false;
    
    cJSON_DeleteItemFromObject(settings->json_root, key);
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}

bool settings_save(Settings* settings) {
    if (!settings) return false;
    
    return save_json_to_file(settings->filename, settings->json_root);
}

bool settings_load(Settings* settings) {
    if (!settings) return false;
    
    cJSON* new_json = load_json_from_file(settings->filename);
    if (!new_json) return false;
    
    cJSON_Delete(settings->json_root);
    settings->json_root = new_json;
    
    return true;
}

bool settings_clear(Settings* settings) {
    if (!settings) return false;
    
    cJSON_Delete(settings->json_root);
    settings->json_root = cJSON_CreateObject();
    
    if (!settings->json_root) return false;
    
    if (settings->auto_save) {
        return settings_save(settings);
    }
    
    return true;
}