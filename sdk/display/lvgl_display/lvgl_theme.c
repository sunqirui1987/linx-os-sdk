#include "lvgl_theme.h"
#include "../log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Log tag for theme module
#define TAG "THEME"

// Static theme manager instance for singleton pattern
static LvglThemeManager* g_theme_manager = NULL;

/**
 * Parse color from string format (e.g., "#112233")
 */
lv_color_t lvgl_theme_parse_color(const char* color) {
    if (!color || strlen(color) < 7 || color[0] != '#') {
        LINX_LOGE(TAG, "Invalid color format: %s", color ? color : "NULL");
        return lv_color_black();
    }
    
    // Convert #112233 to lv_color_t
    char r_str[3] = {color[1], color[2], '\0'};
    char g_str[3] = {color[3], color[4], '\0'};
    char b_str[3] = {color[5], color[6], '\0'};
    
    uint8_t r = (uint8_t)strtol(r_str, NULL, 16);
    uint8_t g = (uint8_t)strtol(g_str, NULL, 16);
    uint8_t b = (uint8_t)strtol(b_str, NULL, 16);
    
    return lv_color_make(r, g, b);
}

/**
 * Create a new LVGL theme
 */
LvglTheme* lvgl_theme_create(const char* name) {
    if (!name) {
        LINX_LOGE(TAG, "Invalid theme name");
        return NULL;
    }
    
    LvglTheme* theme = (LvglTheme*)malloc(sizeof(LvglTheme));
    if (!theme) {
        LINX_LOGE(TAG, "Failed to allocate memory for LvglTheme");
        return NULL;
    }
    
    // Initialize base theme
    memset(theme, 0, sizeof(LvglTheme));
    strncpy(theme->base.name, name, sizeof(theme->base.name) - 1);
    theme->base.name[sizeof(theme->base.name) - 1] = '\0';
    theme->base.theme_data = theme;  // Point to self for type identification
    
    // Initialize default values
    theme->spacing = 2;
    
    // Initialize colors to default values
    theme->background_color = lv_color_white();
    theme->text_color = lv_color_black();
    theme->chat_background_color = lv_color_white();
    theme->user_bubble_color = lv_color_make(0x00, 0x7A, 0xFF);
    theme->assistant_bubble_color = lv_color_make(0xF0, 0xF0, 0xF0);
    theme->system_bubble_color = lv_color_make(0xFF, 0xE0, 0x00);
    theme->system_text_color = lv_color_black();
    theme->border_color = lv_color_make(0xE0, 0xE0, 0xE0);
    theme->low_battery_color = lv_color_make(0xFF, 0x00, 0x00);
    
    // Initialize resource pointers to NULL
    theme->background_image = NULL;
    theme->text_font = NULL;
    theme->icon_font = NULL;
    theme->large_icon_font = NULL;
    theme->emoji_collection = NULL;
    
    return theme;
}

/**
 * Destroy LVGL theme and free resources
 */
void lvgl_theme_destroy(LvglTheme* theme) {
    if (!theme) {
        return;
    }
    
    // Note: We don't free the resource pointers (fonts, images, etc.) 
    // as they may be shared between themes. The caller is responsible
    // for managing their lifecycle.
    
    free(theme);
}

/**
 * Get theme spacing with scale factor
 */
int lvgl_theme_get_spacing(const LvglTheme* theme, int scale) {
    if (!theme) {
        return 0;
    }
    return theme->spacing * scale;
}

// Color getters
lv_color_t lvgl_theme_get_background_color(const LvglTheme* theme) {
    return theme ? theme->background_color : lv_color_black();
}

lv_color_t lvgl_theme_get_text_color(const LvglTheme* theme) {
    return theme ? theme->text_color : lv_color_black();
}

lv_color_t lvgl_theme_get_chat_background_color(const LvglTheme* theme) {
    return theme ? theme->chat_background_color : lv_color_black();
}

lv_color_t lvgl_theme_get_user_bubble_color(const LvglTheme* theme) {
    return theme ? theme->user_bubble_color : lv_color_black();
}

lv_color_t lvgl_theme_get_assistant_bubble_color(const LvglTheme* theme) {
    return theme ? theme->assistant_bubble_color : lv_color_black();
}

lv_color_t lvgl_theme_get_system_bubble_color(const LvglTheme* theme) {
    return theme ? theme->system_bubble_color : lv_color_black();
}

lv_color_t lvgl_theme_get_system_text_color(const LvglTheme* theme) {
    return theme ? theme->system_text_color : lv_color_black();
}

lv_color_t lvgl_theme_get_border_color(const LvglTheme* theme) {
    return theme ? theme->border_color : lv_color_black();
}

lv_color_t lvgl_theme_get_low_battery_color(const LvglTheme* theme) {
    return theme ? theme->low_battery_color : lv_color_black();
}

// Resource getters
LvglImage* lvgl_theme_get_background_image(const LvglTheme* theme) {
    return theme ? theme->background_image : NULL;
}

EmojiCollection* lvgl_theme_get_emoji_collection(const LvglTheme* theme) {
    return theme ? theme->emoji_collection : NULL;
}

LvglFont* lvgl_theme_get_text_font(const LvglTheme* theme) {
    return theme ? theme->text_font : NULL;
}

