#ifndef LVGL_THEME_H
#define LVGL_THEME_H


#include "../theme/theme.h"
#include "lvgl_image.h"
#include "lvgl_font.h"
#include "emoji_collection.h"

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */
typedef struct LvglTheme LvglTheme;
typedef struct LvglThemeManager LvglThemeManager;

/**
 * LVGL Theme structure
 * Extends DisplayTheme with LVGL-specific properties
 */
struct LvglTheme {
    DisplayTheme base;  // Base theme structure
    
    // Theme properties
    int spacing;
    
    // Colors
    lv_color_t background_color;
    lv_color_t text_color;
    lv_color_t chat_background_color;
    lv_color_t user_bubble_color;
    lv_color_t assistant_bubble_color;
    lv_color_t system_bubble_color;
    lv_color_t system_text_color;
    lv_color_t border_color;
    lv_color_t low_battery_color;
    
    // Resources
    LvglImage* background_image;
    LvglFont* text_font;
    LvglFont* icon_font;
    LvglFont* large_icon_font;
    EmojiCollection* emoji_collection;
};

/**
 * Theme manager entry structure for storing themes
 */
typedef struct ThemeEntry {
    char name[64];
    LvglTheme* theme;
    struct ThemeEntry* next;
} ThemeEntry;

/**
 * LVGL Theme Manager structure
 * Manages theme registration and retrieval
 */
struct LvglThemeManager {
    ThemeEntry* themes;  // Linked list of themes
};

/**
 * Parse color from string format (e.g., "#112233")
 * @param color Color string in hex format
 * @return Parsed lv_color_t
 */
lv_color_t lvgl_theme_parse_color(const char* color);

/**
 * Create a new LVGL theme
 * @param name Theme name
 * @return New LvglTheme instance or NULL on failure
 */
LvglTheme* lvgl_theme_create(const char* name);

/**
 * Destroy LVGL theme and free resources
 * @param theme Theme to destroy
 */
void lvgl_theme_destroy(LvglTheme* theme);

/**
 * Get theme spacing with scale factor
 * @param theme Theme instance
 * @param scale Scale factor
 * @return Scaled spacing value
 */
int lvgl_theme_get_spacing(const LvglTheme* theme, int scale);

// Color getters
lv_color_t lvgl_theme_get_background_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_text_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_chat_background_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_user_bubble_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_assistant_bubble_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_system_bubble_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_system_text_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_border_color(const LvglTheme* theme);
lv_color_t lvgl_theme_get_low_battery_color(const LvglTheme* theme);

// Resource getters
LvglImage* lvgl_theme_get_background_image(const LvglTheme* theme);
EmojiCollection* lvgl_theme_get_emoji_collection(const LvglTheme* theme);
LvglFont* lvgl_theme_get_text_font(const LvglTheme* theme);
LvglFont* lvgl_theme_get_icon_font(const LvglTheme* theme);
LvglFont* lvgl_theme_get_large_icon_font(const LvglTheme* theme);

// Color setters
void lvgl_theme_set_background_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_text_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_chat_background_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_user_bubble_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_assistant_bubble_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_system_bubble_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_system_text_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_border_color(LvglTheme* theme, lv_color_t color);
void lvgl_theme_set_low_battery_color(LvglTheme* theme, lv_color_t color);

// Resource setters
void lvgl_theme_set_background_image(LvglTheme* theme, LvglImage* image);
void lvgl_theme_set_emoji_collection(LvglTheme* theme, EmojiCollection* collection);
void lvgl_theme_set_text_font(LvglTheme* theme, LvglFont* font);
void lvgl_theme_set_icon_font(LvglTheme* theme, LvglFont* font);
void lvgl_theme_set_large_icon_font(LvglTheme* theme, LvglFont* font);

/**
 * Get singleton instance of theme manager
 * @return LvglThemeManager instance
 */
LvglThemeManager* lvgl_theme_manager_get_instance(void);

/**
 * Register a theme with the manager
 * @param manager Theme manager instance
 * @param theme_name Theme name
 * @param theme Theme instance
 */
void lvgl_theme_manager_register_theme(LvglThemeManager* manager, const char* theme_name, LvglTheme* theme);

/**
 * Get a theme by name
 * @param manager Theme manager instance
 * @param theme_name Theme name
 * @return Theme instance or NULL if not found
 */
LvglTheme* lvgl_theme_manager_get_theme(LvglThemeManager* manager, const char* theme_name);

/**
 * Initialize default themes
 * @param manager Theme manager instance
 */
void lvgl_theme_manager_initialize_default_themes(LvglThemeManager* manager);

/**
 * Destroy theme manager and free all resources
 * @param manager Theme manager instance
 */
void lvgl_theme_manager_destroy(LvglThemeManager* manager);

#ifdef __cplusplus
}
#endif

#endif // LVGL_THEME_H