#include "lvgl_gif.h"
#include "log/linx_log.h"
#include <string.h>
#include <stdlib.h>

#define TAG "LvglGif"

/* Forward declarations of internal functions */
static void lvgl_gif_next_frame(lvgl_gif_t* gif);
static void lvgl_gif_cleanup(lvgl_gif_t* gif);
static void lvgl_gif_timer_callback(lv_timer_t* timer);

/* Create a new LVGL GIF instance from image descriptor */
lvgl_gif_t* lvgl_gif_create(const lv_img_dsc_t* img_dsc)
{
    if (!img_dsc || !img_dsc->data) {
        LINX_LOGE(TAG, "Invalid image descriptor");
        return NULL;
    }

    lvgl_gif_t* gif = (lvgl_gif_t*)lv_malloc(sizeof(lvgl_gif_t));
    if (!gif) {
        LINX_LOGE(TAG, "Failed to allocate memory for LVGL GIF");
        return NULL;
    }

    /* Initialize structure */
    memset(gif, 0, sizeof(lvgl_gif_t));
    gif->gif = NULL;
    gif->timer = NULL;
    gif->last_call = 0;
    gif->playing = false;
    gif->loaded = false;
    gif->frame_callback = NULL;

    /* Open GIF from image descriptor data */
    gif->gif = gd_open_gif_data(img_dsc->data);
    if (!gif->gif) {
        LINX_LOGE(TAG, "Failed to open GIF from image descriptor");
        lv_free(gif);
        return NULL;
    }

    /* Setup LVGL image descriptor */
    memset(&gif->img_dsc, 0, sizeof(gif->img_dsc));
    gif->img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    gif->img_dsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE;
    gif->img_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
    gif->img_dsc.header.w = gif->gif->width;
    gif->img_dsc.header.h = gif->gif->height;
    gif->img_dsc.header.stride = gif->gif->width * 4;
    gif->img_dsc.data = gif->gif->canvas;
    gif->img_dsc.data_size = gif->gif->width * gif->gif->height * 4;

    /* Render first frame */
    if (gif->gif->canvas) {
        gd_render_frame(gif->gif, gif->gif->canvas);
    }

    gif->loaded = true;
    LINX_LOGI(TAG, "GIF loaded from image descriptor: %dx%d", gif->gif->width, gif->gif->height);

    return gif;
}

/* Destroy LVGL GIF instance and free resources */
void lvgl_gif_destroy(lvgl_gif_t* gif)
{
    if (!gif) {
        return;
    }

    lvgl_gif_cleanup(gif);
    lv_free(gif);
}

/* Get image descriptor for LVGL */
const lv_img_dsc_t* lvgl_gif_get_image_dsc(const lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded) {
        return NULL;
    }
    return &gif->img_dsc;
}

/* Start/restart GIF animation */
void lvgl_gif_start(lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif) {
        LINX_LOGW(TAG, "GIF not loaded, cannot start");
        return;
    }

    if (!gif->timer) {
        gif->timer = lv_timer_create(lvgl_gif_timer_callback, 10, gif);
    }

    if (gif->timer) {
        gif->playing = true;
        gif->last_call = lv_tick_get();
        lv_timer_resume(gif->timer);
        lv_timer_reset(gif->timer);
        
        /* Render first frame */
        lvgl_gif_next_frame(gif);
        
        LINX_LOGI(TAG, "GIF animation started");
    }
}

/* Pause GIF animation */
void lvgl_gif_pause(lvgl_gif_t* gif)
{
    if (!gif) {
        return;
    }

    if (gif->timer) {
        gif->playing = false;
        lv_timer_pause(gif->timer);
        LINX_LOGI(TAG, "GIF animation paused");
    }
}

/* Resume GIF animation */
void lvgl_gif_resume(lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif) {
        LINX_LOGW(TAG, "GIF not loaded, cannot resume");
        return;
    }

    if (gif->timer) {
        gif->playing = true;
        lv_timer_resume(gif->timer);
        LINX_LOGI(TAG, "GIF animation resumed");
    }
}

