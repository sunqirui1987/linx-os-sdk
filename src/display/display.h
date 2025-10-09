#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include <stdbool.h>
#include "theme/theme.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display interface structure for C99 compatibility
 */
typedef struct DisplayInterface DisplayInterface;


/**
 * Display interface function pointers
 */
typedef struct {
    int (*init)(DisplayInterface* self);
    void (*set_status)(DisplayInterface* self, const char* status);
    void (*show_notification)(DisplayInterface* self, const char* notification, int duration_ms);
    void (*set_emotion)(DisplayInterface* self, const char* emotion);
    void (*set_chat_message)(DisplayInterface* self, const char* role, const char* content);
    void (*set_theme)(DisplayInterface* self, DisplayTheme* theme);
    DisplayTheme* (*get_theme)(DisplayInterface* self);
    void (*update_status_bar)(DisplayInterface* self, bool update_all);
    void (*set_power_save_mode)(DisplayInterface* self, bool on);
    bool (*lock)(DisplayInterface* self, int timeout_ms);
    void (*unlock)(DisplayInterface* self);
    int (*destroy)(DisplayInterface* self);
} DisplayInterfaceVTable;

/**
 * Base display interface structure
 */
struct DisplayInterface {
    const DisplayInterfaceVTable* vtable;
    void* impl_data;  // Implementation-specific data
    
    // Display properties
    int width;
    int height;
    
    // Current state
    DisplayTheme* current_theme;
    bool is_initialized;
    bool power_save_mode;
    bool is_locked;
    
    // Lock mechanism data
    void* lock_data;  // Implementation-specific lock data
};

/**
 * Initialize display interface
 * @param self Display interface instance
 * @return 0 on success, negative on error
 */
int display_interface_init(DisplayInterface* self);

/**
 * Set display status text
 * @param self Display interface instance
 * @param status Status text to display
 */
void display_interface_set_status(DisplayInterface* self, const char* status);

/**
 * Show notification message
 * @param self Display interface instance
 * @param notification Notification text
 * @param duration_ms Duration in milliseconds (default 3000)
 */
void display_interface_show_notification(DisplayInterface* self, const char* notification, int duration_ms);

/**
 * Set emotion display
 * @param self Display interface instance
 * @param emotion Emotion identifier
 */
void display_interface_set_emotion(DisplayInterface* self, const char* emotion);

/**
 * Set chat message display
 * @param self Display interface instance
 * @param role Message role (user, assistant, etc.)
 * @param content Message content
 */
void display_interface_set_chat_message(DisplayInterface* self, const char* role, const char* content);

/**
 * Set display theme
 * @param self Display interface instance
 * @param theme Theme to apply
 */
void display_interface_set_theme(DisplayInterface* self, DisplayTheme* theme);

/**
 * Get current display theme
 * @param self Display interface instance
 * @return Current theme or NULL
 */
DisplayTheme* display_interface_get_theme(DisplayInterface* self);

/**
 * Update status bar
 * @param self Display interface instance
 * @param update_all Whether to update all status elements
 */
void display_interface_update_status_bar(DisplayInterface* self, bool update_all);

/**
 * Set power save mode
 * @param self Display interface instance
 * @param on Enable/disable power save mode
 */
void display_interface_set_power_save_mode(DisplayInterface* self, bool on);

/**
 * Lock display for exclusive access
 * @param self Display interface instance
 * @param timeout_ms Timeout in milliseconds (0 for no timeout)
 * @return true on success, false on timeout or error
 */
bool display_interface_lock(DisplayInterface* self, int timeout_ms);

/**
 * Unlock display
 * @param self Display interface instance
 */
void display_interface_unlock(DisplayInterface* self);

/**
 * Get display width
 * @param self Display interface instance
 * @return Display width in pixels
 */
int display_interface_get_width(DisplayInterface* self);

/**
 * Get display height
 * @param self Display interface instance
 * @return Display height in pixels
 */
int display_interface_get_height(DisplayInterface* self);

/**
 * Create display interface (stub implementation)
 * @return Display interface instance or NULL on failure
 */
DisplayInterface* display_interface_create(void);

/**
 * Create display interface with specified dimensions
 * @param width Display width
 * @param height Display height
 * @return Display interface instance or NULL on failure
 */
DisplayInterface* display_interface_create_with_size(int width, int height);

/**
 * Destroy display interface
 * @param self Display interface instance
 * @return 0 on success, negative on error
 */
int display_interface_destroy(DisplayInterface* self);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H