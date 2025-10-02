#include "lvgl.h"
#include <stdlib.h>

/**
 * TUYA custom LVGL functions implementation
 * These functions are called by modified LVGL code but not implemented
 * Providing empty implementations to resolve linking errors
 */

/**
 * Handle LVGL messages - called from lv_timer.c
 */
void lvMsgHandle(void)
{
    // Empty implementation - can be extended for custom message handling
}

/**
 * Register event for object - called from lv_obj_event.c
 * @param obj pointer to the object
 * @param eventCode event code to register
 */
void lvMsgEventReg(lv_obj_t *obj, lv_event_code_t eventCode)
{
    // Empty implementation - can be extended for custom event registration
    (void)obj;      // Suppress unused parameter warning
    (void)eventCode; // Suppress unused parameter warning
}

/**
 * Delete event for object - called from lv_obj_tree.c
 * @param obj pointer to the object
 */
void lvMsgEventDel(lv_obj_t *obj)
{
    // Empty implementation - can be extended for custom event deletion
    (void)obj; // Suppress unused parameter warning
}

/**
 * LVGL memory management functions
 * These are required when LV_USE_STDLIB_MALLOC is set to LV_STDLIB_CUSTOM
 */

/**
 * Core malloc function - wrapper around standard malloc
 * @param size size in bytes to allocate
 * @return pointer to allocated memory or NULL on failure
 */
void * lv_malloc_core(size_t size)
{
    return malloc(size);
}

/**
 * Core free function - wrapper around standard free
 * @param p pointer to memory to free
 */
void lv_free_core(void * p)
{
    free(p);
}

/**
 * Core realloc function - wrapper around standard realloc
 * @param p pointer to memory to reallocate
 * @param new_size new size in bytes
 * @return pointer to reallocated memory or NULL on failure
 */
void * lv_realloc_core(void * p, size_t new_size)
{
    return realloc(p, new_size);
}

/**
 * Memory deinitialization function
 * Called when LVGL is deinitialized
 */
void lv_mem_deinit(void)
{
    // Empty implementation - no cleanup needed for standard malloc/free
}