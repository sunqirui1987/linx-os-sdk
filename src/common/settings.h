#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Settings interface for configuration management
 */
typedef struct Settings Settings;

/**
 * Create a new settings instance
 * @param namespace Configuration namespace (e.g., "board", "audio", etc.)
 * @param auto_save Whether to automatically save changes
 * @return Settings instance or NULL on error
 */
Settings* settings_create(const char* namespace, bool auto_save);

/**
 * Destroy settings instance
 * @param settings Settings instance
 */
void settings_destroy(Settings* settings);

/**
 * Get string value
 * @param settings Settings instance
 * @param key Configuration key
 * @return String value or NULL if not found
 */
const char* settings_get_string(Settings* settings, const char* key);

/**
 * Set string value
 * @param settings Settings instance
 * @param key Configuration key
 * @param value String value
 * @return true on success, false on error
 */
bool settings_set_string(Settings* settings, const char* key, const char* value);

/**
 * Get integer value
 * @param settings Settings instance
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Integer value
 */
int settings_get_int(Settings* settings, const char* key, int default_value);

/**
 * Set integer value
 * @param settings Settings instance
 * @param key Configuration key
 * @param value Integer value
 * @return true on success, false on error
 */
bool settings_set_int(Settings* settings, const char* key, int value);

/**
 * Get boolean value
 * @param settings Settings instance
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Boolean value
 */
bool settings_get_bool(Settings* settings, const char* key, bool default_value);

/**
 * Set boolean value
 * @param settings Settings instance
 * @param key Configuration key
 * @param value Boolean value
 * @return true on success, false on error
 */
bool settings_set_bool(Settings* settings, const char* key, bool value);

/**
 * Get float value
 * @param settings Settings instance
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Float value
 */
float settings_get_float(Settings* settings, const char* key, float default_value);

/**
 * Set float value
 * @param settings Settings instance
 * @param key Configuration key
 * @param value Float value
 * @return true on success, false on error
 */
bool settings_set_float(Settings* settings, const char* key, float value);

/**
 * Check if key exists
 * @param settings Settings instance
 * @param key Configuration key
 * @return true if key exists, false otherwise
 */
bool settings_has_key(Settings* settings, const char* key);

/**
 * Remove key
 * @param settings Settings instance
 * @param key Configuration key
 * @return true on success, false on error
 */
bool settings_remove_key(Settings* settings, const char* key);

/**
 * Save settings to persistent storage
 * @param settings Settings instance
 * @return true on success, false on error
 */
bool settings_save(Settings* settings);

/**
 * Load settings from persistent storage
 * @param settings Settings instance
 * @return true on success, false on error
 */
bool settings_load(Settings* settings);

/**
 * Clear all settings
 * @param settings Settings instance
 * @return true on success, false on error
 */
bool settings_clear(Settings* settings);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H