#include "lvgl_font.h"
#include <stdlib.h>
#include <string.h>

/* Built-in font implementation */

static const lv_font_t* lvgl_builtin_font_get_impl(const LvglFont* self) {
    const LvglBuiltInFont* builtin_font = (const LvglBuiltInFont*)self;
    return builtin_font->font_ptr;
}

static void lvgl_builtin_font_destroy_impl(LvglFont* self) {
    LvglBuiltInFont* builtin_font = (LvglBuiltInFont*)self;
    if (builtin_font) {
        free(builtin_font);
    }
}

LvglBuiltInFont* lvgl_builtin_font_create(const lv_font_t* font) {
    if (!font) {
        return NULL;
    }
    
    LvglBuiltInFont* builtin_font = (LvglBuiltInFont*)malloc(sizeof(LvglBuiltInFont));
    if (!builtin_font) {
        return NULL;
    }
    
    /* Initialize base structure */
    builtin_font->base.get_font = lvgl_builtin_font_get_impl;
    builtin_font->base.destroy = lvgl_builtin_font_destroy_impl;
    
    /* Initialize built-in font specific data */
    builtin_font->font_ptr = font;
    
    return builtin_font;
}

void lvgl_builtin_font_destroy(LvglBuiltInFont* font) {
    if (font) {
        lvgl_builtin_font_destroy_impl((LvglFont*)font);
    }
}

const lv_font_t* lvgl_builtin_font_get(const LvglFont* self) {
    return lvgl_builtin_font_get_impl(self);
}

/* CBin font implementation */

static const lv_font_t* lvgl_cbin_font_get_impl(const LvglFont* self) {
    const LvglCBinFont* cbin_font = (const LvglCBinFont*)self;
    return cbin_font->font_ptr;
}

static void lvgl_cbin_font_destroy_impl(LvglFont* self) {
    LvglCBinFont* cbin_font = (LvglCBinFont*)self;
    if (cbin_font) {
        if (cbin_font->font_ptr) {
            cbin_font_delete(cbin_font->font_ptr);
        }
        free(cbin_font);
    }
}

LvglCBinFont* lvgl_cbin_font_create(void* data) {
    if (!data) {
        return NULL;
    }
    
    LvglCBinFont* cbin_font = (LvglCBinFont*)malloc(sizeof(LvglCBinFont));
    if (!cbin_font) {
        return NULL;
    }
    
    /* Initialize base structure */
    cbin_font->base.get_font = lvgl_cbin_font_get_impl;
    cbin_font->base.destroy = lvgl_cbin_font_destroy_impl;
    
    /* Create the font from binary data */
    cbin_font->font_ptr = cbin_font_create((uint8_t*)data);
    if (!cbin_font->font_ptr) {
        free(cbin_font);
        return NULL;
    }
    
    return cbin_font;
}

void lvgl_cbin_font_destroy(LvglCBinFont* font) {
    if (font) {
        lvgl_cbin_font_destroy_impl((LvglFont*)font);
    }
}

const lv_font_t* lvgl_cbin_font_get(const LvglFont* self) {
    return lvgl_cbin_font_get_impl(self);
}