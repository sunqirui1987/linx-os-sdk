#ifndef DISPLAY_STUB_H
#define DISPLAY_STUB_H

#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stub display implementation data structure
 * This is a placeholder implementation for platforms without display support
 */
typedef struct {
    bool initialized;
    bool power_save_mode;
    bool is_locked;
    char last_status[256];
    char last_notification[256];
    char last_emotion[64];
    char last_role[64];
    char last_content[512];
    DisplayTheme* current_theme;
} DisplayStubData;

/**
 * Create stub display implementation
 * @return DisplayInterface instance or NULL on failure
 */
DisplayInterface* display_stub_create(void);

/**
 * Create stub display implementation with specified dimensions
 * @param width Display width
 * @param height Display height
 * @return DisplayInterface instance or NULL on failure
 */
DisplayInterface* display_stub_create_with_size(int width, int height);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_STUB_H