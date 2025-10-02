#ifndef LVGL_GIF_H
#define LVGL_GIF_H

#ifdef __cplusplus
extern "C" {
#endif


#include "lvgl/lvgl.h"
#include "lvgl/src/libs/gif/gifdec.h"

/**
 * C implementation of LVGL GIF widget
 * Provides GIF animation functionality using gifdec library
 */



/* Frame callback function pointer type */
typedef void (*lvgl_gif_frame_callback_t)(void);

/* LVGL GIF structure */
typedef struct lvgl_gif_t {
    /* GIF decoder instance */
    gd_GIF* gif;
    
    /* LVGL image descriptor */
    lv_img_dsc_t img_dsc;
    
    /* Animation timer */
    lv_timer_t* timer;
    
    /* Last frame update time */
    uint32_t last_call;
    
    /* Animation state */
    bool playing;
    bool loaded;
    
    /* Frame update callback */
    lvgl_gif_frame_callback_t frame_callback;
} lvgl_gif_t;

/**
 * Create a new LVGL GIF instance from image descriptor
 * @param img_dsc Image descriptor containing GIF data
 * @return Pointer to lvgl_gif_t instance or NULL on failure
 */
lvgl_gif_t* lvgl_gif_create(const lv_img_dsc_t* img_dsc);

/**
 * Destroy LVGL GIF instance and free resources
 * @param gif Pointer to lvgl_gif_t instance
 */
void lvgl_gif_destroy(lvgl_gif_t* gif);

/**
 * Get image descriptor for LVGL
 * @param gif Pointer to lvgl_gif_t instance
 * @return Pointer to image descriptor or NULL if not loaded
 */
const lv_img_dsc_t* lvgl_gif_get_image_dsc(const lvgl_gif_t* gif);

/**
 * Start/restart GIF animation
 * @param gif Pointer to lvgl_gif_t instance
 */
void lvgl_gif_start(lvgl_gif_t* gif);

/**
 * Pause GIF animation
 * @param gif Pointer to lvgl_gif_t instance
 */
void lvgl_gif_pause(lvgl_gif_t* gif);

/**
 * Resume GIF animation
 * @param gif Pointer to lvgl_gif_t instance
 */
void lvgl_gif_resume(lvgl_gif_t* gif);

/**
 * Stop GIF animation and rewind to first frame
 * @param gif Pointer to lvgl_gif_t instance
 */
void lvgl_gif_stop(lvgl_gif_t* gif);

/**
 * Check if GIF is currently playing
 * @param gif Pointer to lvgl_gif_t instance
 * @return true if playing, false otherwise
 */
bool lvgl_gif_is_playing(const lvgl_gif_t* gif);

/**
 * Check if GIF was loaded successfully
 * @param gif Pointer to lvgl_gif_t instance
 * @return true if loaded, false otherwise
 */
bool lvgl_gif_is_loaded(const lvgl_gif_t* gif);

/**
 * Get loop count
 * @param gif Pointer to lvgl_gif_t instance
 * @return Loop count (-1 for infinite)
 */
int32_t lvgl_gif_get_loop_count(const lvgl_gif_t* gif);

/**
 * Set loop count
 * @param gif Pointer to lvgl_gif_t instance
 * @param count Loop count (-1 for infinite, 0 for no loop, >0 for specific count)
 */
void lvgl_gif_set_loop_count(lvgl_gif_t* gif, int32_t count);

/**
 * Get GIF width
 * @param gif Pointer to lvgl_gif_t instance
 * @return Width in pixels
 */
uint16_t lvgl_gif_get_width(const lvgl_gif_t* gif);

/**
 * Get GIF height
 * @param gif Pointer to lvgl_gif_t instance
 * @return Height in pixels
 */
uint16_t lvgl_gif_get_height(const lvgl_gif_t* gif);

/**
 * Set frame update callback
 * @param gif Pointer to lvgl_gif_t instance
 * @param callback Function to call on each frame update
 */
void lvgl_gif_set_frame_callback(lvgl_gif_t* gif, lvgl_gif_frame_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_GIF_H */