#include "image_to_jpeg.h"
#include "jpeg_encoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../log/linx_log.h"

#define TAG "image_to_jpeg"

/* Memory allocation function */
static void* _malloc(size_t size) {
    return malloc(size);
}

/* Convert line format function */
static void convert_line_format(uint8_t* dest, const uint8_t* src, pixformat_t format, size_t width) {
    switch (format) {
        case PIXFORMAT_GRAYSCALE:
            memcpy(dest, src, width);
            break;
            
        case PIXFORMAT_RGB888:
            memcpy(dest, src, width * 3);
            break;
            
        case PIXFORMAT_RGB565: {
            const uint16_t* src16 = (const uint16_t*)src;
            for (size_t i = 0; i < width; i++) {
                uint16_t pixel = src16[i];
                dest[i * 3 + 0] = (pixel >> 8) & 0xF8;     /* Red */
                dest[i * 3 + 1] = (pixel >> 3) & 0xFC;     /* Green */
                dest[i * 3 + 2] = (pixel << 3) & 0xF8;     /* Blue */
            }
            break;
        }
        
        case PIXFORMAT_YUV422: {
            const uint8_t* yuv = src;
            for (size_t i = 0; i < width; i += 2) {
                uint8_t y1 = yuv[i * 2];
                uint8_t u = yuv[i * 2 + 1];
                uint8_t y2 = yuv[i * 2 + 2];
                uint8_t v = yuv[i * 2 + 3];
                
                /* Convert YUV to RGB for pixel 1 */
                int c1 = y1 - 16;
                int d1 = u - 128;
                int e1 = v - 128;
                
                dest[i * 3 + 0] = (uint8_t)((298 * c1 + 409 * e1 + 128) >> 8);
                dest[i * 3 + 1] = (uint8_t)((298 * c1 - 100 * d1 - 208 * e1 + 128) >> 8);
                dest[i * 3 + 2] = (uint8_t)((298 * c1 + 516 * d1 + 128) >> 8);
                
                /* Convert YUV to RGB for pixel 2 */
                if (i + 1 < width) {
                    int c2 = y2 - 16;
                    dest[(i + 1) * 3 + 0] = (uint8_t)((298 * c2 + 409 * e1 + 128) >> 8);
                    dest[(i + 1) * 3 + 1] = (uint8_t)((298 * c2 - 100 * d1 - 208 * e1 + 128) >> 8);
                    dest[(i + 1) * 3 + 2] = (uint8_t)((298 * c2 + 516 * d1 + 128) >> 8);
                }
            }
            break;
        }
        
        default:
            LINX_LOGE(TAG, "Unsupported pixel format: %d", format);
            break;
    }
}

/* Callback stream implementation */
typedef struct {
    jpeg_output_stream_t base;
    jpg_out_cb callback;
    void* user_data;
} callback_stream_t;

static bool callback_stream_put_buf(jpeg_output_stream_t* stream, const void* buf, int len) {
    callback_stream_t* cb_stream = (callback_stream_t*)stream;
    size_t result = cb_stream->callback(cb_stream->user_data, 0, buf, len);
    return (result == len);
}

static jpeg_output_stream_vtable_t callback_stream_vtable = {
    .put_buf = callback_stream_put_buf
};

static jpeg_output_stream_t* create_callback_stream(jpg_out_cb callback, void* user_data) {
    callback_stream_t* stream = (callback_stream_t*)malloc(sizeof(callback_stream_t));
    if (!stream) {
        return NULL;
    }
    
    stream->base.vtable = &callback_stream_vtable;
    stream->callback = callback;
    stream->user_data = user_data;
    
    return &stream->base;
}

/* Memory stream implementation */
typedef struct {
    jpeg_output_stream_t base;
    uint8_t* buffer;
    size_t buffer_size;
    size_t current_pos;
    size_t* out_len;
} memory_stream_t;

static bool memory_stream_put_buf(jpeg_output_stream_t* stream, const void* buf, int len) {
    memory_stream_t* mem_stream = (memory_stream_t*)stream;
    
    if (mem_stream->current_pos + len > mem_stream->buffer_size) {
        return false; /* Buffer overflow */
    }
    
    memcpy(mem_stream->buffer + mem_stream->current_pos, buf, len);
    mem_stream->current_pos += len;
    *mem_stream->out_len = mem_stream->current_pos;
    
    return true;
}

static jpeg_output_stream_vtable_t memory_stream_vtable = {
    .put_buf = memory_stream_put_buf
};

