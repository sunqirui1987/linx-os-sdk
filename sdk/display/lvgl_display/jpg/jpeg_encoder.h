#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Type definitions */
typedef unsigned char  uint8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned int   uint_t;

/* JPEG encoder enums */
typedef enum {
    JPEG_Y_ONLY = 0,
    JPEG_H1V1 = 1,
    JPEG_H2V1 = 2,
    JPEG_H2V2 = 3
} jpeg_subsampling_t;

/* JPEG encoder parameters */
typedef struct {
    int quality;
    jpeg_subsampling_t subsampling;
} jpeg_params_t;

/* Output stream interface */
typedef struct jpeg_output_stream_t jpeg_output_stream_t;

typedef struct {
    bool (*put_buf)(jpeg_output_stream_t* stream, const void* data, int len);
    uint_t (*get_size)(const jpeg_output_stream_t* stream);
    void (*destroy)(jpeg_output_stream_t* stream);
} jpeg_output_stream_vtable_t;

struct jpeg_output_stream_t {
    const jpeg_output_stream_vtable_t* vtable;
    void* user_data;
};

/* JPEG encoder structure */
typedef struct {
    /* Stream and parameters */
    jpeg_output_stream_t* stream;
    jpeg_params_t params;
    
    /* Image properties */
    uint8_t num_components;
    uint8_t comp_h_samp[3];
    uint8_t comp_v_samp[3];
    int image_x, image_y, image_bpp, image_bpl;
    int image_x_mcu, image_y_mcu;
    int image_bpl_xlt, image_bpl_mcu;
    int mcus_per_row;
    int mcu_x, mcu_y;
    uint8_t* mcu_lines[16];
    uint8_t mcu_y_ofs;
    
    /* Processing arrays */
    int32_t sample_array[64];
    int16_t coefficient_array[64];
    
    /* DC values and output buffer */
    int last_dc_val[3];
    uint8_t out_buf[512];  /* JPGE_OUT_BUF_SIZE */
    uint8_t* out_buf_ptr;
    uint_t out_buf_left;
    uint32_t bit_buffer;
    uint_t bits_in;
    uint8_t pass_num;
    bool all_stream_writes_succeeded;
    
    /* Quantization and Huffman tables (approximately 8KB) */
    int32_t last_quality;
    int32_t quantization_tables[2][64];      /* 512 bytes */
    bool huff_initialized;
    uint_t huff_codes[4][256];               /* 4096 bytes */
    uint8_t huff_code_sizes[4][256];         /* 1024 bytes */
    uint8_t huff_bits[4][17];                /* 68 bytes */
    uint8_t huff_val[4][256];                /* 1024 bytes */
    
    /* Temporary buffers for Huffman table computation */
    uint8_t huff_size_temp[257];             /* 257 bytes */
    uint_t huff_code_temp[257];              /* 1028 bytes */
} jpeg_encoder_t;

/**
 * Create a new JPEG encoder instance
 * @return Pointer to jpeg_encoder_t or NULL on failure
 */
jpeg_encoder_t* jpeg_encoder_create(void);

/**
 * Destroy JPEG encoder instance and free resources
 * @param encoder Pointer to jpeg_encoder_t instance
 */
void jpeg_encoder_destroy(jpeg_encoder_t* encoder);

/**
 * Initialize JPEG encoder
 * @param encoder Pointer to jpeg_encoder_t instance
 * @param stream Output stream for JPEG data
 * @param width Image width
 * @param height Image height
 * @param src_channels Number of source channels (1 or 3)
 * @param params Compression parameters
 * @return true on success, false on failure
 */
bool jpeg_encoder_init(jpeg_encoder_t* encoder, jpeg_output_stream_t* stream, 
                       int width, int height, int src_channels, const jpeg_params_t* params);

/**
 * Process a scanline of image data
 * @param encoder Pointer to jpeg_encoder_t instance
 * @param scanline Pointer to scanline data (NULL to finish encoding)
 * @return true on success, false on failure
 */
bool jpeg_encoder_process_scanline(jpeg_encoder_t* encoder, const void* scanline);

/**
 * Deinitialize JPEG encoder
 * @param encoder Pointer to jpeg_encoder_t instance
 */
void jpeg_encoder_deinit(jpeg_encoder_t* encoder);

/**
 * Create default JPEG parameters
 * @return Default parameters structure
 */
jpeg_params_t jpeg_params_default(void);

/**
 * Validate JPEG parameters
 * @param params Pointer to parameters structure
 * @return true if valid, false otherwise
 */
bool jpeg_params_check(const jpeg_params_t* params);

/* Output stream helper functions */

/**
 * Create memory output stream
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Pointer to output stream or NULL on failure
 */
jpeg_output_stream_t* jpeg_memory_stream_create(void* buffer, size_t buffer_size);

/**
 * Create callback output stream
 * @param callback Callback function for data output
 * @param user_data User data passed to callback
 * @return Pointer to output stream or NULL on failure
 */
typedef size_t (*jpeg_output_callback_t)(void* user_data, size_t index, const void* data, size_t len);
jpeg_output_stream_t* jpeg_callback_stream_create(jpeg_output_callback_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* JPEG_ENCODER_H */