#ifndef IMAGE_TO_JPEG_H
#define IMAGE_TO_JPEG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Image pixel format definitions (platform-independent) */
typedef enum {
    PIXFORMAT_RGB565,    /* 2BPP/RGB565 */
    PIXFORMAT_YUV422,    /* 2BPP/YUV422 */
    PIXFORMAT_GRAYSCALE, /* 1BPP/GRAYSCALE */
    PIXFORMAT_JPEG,      /* JPEG/COMPRESSED */
    PIXFORMAT_RGB888,    /* 3BPP/RGB888 */
    PIXFORMAT_RAW,       /* RAW */
    PIXFORMAT_RGB444,    /* 3BP2P/RGB444 */
    PIXFORMAT_RGB555,    /* 3BP2P/RGB555 */
} pixformat_t;

/* JPEG output callback function type */
/* user_data: User-defined parameter, index: Current data index, data: JPEG data chunk, len: Data chunk length */
/* Returns: Number of bytes actually processed */
typedef size_t (*jpg_out_cb)(void* user_data, size_t index, const void* data, size_t len);

/**
 * @brief Convert image format to JPEG efficiently
 * 
 * This function uses an optimized JPEG encoder for encoding, main features:
 * - Saves approximately 8KB of SRAM usage (static variables changed to heap allocation)
 * - Supports multiple image format inputs
 * - High quality JPEG output
 * 
 * @param src       Source image data
 * @param src_len   Source image data length
 * @param width     Image width
 * @param height    Image height  
 * @param format    Image format (PIXFORMAT_RGB565, PIXFORMAT_RGB888, etc.)
 * @param quality   JPEG quality (1-100)
 * @param out       Output JPEG data pointer (caller must free)
 * @param out_len   Output JPEG data length
 * 
 * @return true on success, false on failure
 */
bool image_to_jpeg(uint8_t* src, size_t src_len, uint16_t width, uint16_t height, 
                   pixformat_t format, uint8_t quality, uint8_t** out, size_t* out_len);

/**
 * @brief Convert image format to JPEG (callback version)
 * 
 * Uses callback function to handle JPEG output data, suitable for streaming or chunked processing:
 * - Saves approximately 8KB of SRAM usage (static variables changed to heap allocation)
 * - Supports streaming output, no need to pre-allocate large buffers
 * - Process JPEG data chunk by chunk through callback function
 * 
 * @param src       Source image data
 * @param src_len   Source image data length
 * @param width     Image width
 * @param height    Image height
 * @param format    Image format
 * @param quality   JPEG quality (1-100)
 * @param cb        Output callback function
 * @param user_data User parameter passed to callback function
 * 
 * @return true on success, false on failure
 */
bool image_to_jpeg_cb(uint8_t* src, size_t src_len, uint16_t width, uint16_t height, 
                      pixformat_t format, uint8_t quality, jpg_out_cb cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_TO_JPEG_H */