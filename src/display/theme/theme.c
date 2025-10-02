#include "theme.h"
#include "log/linx_log.h"
#include <stdlib.h>
#include <string.h>


DisplayTheme* display_theme_create(const char* name) {
    if (!name) {
        LOG_ERROR("Invalid theme name");
        return NULL;
    }
    
    DisplayTheme* theme = (DisplayTheme*)malloc(sizeof(DisplayTheme));
    if (!theme) {
        LOG_ERROR("Failed to allocate memory for theme");
        return NULL;
    }
    
    memset(theme, 0, sizeof(DisplayTheme));
    strncpy(theme->name, name, sizeof(theme->name) - 1);
    theme->name[sizeof(theme->name) - 1] = '\0';
    
    return theme;
}

void display_theme_destroy(DisplayTheme* theme) {
    if (!theme) {
        return;
    }
    
    // Free implementation-specific theme data if needed
    if (theme->theme_data) {
        // Note: Implementation should handle freeing theme_data
        LOG_WARN("Theme data should be freed by implementation");
    }
    
    free(theme);
}