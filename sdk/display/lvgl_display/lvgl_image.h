#ifndef LVGL_IMAGE_H
#define LVGL_IMAGE_H

#include <lvgl.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <cbin_font.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declaration of LvglImage structure
 */
typedef struct LvglImage LvglImage;

/**
 * Function pointer type for getting image descriptor
 */
typedef const lv_img_dsc_t* (*lvgl_image_get_dsc_func_t)(const LvglImage* self);

/**
 * Function pointer type for checking if image is GIF
 */
typedef bool (*lvgl_image_is_gif_func_t)(const LvglImage* self);

/**
 * Function pointer type for destroying image
 */
typedef void (*lvgl_image_destroy_func_t)(LvglImage* self);

/**
 * Base image structure with virtual function table
 */
struct LvglImage {
    lvgl_image_get_dsc_func_t get_image_dsc;
    lvgl_image_is_gif_func_t is_gif;
    lvgl_image_destroy_func_t destroy;
};



/**
 * Raw image structure
 */
typedef struct {
    LvglImage base;
    lv_img_dsc_t image_dsc;
} LvglRawImage;

/**
 * CBin image structure
 */
typedef struct {
    LvglImage base;
    lv_img_dsc_t* image_dsc_ptr;
} LvglCBinImage;

/**
 * Source image structure (wrapper for existing image descriptor)
 */
typedef struct {
    LvglImage base;
    const lv_img_dsc_t* image_dsc_ptr;
} LvglSourceImage;

/**
 * Allocated image structure
 */
typedef struct {
    LvglImage base;
    lv_img_dsc_t image_dsc;
} LvglAllocatedImage;

/**
 * Create a raw image from data
 * @param data Pointer to image data
 * @param size Size of image data
 * @return Pointer to LvglRawImage structure, or NULL on failure
 */
LvglRawImage* lvgl_raw_image_create(void* data, size_t size);

/**
 * Destroy a raw image
 * @param image Pointer to LvglRawImage structure
 */
void lvgl_raw_image_destroy(LvglRawImage* image);

/**
 * Get image descriptor from raw image
 * @param self Pointer to LvglImage structure
 * @return Pointer to lv_img_dsc_t
 */
const lv_img_dsc_t* lvgl_raw_image_get_dsc(const LvglImage* self);

/**
 * Check if raw image is GIF
 * @param self Pointer to LvglImage structure
 * @return true if image is GIF, false otherwise
 */
bool lvgl_raw_image_is_gif(const LvglImage* self);

/**
 * Create a CBin image from binary data
 * @param data Pointer to binary image data
 * @return Pointer to LvglCBinImage structure, or NULL on failure
 */
LvglCBinImage* lvgl_cbin_image_create(void* data);

/**
 * Destroy a CBin image
 * @param image Pointer to LvglCBinImage structure
 */
void lvgl_cbin_image_destroy(LvglCBinImage* image);

/**
 * Get image descriptor from CBin image
 * @param self Pointer to LvglImage structure
 * @return Pointer to lv_img_dsc_t
 */
const lv_img_dsc_t* lvgl_cbin_image_get_dsc(const LvglImage* self);

/**
 * Check if CBin image is GIF (always false for CBin)
 * @param self Pointer to LvglImage structure
 * @return false
 */
bool lvgl_cbin_image_is_gif(const LvglImage* self);

/**
 * Create a source image wrapper
 * @param image_dsc Pointer to existing image descriptor
 * @return Pointer to LvglSourceImage structure, or NULL on failure
 */
LvglSourceImage* lvgl_source_image_create(const lv_img_dsc_t* image_dsc);

/**
 * Destroy a source image wrapper
 * @param image Pointer to LvglSourceImage structure
 */
void lvgl_source_image_destroy(LvglSourceImage* image);

/**
 * Get image descriptor from source image
 * @param self Pointer to LvglImage structure
 * @return Pointer to lv_img_dsc_t
 */
const lv_img_dsc_t* lvgl_source_image_get_dsc(const LvglImage* self);

/**
 * Check if source image is GIF (always false)
 * @param self Pointer to LvglImage structure
 * @return false
 */
bool lvgl_source_image_is_gif(const LvglImage* self);

/**
 * Create an allocated image from data (auto-detect format)
 * @param data Pointer to image data
 * @param size Size of image data
 * @return Pointer to LvglAllocatedImage structure, or NULL on failure
 */
LvglAllocatedImage* lvgl_allocated_image_create(void* data, size_t size);

/**
 * Create an allocated image with specific parameters
 * @param data Pointer to image data
 * @param size Size of image data
 * @param width Image width
 * @param height Image height
 * @param stride Image stride
 * @param color_format Color format
 * @return Pointer to LvglAllocatedImage structure, or NULL on failure
 */
LvglAllocatedImage* lvgl_allocated_image_create_with_params(void* data, size_t size, 
                                                           int width, int height, 
                                                           int stride, int color_format);

/**
 * Destroy an allocated image
 * @param image Pointer to LvglAllocatedImage structure
 */
void lvgl_allocated_image_destroy(LvglAllocatedImage* image);

/**
 * Get image descriptor from allocated image
 * @param self Pointer to LvglImage structure
 * @return Pointer to lv_img_dsc_t
 */
const lv_img_dsc_t* lvgl_allocated_image_get_dsc(const LvglImage* self);

/**
 * Check if allocated image is GIF (always false)
 * @param self Pointer to LvglImage structure
 * @return false
 */
bool lvgl_allocated_image_is_gif(const LvglImage* self);

/**
 * Generic image descriptor getter function
 * @param image Pointer to LvglImage structure
 * @return Pointer to lv_img_dsc_t
 */
static inline const lv_img_dsc_t* lvgl_image_get_dsc(const LvglImage* image) {
    if (image && image->get_image_dsc) {
        return image->get_image_dsc(image);
    }
    return NULL;
}

/**
 * Generic image GIF checker function
 * @param image Pointer to LvglImage structure
 * @return true if image is GIF, false otherwise
 */
static inline bool lvgl_image_is_gif(const LvglImage* image) {
    if (image && image->is_gif) {
        return image->is_gif(image);
    }
    return false;
}

/**
 * Generic image destroyer function
 * @param image Pointer to LvglImage structure
 */
static inline void lvgl_image_destroy(LvglImage* image) {
    if (image && image->destroy) {
        image->destroy(image);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* LVGL_IMAGE_H */