#include "emoji_collection.h"
#include "lvgl_image.h"
#include "../log/linx_log.h"

#include <string.h>
#include <stdlib.h>

#define TAG "EmojiCollection"

// External emoji declarations for 32px emojis
extern const lv_image_dsc_t emoji_1f636_32; // neutral
extern const lv_image_dsc_t emoji_1f642_32; // happy
extern const lv_image_dsc_t emoji_1f606_32; // laughing
extern const lv_image_dsc_t emoji_1f602_32; // funny
extern const lv_image_dsc_t emoji_1f614_32; // sad
extern const lv_image_dsc_t emoji_1f620_32; // angry
extern const lv_image_dsc_t emoji_1f62d_32; // crying
extern const lv_image_dsc_t emoji_1f60d_32; // loving
extern const lv_image_dsc_t emoji_1f633_32; // embarrassed
extern const lv_image_dsc_t emoji_1f62f_32; // surprised
extern const lv_image_dsc_t emoji_1f631_32; // shocked
extern const lv_image_dsc_t emoji_1f914_32; // thinking
extern const lv_image_dsc_t emoji_1f609_32; // winking
extern const lv_image_dsc_t emoji_1f60e_32; // cool
extern const lv_image_dsc_t emoji_1f60c_32; // relaxed
extern const lv_image_dsc_t emoji_1f924_32; // delicious
extern const lv_image_dsc_t emoji_1f618_32; // kissy
extern const lv_image_dsc_t emoji_1f60f_32; // confident
extern const lv_image_dsc_t emoji_1f634_32; // sleepy
extern const lv_image_dsc_t emoji_1f61c_32; // silly
extern const lv_image_dsc_t emoji_1f644_32; // confused

// External emoji declarations for 64px emojis
extern const lv_image_dsc_t emoji_1f636_64; // neutral
extern const lv_image_dsc_t emoji_1f642_64; // happy
extern const lv_image_dsc_t emoji_1f606_64; // laughing
extern const lv_image_dsc_t emoji_1f602_64; // funny
extern const lv_image_dsc_t emoji_1f614_64; // sad
extern const lv_image_dsc_t emoji_1f620_64; // angry
extern const lv_image_dsc_t emoji_1f62d_64; // crying
extern const lv_image_dsc_t emoji_1f60d_64; // loving
extern const lv_image_dsc_t emoji_1f633_64; // embarrassed
extern const lv_image_dsc_t emoji_1f62f_64; // surprised
extern const lv_image_dsc_t emoji_1f631_64; // shocked
extern const lv_image_dsc_t emoji_1f914_64; // thinking
extern const lv_image_dsc_t emoji_1f609_64; // winking
extern const lv_image_dsc_t emoji_1f60e_64; // cool
extern const lv_image_dsc_t emoji_1f60c_64; // relaxed
extern const lv_image_dsc_t emoji_1f924_64; // delicious
extern const lv_image_dsc_t emoji_1f618_64; // kissy
extern const lv_image_dsc_t emoji_1f60f_64; // confident
extern const lv_image_dsc_t emoji_1f634_64; // sleepy
extern const lv_image_dsc_t emoji_1f61c_64; // silly
extern const lv_image_dsc_t emoji_1f644_64; // confused

int emoji_collection_init(EmojiCollection* collection) {
    if (!collection) {
        LINX_LOGW(TAG, "Invalid collection pointer");
        return -1;
    }
    
    memset(collection, 0, sizeof(EmojiCollection));
    collection->count = 0;
    
    LINX_LOGI(TAG, "Emoji collection initialized successfully");
    return 0;
}

int emoji_collection_add(EmojiCollection* collection, const char* name, LvglImage* image) {
    if (!collection || !name || !image || collection->count >= MAX_EMOJI_COUNT) {
        return -1;
    }
    
    size_t index = collection->count;
    strncpy(collection->entries[index].name, name, MAX_EMOJI_NAME_LEN - 1);
    collection->entries[index].name[MAX_EMOJI_NAME_LEN - 1] = '\0';
    collection->entries[index].image = image;
    collection->entries[index].is_used = true;
    collection->count++;
    
    return 0;
}

const LvglImage* emoji_collection_get(const EmojiCollection* collection, const char* name) {
    if (!collection || !name) {
        return NULL;
    }
    
    for (size_t i = 0; i < collection->count; i++) {
        if (strcmp(collection->entries[i].name, name) == 0) {
            return collection->entries[i].image;
        }
    }
    
    return NULL;
}

void emoji_collection_destroy(EmojiCollection* collection) {
    if (!collection) {
        return;
    }
    
    for (size_t i = 0; i < collection->count; i++) {
        if (collection->entries[i].image) {
            lvgl_image_destroy(collection->entries[i].image);
        }
    }
    
    memset(collection, 0, sizeof(EmojiCollection));
}



