#include "lvgl_image.h"
#include <linx_log.h>
#include <stdlib.h>
#include <string.h>

#define TAG "LvglImage"

/* Raw image implementation */

static const lv_img_dsc_t* lvgl_raw_image_get_dsc_impl(const LvglImage* self) {
    const LvglRawImage* raw_image = (const LvglRawImage*)self;
    return &raw_image->image_dsc;
}

static bool lvgl_raw_image_is_gif_impl(const LvglImage* self) {
    const LvglRawImage* raw_image = (const LvglRawImage*)self;
    const uint8_t* ptr = (const uint8_t*)raw_image->image_dsc.data;
    return ptr && ptr[0] == 'G' && ptr[1] == 'I' && ptr[2] == 'F';
}

static void lvgl_raw_image_destroy_impl(LvglImage* self) {
    LvglRawImage* raw_image = (LvglRawImage*)self;
    if (raw_image) {
        free(raw_image);
    }
}

LvglRawImage* lvgl_raw_image_create(void* data, size_t size) {
    if (!data || size == 0) {
        return NULL;
    }
    
    LvglRawImage* raw_image = (LvglRawImage*)malloc(sizeof(LvglRawImage));
    if (!raw_image) {
        return NULL;
    }
    
    /* Initialize base structure */
    raw_image->base.get_image_dsc = lvgl_raw_image_get_dsc_impl;
    raw_image->base.is_gif = lvgl_raw_image_is_gif_impl;
    raw_image->base.destroy = lvgl_raw_image_destroy_impl;
    
    /* Initialize image descriptor */
    memset(&raw_image->image_dsc, 0, sizeof(raw_image->image_dsc));
    raw_image->image_dsc.data_size = size;
    raw_image->image_dsc.data = (uint8_t*)data;
    raw_image->image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    raw_image->image_dsc.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
    raw_image->image_dsc.header.w = 0;
    raw_image->image_dsc.header.h = 0;
    
    return raw_image;
}

void lvgl_raw_image_destroy(LvglRawImage* image) {
    if (image) {
        lvgl_raw_image_destroy_impl((LvglImage*)image);
    }
}

const lv_img_dsc_t* lvgl_raw_image_get_dsc(const LvglImage* self) {
    return lvgl_raw_image_get_dsc_impl(self);
}

bool lvgl_raw_image_is_gif(const LvglImage* self) {
    return lvgl_raw_image_is_gif_impl(self);
}

/* CBin image implementation */

static const lv_img_dsc_t* lvgl_cbin_image_get_dsc_impl(const LvglImage* self) {
    const LvglCBinImage* cbin_image = (const LvglCBinImage*)self;
    return cbin_image->image_dsc_ptr;
}

static bool lvgl_cbin_image_is_gif_impl(const LvglImage* self) {
    /* CBin images are never GIF */
    (void)self;
    return false;
}

static void lvgl_cbin_image_destroy_impl(LvglImage* self) {
    LvglCBinImage* cbin_image = (LvglCBinImage*)self;
    if (cbin_image) {
        if (cbin_image->image_dsc_ptr) {
            cbin_img_dsc_delete(cbin_image->image_dsc_ptr);
        }
        free(cbin_image);
    }
}

LvglCBinImage* lvgl_cbin_image_create(void* data) {
    if (!data) {
        return NULL;
    }
    
    LvglCBinImage* cbin_image = (LvglCBinImage*)malloc(sizeof(LvglCBinImage));
    if (!cbin_image) {
        return NULL;
    }
    
    /* Initialize base structure */
    cbin_image->base.get_image_dsc = lvgl_cbin_image_get_dsc_impl;
    cbin_image->base.is_gif = lvgl_cbin_image_is_gif_impl;
    cbin_image->base.destroy = lvgl_cbin_image_destroy_impl;
    
    /* Create the image descriptor from binary data */
    cbin_image->image_dsc_ptr = cbin_img_dsc_create((uint8_t*)data);
    if (!cbin_image->image_dsc_ptr) {
        free(cbin_image);
        return NULL;
    }
    
    return cbin_image;
}

void lvgl_cbin_image_destroy(LvglCBinImage* image) {
    if (image) {
        lvgl_cbin_image_destroy_impl((LvglImage*)image);
    }
}

const lv_img_dsc_t* lvgl_cbin_image_get_dsc(const LvglImage* self) {
    return lvgl_cbin_image_get_dsc_impl(self);
}

bool lvgl_cbin_image_is_gif(const LvglImage* self) {
    return lvgl_cbin_image_is_gif_impl(self);
}

/* Source image implementation */

static const lv_img_dsc_t* lvgl_source_image_get_dsc_impl(const LvglImage* self) {
    const LvglSourceImage* source_image = (const LvglSourceImage*)self;
    return source_image->image_dsc_ptr;
}

static bool lvgl_source_image_is_gif_impl(const LvglImage* self) {
    /* Source images are typically not GIF */
    (void)self;
    return false;
}

static void lvgl_source_image_destroy_impl(LvglImage* self) {
    LvglSourceImage* source_image = (LvglSourceImage*)self;
    if (source_image) {
        /* Don't free the image descriptor as it's not owned by us */
        free(source_image);
    }
}