static jpeg_output_stream_t* create_memory_stream(uint8_t* buffer, size_t buffer_size, size_t* out_len) {
    memory_stream_t* stream = (memory_stream_t*)malloc(sizeof(memory_stream_t));
    if (!stream) {
        return NULL;
    }
    
    stream->base.vtable = &memory_stream_vtable;
    stream->buffer = buffer;
    stream->buffer_size = buffer_size;
    stream->current_pos = 0;
    stream->out_len = out_len;
    *out_len = 0;
    
    return &stream->base;
}

/* Convert image function */
static bool convert_image(jpeg_output_stream_t* stream, const uint8_t* src, int width, int height, 
                         pixformat_t format, int quality) {
    jpeg_encoder_t* encoder = jpeg_encoder_create();
    if (!encoder) {
        LINX_LOGE(TAG, "Failed to create JPEG encoder");
        return false;
    }
    
    /* Set encoder parameters */
    jpeg_params_t params;
    jpeg_params_default(&params);
    params.quality = quality;
    params.subsampling = (format == PIXFORMAT_GRAYSCALE) ? JPEG_Y_ONLY : JPEG_H2V2;
    
    if (!jpeg_params_check(&params)) {
        LINX_LOGE(TAG, "Invalid JPEG parameters");
        jpeg_encoder_destroy(encoder);
        return false;
    }
    
    /* Initialize encoder */
    int src_channels = (format == PIXFORMAT_GRAYSCALE) ? 1 : 3;
    if (!jpeg_encoder_init(encoder, stream, width, height, src_channels, &params)) {
        LINX_LOGE(TAG, "Failed to initialize JPEG encoder");
        jpeg_encoder_destroy(encoder);
        return false;
    }
    
    /* Allocate line buffer */
    size_t line_size = width * src_channels;
    uint8_t* line_buf = (uint8_t*)_malloc(line_size);
    if (!line_buf) {
        LINX_LOGE(TAG, "Failed to allocate line buffer");
        jpeg_encoder_deinit(encoder);
        jpeg_encoder_destroy(encoder);
        return false;
    }
    
    /* Process each scanline */
    bool success = true;
    for (int y = 0; y < height && success; y++) {
        const uint8_t* src_line = src + y * width * ((format == PIXFORMAT_RGB565) ? 2 : 
                                                     (format == PIXFORMAT_RGB888) ? 3 :
                                                     (format == PIXFORMAT_YUV422) ? 2 : 1);
        
        convert_line_format(line_buf, src_line, format, width);
        
        if (!jpeg_encoder_process_scanline(encoder, line_buf)) {
            LINX_LOGE(TAG, "Failed to process scanline %d", y);
            success = false;
            break;
        }
    }
    
    /* Finalize encoding */
    if (success) {
        jpeg_encoder_deinit(encoder);
        LINX_LOGI(TAG, "JPEG encoding finalized successfully");
    } else {
        jpeg_encoder_deinit(encoder);
    }
    
    free(line_buf);
    jpeg_encoder_destroy(encoder);
    
    if (success) {
        LINX_LOGI(TAG, "JPEG encoding completed successfully");
    }
    
    return success;
}

/* Public API implementations */
bool image_to_jpeg(uint8_t* dest, size_t dest_size, size_t* out_len,
                   int width, int height, pixformat_t format, int quality, const uint8_t* src) {
    if (!dest || !out_len || !src || width <= 0 || height <= 0) {
        LINX_LOGE(TAG, "Invalid parameters");
        return false;
    }
    
    jpeg_output_stream_t* stream = create_memory_stream(dest, dest_size, out_len);
    if (!stream) {
        LINX_LOGE(TAG, "Failed to create memory stream");
        return false;
    }
    
    bool result = convert_image(stream, src, width, height, format, quality);
    
    free(stream);
    return result;
}

bool image_to_jpeg_cb(int width, int height, pixformat_t format, int quality, 
                      const uint8_t* src, jpg_out_cb callback, void* user_data) {
    if (!src || !callback || width <= 0 || height <= 0) {
        LINX_LOGE(TAG, "Invalid parameters");
        return false;
    }
    
    jpeg_output_stream_t* stream = create_callback_stream(callback, user_data);
    if (!stream) {
        LINX_LOGE(TAG, "Failed to create callback stream");
        return false;
    }
    
    bool result = convert_image(stream, src, width, height, format, quality);
    
    free(stream);
    return result;
}