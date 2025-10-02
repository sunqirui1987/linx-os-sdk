#ifndef EMOJI_COLLECTION_H
#define EMOJI_COLLECTION_H

#include <lvgl.h>
#include <stddef.h>
#include <stdbool.h>
#include "lvgl_image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum number of emojis that can be stored in a collection
 */
#define MAX_EMOJI_COUNT 32

/**
 * Maximum length for emoji name
 */
#define MAX_EMOJI_NAME_LEN 32

// LvglImage is defined in lvgl_image.h

/**
 * Emoji entry structure
 */
typedef struct {
    char name[MAX_EMOJI_NAME_LEN];
    LvglImage* image;
    bool is_used;
} EmojiEntry;

/**
 * Emoji collection structure
 */
typedef struct {
    EmojiEntry entries[MAX_EMOJI_COUNT];
    size_t count;
} EmojiCollection;

/**
 * Initialize an emoji collection
 * @param collection Pointer to the emoji collection to initialize
 * @return 0 on success, -1 on failure
 */
int emoji_collection_init(EmojiCollection* collection);

/**
 * Add an emoji to the collection
 * @param collection Pointer to the emoji collection
 * @param name Name of the emoji (null-terminated string)
 * @param image Pointer to the LVGL image
 * @return 0 on success, -1 on failure (collection full or invalid parameters)
 */
int emoji_collection_add(EmojiCollection* collection, const char* name, LvglImage* image);

/**
 * Get an emoji image by name
 * @param collection Pointer to the emoji collection
 * @param name Name of the emoji to find
 * @return Pointer to the LvglImage if found, NULL otherwise
 */
const LvglImage* emoji_collection_get(const EmojiCollection* collection, const char* name);

/**
 * Destroy an emoji collection and free all associated resources
 * @param collection Pointer to the emoji collection to destroy
 */
void emoji_collection_destroy(EmojiCollection* collection);



/**
 * Initialize a Twemoji 32px collection with predefined emojis
 * @param collection Pointer to the emoji collection to initialize
 * @return 0 on success, -1 on failure
 */
int twemoji32_init(EmojiCollection* collection);

/**
 * Initialize a Twemoji 64px collection with predefined emojis
 * @param collection Pointer to the emoji collection to initialize
 * @return 0 on success, -1 on failure
 */
int twemoji64_init(EmojiCollection* collection);

#ifdef __cplusplus
}
#endif

#endif /* EMOJI_COLLECTION_H */