LvglFont* lvgl_theme_get_icon_font(const LvglTheme* theme) {
    return theme ? theme->icon_font : NULL;
}

LvglFont* lvgl_theme_get_large_icon_font(const LvglTheme* theme) {
    return theme ? theme->large_icon_font : NULL;
}

// Color setters
void lvgl_theme_set_background_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->background_color = color;
    }
}

void lvgl_theme_set_text_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->text_color = color;
    }
}

void lvgl_theme_set_chat_background_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->chat_background_color = color;
    }
}

void lvgl_theme_set_user_bubble_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->user_bubble_color = color;
    }
}

void lvgl_theme_set_assistant_bubble_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->assistant_bubble_color = color;
    }
}

void lvgl_theme_set_system_bubble_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->system_bubble_color = color;
    }
}

void lvgl_theme_set_system_text_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->system_text_color = color;
    }
}

void lvgl_theme_set_border_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->border_color = color;
    }
}

void lvgl_theme_set_low_battery_color(LvglTheme* theme, lv_color_t color) {
    if (theme) {
        theme->low_battery_color = color;
    }
}

// Resource setters
void lvgl_theme_set_background_image(LvglTheme* theme, LvglImage* image) {
    if (theme) {
        theme->background_image = image;
    }
}

void lvgl_theme_set_emoji_collection(LvglTheme* theme, EmojiCollection* collection) {
    if (theme) {
        theme->emoji_collection = collection;
    }
}

void lvgl_theme_set_text_font(LvglTheme* theme, LvglFont* font) {
    if (theme) {
        theme->text_font = font;
    }
}

void lvgl_theme_set_icon_font(LvglTheme* theme, LvglFont* font) {
    if (theme) {
        theme->icon_font = font;
    }
}

void lvgl_theme_set_large_icon_font(LvglTheme* theme, LvglFont* font) {
    if (theme) {
        theme->large_icon_font = font;
    }
}

/**
 * Get singleton instance of theme manager
 */
LvglThemeManager* lvgl_theme_manager_get_instance(void) {
    if (!g_theme_manager) {
        g_theme_manager = (LvglThemeManager*)malloc(sizeof(LvglThemeManager));
        if (g_theme_manager) {
            g_theme_manager->themes = NULL;
        } else {
            LINX_LOGE(TAG, "Failed to allocate memory for LvglThemeManager");
        }
    }
    return g_theme_manager;
}

/**
 * Register a theme with the manager
 */
void lvgl_theme_manager_register_theme(LvglThemeManager* manager, const char* theme_name, LvglTheme* theme) {
    if (!manager || !theme_name || !theme) {
        LINX_LOGE(TAG, "Invalid parameters for theme registration");
        return;
    }
    
    // Check if theme already exists
    ThemeEntry* current = manager->themes;
    while (current) {
        if (strcmp(current->name, theme_name) == 0) {
            // Update existing theme
            current->theme = theme;
            return;
        }
        current = current->next;
    }
    
    // Create new theme entry
    ThemeEntry* entry = (ThemeEntry*)malloc(sizeof(ThemeEntry));
    if (!entry) {
        LINX_LOGE(TAG, "Failed to allocate memory for theme entry");
        return;
    }
    
    strncpy(entry->name, theme_name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->theme = theme;
    entry->next = manager->themes;
    manager->themes = entry;
}

/**
 * Get a theme by name
 */
LvglTheme* lvgl_theme_manager_get_theme(LvglThemeManager* manager, const char* theme_name) {
    if (!manager || !theme_name) {
        return NULL;
    }
    
    ThemeEntry* current = manager->themes;
    while (current) {
        if (strcmp(current->name, theme_name) == 0) {
            return current->theme;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * Initialize default themes
 */
void lvgl_theme_manager_initialize_default_themes(LvglThemeManager* manager) {
    if (!manager) {
        return;
    }
    
    // Create a default light theme
    LvglTheme* light_theme = lvgl_theme_create("light");
    if (light_theme) {
        lvgl_theme_set_background_color(light_theme, lv_color_white());
        lvgl_theme_set_text_color(light_theme, lv_color_black());
        lvgl_theme_manager_register_theme(manager, "light", light_theme);
    }
    
    // Create a default dark theme
    LvglTheme* dark_theme = lvgl_theme_create("dark");
    if (dark_theme) {
        lvgl_theme_set_background_color(dark_theme, lv_color_make(0x20, 0x20, 0x20));
        lvgl_theme_set_text_color(dark_theme, lv_color_white());
        lvgl_theme_set_chat_background_color(dark_theme, lv_color_make(0x30, 0x30, 0x30));
        lvgl_theme_manager_register_theme(manager, "dark", dark_theme);
    }
}

/**
 * Destroy theme manager and free all resources
 */
void lvgl_theme_manager_destroy(LvglThemeManager* manager) {
    if (!manager) {
        return;
    }
    
    // Free all theme entries
    ThemeEntry* current = manager->themes;
    while (current) {
        ThemeEntry* next = current->next;
        
        // Destroy the theme
        if (current->theme) {
            lvgl_theme_destroy(current->theme);
        }
        
        free(current);
        current = next;
    }
    
    free(manager);
    
    // Reset global instance if it's the same
    if (g_theme_manager == manager) {
        g_theme_manager = NULL;
    }
}