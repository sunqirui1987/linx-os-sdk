#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct SystemInfo SystemInfo;
typedef struct SystemInfoVTable SystemInfoVTable;

// SystemInfo vtable structure - contains function pointers for virtual methods
struct SystemInfoVTable {
    // Hardware information
    uint32_t (*get_flash_size)(SystemInfo* self);
    uint32_t (*get_minimum_free_heap_size)(SystemInfo* self);
    uint32_t (*get_free_heap_size)(SystemInfo* self);
    bool (*get_mac_address)(SystemInfo* self, char* mac_str, size_t size);
    const char* (*get_chip_model_name)(SystemInfo* self);
    uint32_t (*get_chip_revision)(SystemInfo* self);
    uint32_t (*get_cpu_cores)(SystemInfo* self);
    uint32_t (*get_chip_features)(SystemInfo* self);
    uint32_t (*get_cpu_freq_hz)(SystemInfo* self);
    bool (*get_temperature)(SystemInfo* self, float* temperature);
    uint32_t (*get_psram_size)(SystemInfo* self);
    uint32_t (*get_free_psram_size)(SystemInfo* self);
    
    // Application information
    const char* (*get_app_name)(SystemInfo* self);
    const char* (*get_app_version)(SystemInfo* self);
    const char* (*get_app_compile_date)(SystemInfo* self);
    const char* (*get_app_compile_time)(SystemInfo* self);
    const char* (*get_idf_version)(SystemInfo* self);
    bool (*get_app_elf_sha256)(SystemInfo* self, char* sha256_str, size_t size);
    
    // System status
    uint64_t (*get_uptime_ms)(SystemInfo* self);
    const char* (*get_reset_reason)(SystemInfo* self);
    
    // System control
    void (*reset)(SystemInfo* self);
    
    // Destructor
    void (*destroy)(SystemInfo* self);
};

// SystemInfo base class structure
struct SystemInfo {
    const SystemInfoVTable* vtable;
    void* data;   // Implementation-specific data
};

// SystemInfo constructor and destructor
SystemInfo* system_info_create(void);
void system_info_destroy(SystemInfo* self);

// Singleton pattern support
SystemInfo* system_info_get_instance(void);

// Public methods - these call the vtable functions
uint32_t system_info_get_flash_size(void);
uint32_t system_info_get_minimum_free_heap_size(void);
uint32_t system_info_get_free_heap_size(void);
bool system_info_get_mac_address(char* mac_str, size_t size);
const char* system_info_get_chip_model_name(void);
uint32_t system_info_get_chip_revision(void);
uint32_t system_info_get_cpu_cores(void);
uint32_t system_info_get_chip_features(void);
uint32_t system_info_get_cpu_freq_hz(void);
bool system_info_get_temperature(float* temperature);
uint32_t system_info_get_psram_size(void);
uint32_t system_info_get_free_psram_size(void);
const char* system_info_get_app_name(void);
const char* system_info_get_app_version(void);
const char* system_info_get_app_compile_date(void);
const char* system_info_get_app_compile_time(void);
const char* system_info_get_idf_version(void);
bool system_info_get_app_elf_sha256(char* sha256_str, size_t size);
uint64_t system_info_get_uptime_ms(void);
const char* system_info_get_reset_reason(void);
void system_info_reset(void);

// =============================================================================
// 默认实现函数 - 供子类重用
// =============================================================================

/**
 * @brief 默认的应用名称获取实现
 */
const char* system_info_default_get_app_name(SystemInfo* self);

/**
 * @brief 默认的应用版本获取实现
 */
const char* system_info_default_get_app_version(SystemInfo* self);

/**
 * @brief 默认的编译日期获取实现
 */
const char* system_info_default_get_app_compile_date(SystemInfo* self);

/**
 * @brief 默认的编译时间获取实现
 */
const char* system_info_default_get_app_compile_time(SystemInfo* self);

// Macro for declaring system_info subclasses
#define DECLARE_SYSTEM_INFO(system_info_create_func) \
    SystemInfo* create_system_info(void) { \
        return system_info_create_func(); \
    }

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INFO_H