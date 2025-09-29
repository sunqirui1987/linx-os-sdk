#ifndef LVGL_FONT_H
#define LVGL_FONT_H

#include <lvgl.h>
#include <stddef.h>
#include <stdbool.h>
#include <cbin_font.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declaration of LvglFont structure
 */
typedef struct LvglFont LvglFont;

/**
 * Function pointer type for getting font
 */
typedef const lv_font_t* (*lvgl_font_get_func_t)(const LvglFont* self);

/**
 * Function pointer type for destroying font
 */
typedef void (*lvgl_font_destroy_func_t)(LvglFont* self);

/**
 * Base font structure with virtual function table
 */
struct LvglFont {
    lvgl_font_get_func_t get_font;
    lvgl_font_destroy_func_t destroy;
};

/**
 * Built-in font structure
 */
typedef struct {
    LvglFont base;
    const lv_font_t* font_ptr;
} LvglBuiltInFont;

/**
 * CBin font structure
 */
typedef struct {
    LvglFont base;
    lv_font_t* font_ptr;
} LvglCBinFont;

/**
 * Create a built-in font wrapper
 * @param font Pointer to the built-in LVGL font
 * @return Pointer to LvglBuiltInFont structure, or NULL on failure
 */
LvglBuiltInFont* lvgl_builtin_font_create(const lv_font_t* font);

/**
 * Destroy a built-in font wrapper
 * @param font Pointer to LvglBuiltInFont structure
 */
void lvgl_builtin_font_destroy(LvglBuiltInFont* font);

/**
 * Get font from built-in font wrapper
 * @param self Pointer to LvglFont structure
 * @return Pointer to lv_font_t
 */
const lv_font_t* lvgl_builtin_font_get(const LvglFont* self);

/**
 * Create a CBin font from binary data
 * @param data Pointer to binary font data
 * @return Pointer to LvglCBinFont structure, or NULL on failure
 */
LvglCBinFont* lvgl_cbin_font_create(void* data);

/**
 * Destroy a CBin font
 * @param font Pointer to LvglCBinFont structure
 */
void lvgl_cbin_font_destroy(LvglCBinFont* font);

/**
 * Get font from CBin font wrapper
 * @param self Pointer to LvglFont structure
 * @return Pointer to lv_font_t
 */
const lv_font_t* lvgl_cbin_font_get(const LvglFont* self);

/**
 * Generic font getter function
 * @param font Pointer to LvglFont structure
 * @return Pointer to lv_font_t
 */
static inline const lv_font_t* lvgl_font_get(const LvglFont* font) {
    if (font && font->get_font) {
        return font->get_font(font);
    }
    return NULL;
}

/**
 * Generic font destroyer function
 * @param font Pointer to LvglFont structure
 */
static inline void lvgl_font_destroy(LvglFont* font) {
    if (font && font->destroy) {
        font->destroy(font);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* LVGL_FONT_H */