LvglSourceImage* lvgl_source_image_create(const lv_img_dsc_t* image_dsc) {
    if (!image_dsc) {
        return NULL;
    }
    
    LvglSourceImage* source_image = (LvglSourceImage*)malloc(sizeof(LvglSourceImage));
    if (!source_image) {
        return NULL;
    }
    
    /* Initialize base structure */
    source_image->base.get_image_dsc = lvgl_source_image_get_dsc_impl;
    source_image->base.is_gif = lvgl_source_image_is_gif_impl;
    source_image->base.destroy = lvgl_source_image_destroy_impl;
    
    /* Store the image descriptor pointer */
    source_image->image_dsc_ptr = image_dsc;
    
    return source_image;
}

void lvgl_source_image_destroy(LvglSourceImage* image) {
    if (image) {
        lvgl_source_image_destroy_impl((LvglImage*)image);
    }
}

const lv_img_dsc_t* lvgl_source_image_get_dsc(const LvglImage* self) {
    return lvgl_source_image_get_dsc_impl(self);
}

bool lvgl_source_image_is_gif(const LvglImage* self) {
    return lvgl_source_image_is_gif_impl(self);
}

/* Allocated image implementation */

static const lv_img_dsc_t* lvgl_allocated_image_get_dsc_impl(const LvglImage* self) {
    const LvglAllocatedImage* allocated_image = (const LvglAllocatedImage*)self;
    return &allocated_image->image_dsc;
}

static bool lvgl_allocated_image_is_gif_impl(const LvglImage* self) {
    /* Allocated images are typically not GIF */
    (void)self;
    return false;
}

static void lvgl_allocated_image_destroy_impl(LvglImage* self) {
    LvglAllocatedImage* allocated_image = (LvglAllocatedImage*)self;
    if (allocated_image) {
        if (allocated_image->image_dsc.data) {
            /* Free the allocated image data */
            free((void*)allocated_image->image_dsc.data);
        }
        free(allocated_image);
    }
}

LvglAllocatedImage* lvgl_allocated_image_create(void* data, size_t size) {
    if (!data || size == 0) {
        return NULL;
    }
    
    LvglAllocatedImage* allocated_image = (LvglAllocatedImage*)malloc(sizeof(LvglAllocatedImage));
    if (!allocated_image) {
        return NULL;
    }
    
    /* Initialize base structure */
    allocated_image->base.get_image_dsc = lvgl_allocated_image_get_dsc_impl;
    allocated_image->base.is_gif = lvgl_allocated_image_is_gif_impl;
    allocated_image->base.destroy = lvgl_allocated_image_destroy_impl;
    
    /* Initialize image descriptor */
    memset(&allocated_image->image_dsc, 0, sizeof(allocated_image->image_dsc));
    allocated_image->image_dsc.data_size = size;
    allocated_image->image_dsc.data = (uint8_t*)data;
    
    /* Try to get image info using LVGL decoder */
    if (lv_image_decoder_get_info(&allocated_image->image_dsc, &allocated_image->image_dsc.header) != LV_RESULT_OK) {
        LINX_LOGE(TAG, "Failed to get image info, data: %p size: %zu", data, size);
        free(allocated_image);
        return NULL;
    }
    
    return allocated_image;
}

LvglAllocatedImage* lvgl_allocated_image_create_with_params(void* data, size_t size, 
                                                           int width, int height, 
                                                           int stride, int color_format) {
    if (!data || size == 0) {
        return NULL;
    }
    
    LvglAllocatedImage* allocated_image = (LvglAllocatedImage*)malloc(sizeof(LvglAllocatedImage));
    if (!allocated_image) {
        return NULL;
    }
    
    /* Initialize base structure */
    allocated_image->base.get_image_dsc = lvgl_allocated_image_get_dsc_impl;
    allocated_image->base.is_gif = lvgl_allocated_image_is_gif_impl;
    allocated_image->base.destroy = lvgl_allocated_image_destroy_impl;
    
    /* Initialize image descriptor with provided parameters */
    memset(&allocated_image->image_dsc, 0, sizeof(allocated_image->image_dsc));
    allocated_image->image_dsc.data_size = size;
    allocated_image->image_dsc.data = (uint8_t*)data;
    allocated_image->image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    allocated_image->image_dsc.header.cf = color_format;
    allocated_image->image_dsc.header.w = width;
    allocated_image->image_dsc.header.h = height;
    allocated_image->image_dsc.header.stride = stride;
    
    return allocated_image;
}

void lvgl_allocated_image_destroy(LvglAllocatedImage* image) {
    if (image) {
        lvgl_allocated_image_destroy_impl((LvglImage*)image);
    }
}

const lv_img_dsc_t* lvgl_allocated_image_get_dsc(const LvglImage* self) {
    return lvgl_allocated_image_get_dsc_impl(self);
}

bool lvgl_allocated_image_is_gif(const LvglImage* self) {
    return lvgl_allocated_image_is_gif_impl(self);
}