/* Stop GIF animation and rewind to first frame */
void lvgl_gif_stop(lvgl_gif_t* gif)
{
    if (!gif) {
        return;
    }

    if (gif->timer) {
        gif->playing = false;
        lv_timer_pause(gif->timer);
    }

    if (gif->gif) {
        gd_rewind(gif->gif);
        lvgl_gif_next_frame(gif);
        LINX_LOGI(TAG, "GIF animation stopped and rewound");
    }
}

/* Check if GIF is currently playing */
bool lvgl_gif_is_playing(const lvgl_gif_t* gif)
{
    if (!gif) {
        return false;
    }
    return gif->playing;
}

/* Check if GIF was loaded successfully */
bool lvgl_gif_is_loaded(const lvgl_gif_t* gif)
{
    if (!gif) {
        return false;
    }
    return gif->loaded;
}

/* Get loop count */
int32_t lvgl_gif_get_loop_count(const lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif) {
        return -1;
    }
    return gif->gif->loop_count;
}

/* Set loop count */
void lvgl_gif_set_loop_count(lvgl_gif_t* gif, int32_t count)
{
    if (!gif || !gif->loaded || !gif->gif) {
        LINX_LOGW(TAG, "GIF not loaded, cannot set loop count");
        return;
    }
    gif->gif->loop_count = count;
}

/* Get GIF width */
uint16_t lvgl_gif_get_width(const lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif) {
        return 0;
    }
    return gif->gif->width;
}

/* Get GIF height */
uint16_t lvgl_gif_get_height(const lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif) {
        return 0;
    }
    return gif->gif->height;
}

/* Set frame update callback */
void lvgl_gif_set_frame_callback(lvgl_gif_t* gif, lvgl_gif_frame_callback_t callback)
{
    if (!gif) {
        return;
    }
    gif->frame_callback = callback;
}

/* Internal function: Update to next frame */
static void lvgl_gif_next_frame(lvgl_gif_t* gif)
{
    if (!gif || !gif->loaded || !gif->gif || !gif->playing) {
        return;
    }

    /* Check if enough time has passed for the next frame */
    uint32_t elapsed = lv_tick_elaps(gif->last_call);
    if (elapsed < gif->gif->gce.delay * 10) {
        return;
    }

    gif->last_call = lv_tick_get();

    /* Get next frame */
    int has_next = gd_get_frame(gif->gif);
    if (has_next == 0) {
        /* Animation finished, pause timer */
        gif->playing = false;
        if (gif->timer) {
            lv_timer_pause(gif->timer);
        }
        LINX_LOGI(TAG, "GIF animation completed");
    }

    /* Render current frame */
    if (gif->gif->canvas) {
        gd_render_frame(gif->gif, gif->gif->canvas);
        
        /* Call frame callback if set */
        if (gif->frame_callback) {
            gif->frame_callback();
        }
    }
}

/* Internal function: Cleanup resources */
static void lvgl_gif_cleanup(lvgl_gif_t* gif)
{
    if (!gif) {
        return;
    }

    /* Stop and delete timer */
    if (gif->timer) {
        lv_timer_delete(gif->timer);
        gif->timer = NULL;
    }

    /* Close GIF decoder */
    if (gif->gif) {
        gd_close_gif(gif->gif);
        gif->gif = NULL;
    }

    gif->playing = false;
    gif->loaded = false;
    
    /* Clear image descriptor */
    memset(&gif->img_dsc, 0, sizeof(gif->img_dsc));
}

/* Timer callback function */
static void lvgl_gif_timer_callback(lv_timer_t* timer)
{
    lvgl_gif_t* gif = (lvgl_gif_t*)lv_timer_get_user_data(timer);
    if (gif) {
        lvgl_gif_next_frame(gif);
    }
}