int twemoji32_init(EmojiCollection* collection) {
    if (!collection) {
        LINX_LOGW(TAG, "Invalid collection pointer for Twemoji32 init");
        return -1;
    }
    
    if (emoji_collection_init(collection) != 0) {
        LINX_LOGW(TAG, "Failed to initialize emoji collection for Twemoji32");
        return -1;
    }
    
    LINX_LOGI(TAG, "Initializing Twemoji 32px collection...");
    
    // Add all 32px emojis
    struct {
        const char* name;
        const lv_image_dsc_t* image_dsc;
    } emoji_list[] = {
        {"neutral", &emoji_1f636_32},
        {"happy", &emoji_1f642_32},
        {"laughing", &emoji_1f606_32},
        {"funny", &emoji_1f602_32},
        {"sad", &emoji_1f614_32},
        {"angry", &emoji_1f620_32},
        {"crying", &emoji_1f62d_32},
        {"loving", &emoji_1f60d_32},
        {"embarrassed", &emoji_1f633_32},
        {"surprised", &emoji_1f62f_32},
        {"shocked", &emoji_1f631_32},
        {"thinking", &emoji_1f914_32},
        {"winking", &emoji_1f609_32},
        {"cool", &emoji_1f60e_32},
        {"relaxed", &emoji_1f60c_32},
        {"delicious", &emoji_1f924_32},
        {"kissy", &emoji_1f618_32},
        {"confident", &emoji_1f60f_32},
        {"sleepy", &emoji_1f634_32},
        {"silly", &emoji_1f61c_32},
        {"confused", &emoji_1f644_32}
    };
    
    size_t emoji_count = sizeof(emoji_list) / sizeof(emoji_list[0]);
    
    for (size_t i = 0; i < emoji_count; i++) {
        LvglSourceImage* source_image = lvgl_source_image_create(emoji_list[i].image_dsc);
        if (!source_image) {
            emoji_collection_destroy(collection);
            return -1;
        }
        
        if (emoji_collection_add(collection, emoji_list[i].name, (LvglImage*)source_image) != 0) {
            lvgl_image_destroy((LvglImage*)source_image);
            emoji_collection_destroy(collection);
            return -1;
        }
    }
    
    LINX_LOGI(TAG, "Twemoji 32px collection initialized with %zu emojis", emoji_count);
    return 0;
}

int twemoji64_init(EmojiCollection* collection) {
    if (!collection) {
        LINX_LOGW(TAG, "Invalid collection pointer for Twemoji64 init");
        return -1;
    }
    
    if (emoji_collection_init(collection) != 0) {
        LINX_LOGW(TAG, "Failed to initialize emoji collection for Twemoji64");
        return -1;
    }
    
    LINX_LOGI(TAG, "Initializing Twemoji 64px collection...");
    
    // Add all 64px emojis
    struct {
        const char* name;
        const lv_image_dsc_t* image_dsc;
    } emoji_list[] = {
        {"neutral", &emoji_1f636_64},
        {"happy", &emoji_1f642_64},
        {"laughing", &emoji_1f606_64},
        {"funny", &emoji_1f602_64},
        {"sad", &emoji_1f614_64},
        {"angry", &emoji_1f620_64},
        {"crying", &emoji_1f62d_64},
        {"loving", &emoji_1f60d_64},
        {"embarrassed", &emoji_1f633_64},
        {"surprised", &emoji_1f62f_64},
        {"shocked", &emoji_1f631_64},
        {"thinking", &emoji_1f914_64},
        {"winking", &emoji_1f609_64},
        {"cool", &emoji_1f60e_64},
        {"relaxed", &emoji_1f60c_64},
        {"delicious", &emoji_1f924_64},
        {"kissy", &emoji_1f618_64},
        {"confident", &emoji_1f60f_64},
        {"sleepy", &emoji_1f634_64},
        {"silly", &emoji_1f61c_64},
        {"confused", &emoji_1f644_64}
    };
    
    size_t emoji_count = sizeof(emoji_list) / sizeof(emoji_list[0]);
    
    for (size_t i = 0; i < emoji_count; i++) {
        LvglSourceImage* source_image = lvgl_source_image_create(emoji_list[i].image_dsc);
        if (!source_image) {
            emoji_collection_destroy(collection);
            return -1;
        }
        
        LvglImage* image = (LvglImage*)source_image;
        if (emoji_collection_add(collection, emoji_list[i].name, image) != 0) {
            lvgl_image_destroy(image);
            emoji_collection_destroy(collection);
            return -1;
        }
    }
    
    LINX_LOGI(TAG, "Twemoji 64px collection initialized with %zu emojis", emoji_count);
    return 0;
}