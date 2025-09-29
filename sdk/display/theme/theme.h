#ifndef THEME_H
#define THEME_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Theme structure for display theming
 */
typedef struct {
    char name[64];
    void* theme_data;  // Implementation-specific theme data
} DisplayTheme;

/**
 * Create a new theme
 * @param name Theme name
 * @return New theme instance or NULL on failure
 */
DisplayTheme* display_theme_create(const char* name);

/**
 * Destroy theme and free resources
 * @param theme Theme to destroy
 */
void display_theme_destroy(DisplayTheme* theme);


#ifdef __cplusplus
}
#endif

#endif // THEME_H
