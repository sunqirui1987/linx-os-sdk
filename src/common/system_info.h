#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get flash size in bytes
 * @return Flash size in bytes, 0 if unknown
 */
uint32_t system_info_get_flash_size(void);

/**
 * Get minimum free heap size in bytes
 * @return Minimum free heap size in bytes
 */
uint32_t system_info_get_minimum_free_heap_size(void);

/**
 * Get current free heap size in bytes
 * @return Current free heap size in bytes
 */
uint32_t system_info_get_free_heap_size(void);

/**
 * Get MAC address as string
 * @param mac_str Buffer to store MAC address string (at least 18 bytes)
 * @param size Size of the buffer
 * @return true on success, false on error
 */
bool system_info_get_mac_address(char* mac_str, size_t size);

/**
 * Get chip model name
 * @return Chip model name string (static string, do not free)
 */
const char* system_info_get_chip_model_name(void);

/**
 * Get chip revision
 * @return Chip revision number
 */
uint32_t system_info_get_chip_revision(void);

/**
 * Get number of CPU cores
 * @return Number of CPU cores
 */
uint32_t system_info_get_cpu_cores(void);

/**
 * Get chip features bitmask
 * @return Chip features bitmask
 */
uint32_t system_info_get_chip_features(void);

/**
 * Get application name
 * @return Application name string (static string, do not free)
 */
const char* system_info_get_app_name(void);

/**
 * Get application version
 * @return Application version string (static string, do not free)
 */
const char* system_info_get_app_version(void);

/**
 * Get application compile date
 * @return Compile date string (static string, do not free)
 */
const char* system_info_get_app_compile_date(void);

/**
 * Get application compile time
 * @return Compile time string (static string, do not free)
 */
const char* system_info_get_app_compile_time(void);

/**
 * Get IDF version (for ESP32) or SDK version
 * @return IDF/SDK version string (static string, do not free)
 */
const char* system_info_get_idf_version(void);

/**
 * Get application ELF SHA256 hash
 * @param sha256_str Buffer to store SHA256 string (at least 65 bytes)
 * @param size Size of the buffer
 * @return true on success, false on error
 */
bool system_info_get_app_elf_sha256(char* sha256_str, size_t size);

/**
 * Get system uptime in milliseconds
 * @return System uptime in milliseconds
 */
uint64_t system_info_get_uptime_ms(void);

/**
 * Get CPU frequency in Hz
 * @return CPU frequency in Hz
 */
uint32_t system_info_get_cpu_freq_hz(void);

/**
 * Get system temperature in Celsius (if available)
 * @param temperature Pointer to store temperature value
 * @return true if temperature is available, false otherwise
 */
bool system_info_get_temperature(float* temperature);

/**
 * Get PSRAM size in bytes (if available)
 * @return PSRAM size in bytes, 0 if not available
 */
uint32_t system_info_get_psram_size(void);

/**
 * Get free PSRAM size in bytes (if available)
 * @return Free PSRAM size in bytes, 0 if not available
 */
uint32_t system_info_get_free_psram_size(void);

/**
 * Reset system
 */
void system_info_reset(void);

/**
 * Get reset reason
 * @return Reset reason string (static string, do not free)
 */
const char* system_info_get_reset_reason(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INFO_H