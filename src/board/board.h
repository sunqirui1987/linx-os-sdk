#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct Board Board;
typedef struct BoardVTable BoardVTable;
typedef struct Settings Settings;

// Board vtable structure - contains function pointers for virtual methods
struct BoardVTable {
    // Pure virtual functions (must be implemented by subclasses)
    const char* (*get_board_type)(Board* self);
    void* (*get_audio_codec)(Board* self);
    void* (*get_network)(Board* self);
    void (*start_network)(Board* self);
    const char* (*get_network_state_icon)(Board* self);
    void (*set_power_save_mode)(Board* self, bool enabled);
    const char* (*get_board_json)(Board* self);
    const char* (*get_device_status_json)(Board* self);
    
    // Virtual functions with default implementations
    const char* (*get_uuid)(Board* self);
    void* (*get_backlight)(Board* self);
    void* (*get_led)(Board* self);
    bool (*get_temperature)(Board* self, float* temperature);
    void* (*get_display)(Board* self);
    void* (*get_camera)(Board* self);
    bool (*get_battery_level)(Board* self, int* level, bool* charging, bool* discharging);
    const char* (*get_system_info_json)(Board* self);
    
    // Destructor
    void (*destroy)(Board* self);
};

// Board base class structure
struct Board {
    const BoardVTable* vtable;
    char* uuid_;  // Protected member: software-generated device unique identifier
    void* data;   // Implementation-specific data
};

// Board constructor and destructor
Board* board_create(void);
void board_destroy(Board* self);

// Protected methods (for use by subclasses)
char* board_generate_uuid(void);

// Public methods - these call the vtable functions
const char* board_get_board_type(Board* self);
const char* board_get_uuid(Board* self);
void* board_get_backlight(Board* self);
void* board_get_led(Board* self);
void* board_get_audio_codec(Board* self);
bool board_get_temperature(Board* self, float* temperature);
void* board_get_display(Board* self);
void* board_get_camera(Board* self);
void* board_get_network(Board* self);
void board_start_network(Board* self);
const char* board_get_network_state_icon(Board* self);
bool board_get_battery_level(Board* self, int* level, bool* charging, bool* discharging);
const char* board_get_system_info_json(Board* self);
void board_set_power_save_mode(Board* self, bool enabled);
const char* board_get_board_json(Board* self);
const char* board_get_device_status_json(Board* self);

// Singleton pattern support
Board* board_get_instance(void);

// Macro for declaring board subclasses (equivalent to DECLARE_BOARD macro)
#define DECLARE_BOARD(board_create_func) \
    Board* create_board(void) { \
        return board_create_func(); \
    }

// Additional utility functions for board management

/**
 * Get board settings instance
 * @param self Board instance
 * @return Settings instance or NULL on error
 */
Settings* board_get_settings(Board* self);

/**
 * Get current power save mode status
 * @param self Board instance
 * @return true if power save mode is enabled, false otherwise
 */
bool board_get_power_save_mode(Board* self);

/**
 * Invalidate cached JSON data to force regeneration
 * @param self Board instance
 */
void board_invalidate_cache(Board* self);

/**
 * Save board settings to persistent storage
 * @param self Board instance
 * @return true on success, false on error
 */
bool board_save_settings(Board* self);

/**
 * Reset board settings to defaults
 * @param self Board instance
 * @return true on success, false on error
 */
bool board_reset_settings(Board* self);

#ifdef __cplusplus
}
#endif

#endif // BOARD_H