#include "jpeg_encoder.h"
#include "../../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define TAG "jpeg_encoder"

/* JPEG constants and tables */
#define JPGE_MAX(a,b) (((a)>(b))?(a):(b))
#define JPGE_MIN(a,b) (((a)<(b))?(a):(b))

/* JPEG markers */
enum { 
    M_SOF0 = 0xC0, M_DHT = 0xC4, M_SOI = 0xD8, M_EOI = 0xD9, 
    M_SOS = 0xDA, M_DQT = 0xDB, M_APP0 = 0xE0 
};

/* Huffman table constants */
enum { 
    DC_LUM_CODES = 12, AC_LUM_CODES = 256, DC_CHROMA_CODES = 12, 
    AC_CHROMA_CODES = 256, MAX_HUFF_SYMBOLS = 257, MAX_HUFF_CODESIZE = 32 
};

/* Static tables */
static const uint8_t s_zag[64] = { 
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 
};

static const int16_t s_std_lum_quant[64] = { 
    16,11,12,14,12,10,16,14,13,14,18,17,16,19,24,40,26,24,22,22,24,49,35,37,29,40,58,51,61,60,57,51,56,55,64,72,92,78,64,68,87,69,55,56,80,109,81,87,95,98,103,104,103,62,77,113,121,112,100,120,92,101,103,99 
};

static const int16_t s_std_croma_quant[64] = { 
    17,18,18,24,21,24,47,26,26,47,99,66,56,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 
};

static const uint8_t s_dc_lum_bits[17] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static const uint8_t s_dc_lum_val[DC_LUM_CODES] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static const uint8_t s_ac_lum_bits[17] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };

static const uint8_t s_ac_lum_val[AC_LUM_CODES] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
    0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
    0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
    0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

static const uint8_t s_dc_chroma_bits[17] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static const uint8_t s_dc_chroma_val[DC_CHROMA_CODES] = { 0,1,2,3,4,5,6,7,8,9,10,11 };
static const uint8_t s_ac_chroma_bits[17] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };

static const uint8_t s_ac_chroma_val[AC_CHROMA_CODES] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
    0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
    0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
    0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

/* Color conversion constants */
const int YR = 19595, YG = 38470, YB = 7471, CB_R = -11059, CB_G = -21709, CB_B = 32768, CR_R = 32768, CR_G = -27439, CR_B = -5329;

/* Memory allocation functions */
static void* jpeg_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        LINX_LOGE(TAG, "Failed to allocate %zu bytes", size);
    }
    return ptr;
}

static void jpeg_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* Utility functions */
static inline uint8_t clamp_uint8(int i) {
    if (i < 0) {
        return 0;
    } else if (i > 255) {
        return 255;
    }
    return (uint8_t)i;
}

/* Color conversion functions */
static void RGB_to_YCC(uint8_t* dst, const uint8_t* src, int num_pixels) {
    for (; num_pixels; dst += 3, src += 3, num_pixels--) {
        const int r = src[0], g = src[1], b = src[2];
        dst[0] = (uint8_t)((r * YR + g * YG + b * YB + 32768) >> 16);
        dst[1] = clamp_uint8(128 + ((r * CB_R + g * CB_G + b * CB_B + 32768) >> 16));
        dst[2] = clamp_uint8(128 + ((r * CR_R + g * CR_G + b * CR_B + 32768) >> 16));
    }
}

static void RGB_to_Y(uint8_t* dst, const uint8_t* src, int num_pixels) {
    for (; num_pixels; dst++, src += 3, num_pixels--) {
        dst[0] = (uint8_t)((src[0] * YR + src[1] * YG + src[2] * YB + 32768) >> 16);
    }
}

static void Y_to_YCC(uint8_t* dst, const uint8_t* src, int num_pixels) {
    for (; num_pixels; dst += 3, src++, num_pixels--) {
        dst[0] = src[0];
        dst[1] = 128;
        dst[2] = 128;
    }
}

/* Forward declarations of internal functions */
static bool jpeg_encoder_jpg_open(jpeg_encoder_t* encoder, int width, int height, int src_channels);
static void jpeg_encoder_flush_output_buffer(jpeg_encoder_t* encoder);
static void jpeg_encoder_put_bits(jpeg_encoder_t* encoder, uint_t bits, uint_t len);
static void jpeg_encoder_emit_byte(jpeg_encoder_t* encoder, uint8_t i);
static void jpeg_encoder_emit_word(jpeg_encoder_t* encoder, uint_t i);
static void jpeg_encoder_emit_marker(jpeg_encoder_t* encoder, int marker);
static void jpeg_encoder_emit_jfif_app0(jpeg_encoder_t* encoder);
static void jpeg_encoder_emit_dqt(jpeg_encoder_t* encoder);
static void jpeg_encoder_emit_sof(jpeg_encoder_t* encoder);
static void jpeg_encoder_emit_dht(jpeg_encoder_t* encoder, uint8_t* bits, uint8_t* val, int index, bool ac_flag);
static void jpeg_encoder_emit_dhts(jpeg_encoder_t* encoder);
static void jpeg_encoder_emit_sos(jpeg_encoder_t* encoder);
static void jpeg_encoder_compute_quant_table(jpeg_encoder_t* encoder, int32_t* dst, const int16_t* src);
static void jpeg_encoder_load_quantized_coefficients(jpeg_encoder_t* encoder, int component_num);
static void jpeg_encoder_load_block_8_8_grey(jpeg_encoder_t* encoder, int x);
static void jpeg_encoder_load_block_8_8(jpeg_encoder_t* encoder, int x, int y, int c);
static void jpeg_encoder_load_block_16_8(jpeg_encoder_t* encoder, int x, int c);
static void jpeg_encoder_load_block_16_8_8(jpeg_encoder_t* encoder, int x, int c);
static void jpeg_encoder_code_coefficients_pass_two(jpeg_encoder_t* encoder, int component_num);
static void jpeg_encoder_code_block(jpeg_encoder_t* encoder, int component_num);
static void jpeg_encoder_process_mcu_row(jpeg_encoder_t* encoder);
static bool jpeg_encoder_process_end_of_image(jpeg_encoder_t* encoder);
static void jpeg_encoder_load_mcu(jpeg_encoder_t* encoder, const void* src);
static void jpeg_encoder_clear(jpeg_encoder_t* encoder);
static void jpeg_encoder_compute_huffman_table(jpeg_encoder_t* encoder, uint_t* codes, uint8_t* code_sizes, uint8_t* bits, uint8_t* val);

/* Public API implementation */
jpeg_encoder_t* jpeg_encoder_create(void) {
    jpeg_encoder_t* encoder = (jpeg_encoder_t*)jpeg_malloc(sizeof(jpeg_encoder_t));
    if (!encoder) {
        return NULL;
    }
    
    memset(encoder, 0, sizeof(jpeg_encoder_t));
    return encoder;
}

void jpeg_encoder_destroy(jpeg_encoder_t* encoder) {
    if (!encoder) {
        return;
    }
    
    jpeg_encoder_deinit(encoder);
    jpeg_free(encoder);
}

jpeg_params_t jpeg_params_default(void) {
    jpeg_params_t params;
    params.quality = 85;
    params.subsampling = JPEG_H2V2;
    return params;
}

bool jpeg_params_check(const jpeg_params_t* params) {
    if (!params) {
        return false;
    }
    if ((params->quality < 1) || (params->quality > 100)) {
        return false;
    }
    if ((uint_t)params->subsampling > (uint_t)JPEG_H2V2) {
        return false;
    }
    return true;
}

/* DCT implementation */
static void DCT2D(int32_t* p) {
    int32_t c1 = 1004, c2 = 1136, c3 = 1213, c4 = 1334, c5 = 1453, c6 = 1607, c7 = 1731;
    int i;
    int32_t* q = p;
    
    for (i = 8; i > 0; i--, q += 8) {
        int32_t s0 = q[0] + q[7], d0 = q[0] - q[7];
        int32_t s1 = q[1] + q[6], d1 = q[1] - q[6];
        int32_t s2 = q[2] + q[5], d2 = q[2] - q[5];
        int32_t s3 = q[3] + q[4], d3 = q[3] - q[4];
        int32_t a0 = s0 + s3, a1 = s1 + s2, a2 = s0 - s3, a3 = s1 - s2;
        int32_t t0 = (a0 + a1) * c4 >> 10;
        int32_t t1 = (a0 - a1) * c4 >> 10;
        int32_t t2 = a2 * c6 + a3 * c2 >> 10;
        int32_t t3 = a2 * c2 - a3 * c6 >> 10;
        q[0] = t0; q[1] = t1; q[2] = t2; q[3] = t3;
        
        int32_t u0 = d0 + d3, u1 = d1 + d2, u2 = d0 - d3, u3 = d1 - d2;
        int32_t v0 = u0 * c7 + u1 * c1 >> 10;
        int32_t v1 = u0 * c1 - u1 * c7 >> 10;
        int32_t v2 = u2 * c3 + u3 * c5 >> 10;
        int32_t v3 = u2 * c5 - u3 * c3 >> 10;
        q[4] = v0; q[5] = v1; q[6] = v2; q[7] = v3;
    }
    
    for (i = 8; i > 0; i--, p++) {
        int32_t s0 = p[0] + p[56], d0 = p[0] - p[56];
        int32_t s1 = p[8] + p[48], d1 = p[8] - p[48];
        int32_t s2 = p[16] + p[40], d2 = p[16] - p[40];
        int32_t s3 = p[24] + p[32], d3 = p[24] - p[32];
        int32_t a0 = s0 + s3, a1 = s1 + s2, a2 = s0 - s3, a3 = s1 - s2;
        int32_t t0 = (a0 + a1) * c4 >> 10;
        int32_t t1 = (a0 - a1) * c4 >> 10;
        int32_t t2 = a2 * c6 + a3 * c2 >> 10;
        int32_t t3 = a2 * c2 - a3 * c6 >> 10;
        p[0] = t0; p[8] = t1; p[16] = t2; p[24] = t3;
        
        int32_t u0 = d0 + d3, u1 = d1 + d2, u2 = d0 - d3, u3 = d1 - d2;
        int32_t v0 = u0 * c7 + u1 * c1 >> 10;
        int32_t v1 = u0 * c1 - u1 * c7 >> 10;
        int32_t v2 = u2 * c3 + u3 * c5 >> 10;
        int32_t v3 = u2 * c5 - u3 * c3 >> 10;
        p[32] = v0; p[40] = v1; p[48] = v2; p[56] = v3;
    }
}

/* Main encoder functions */
bool jpeg_encoder_init(jpeg_encoder_t* encoder, jpeg_output_stream_t* stream, 
                       int width, int height, int src_channels, const jpeg_params_t* params) {
    if (!encoder || !stream || !params) {
        LINX_LOGE(TAG, "Invalid parameters");
        return false;
    }
    
    if (!jpeg_params_check(params)) {
        LINX_LOGE(TAG, "Invalid JPEG parameters");
        return false;
    }
    
    if ((width < 1) || (height < 1) || (width > 65535) || (height > 65535)) {
        LINX_LOGE(TAG, "Invalid image dimensions: %dx%d", width, height);
        return false;
    }
    
    if ((src_channels != 1) && (src_channels != 3)) {
        LINX_LOGE(TAG, "Invalid number of channels: %d", src_channels);
        return false;
    }
    
    encoder->stream = stream;
    encoder->params = *params;
    
    if (!jpeg_encoder_jpg_open(encoder, width, height, src_channels)) {
        LINX_LOGE(TAG, "Failed to initialize JPEG encoder");
        return false;
    }
    
    return true;
}

bool jpeg_encoder_process_scanline(jpeg_encoder_t* encoder, const void* scanline) {
    if (!encoder) {
        return false;
    }
    
    if (scanline) {
        jpeg_encoder_load_mcu(encoder, scanline);
        encoder->mcu_y_ofs++;
        if (encoder->mcu_y_ofs >= encoder->comp_v_samp[0]) {
            jpeg_encoder_process_mcu_row(encoder);
            encoder->mcu_y_ofs = 0;
            encoder->mcu_y++;
        }
    } else {
        return jpeg_encoder_process_end_of_image(encoder);
    }
    
    return encoder->all_stream_writes_succeeded;
}

void jpeg_encoder_deinit(jpeg_encoder_t* encoder) {
    if (!encoder) {
        return;
    }
    
    jpeg_encoder_clear(encoder);
}

/* Internal function implementations */
static void jpeg_encoder_clear(jpeg_encoder_t* encoder) {
    if (!encoder) {
        return;
    }
    
    encoder->stream = NULL;
    encoder->all_stream_writes_succeeded = true;
    encoder->huff_initialized = false;
    encoder->last_quality = -1;
    
    /* Clear MCU line pointers */
    for (int i = 0; i < 16; i++) {
        if (encoder->mcu_lines[i]) {
            jpeg_free(encoder->mcu_lines[i]);
            encoder->mcu_lines[i] = NULL;
        }
    }
}

static void jpeg_encoder_flush_output_buffer(jpeg_encoder_t* encoder) {
    if (encoder->out_buf_left != 512) {
        if (!encoder->stream->vtable->put_buf(encoder->stream, encoder->out_buf, 512 - encoder->out_buf_left)) {
            encoder->all_stream_writes_succeeded = false;
        }
        encoder->out_buf_ptr = encoder->out_buf;
        encoder->out_buf_left = 512;
    }
}

static void jpeg_encoder_put_bits(jpeg_encoder_t* encoder, uint_t bits, uint_t len) {
    encoder->bit_buffer |= ((uint32_t)bits << (24 - (encoder->bits_in + len)));
    encoder->bits_in += len;
    while (encoder->bits_in >= 8) {
        uint8_t c = (uint8_t)((encoder->bit_buffer >> 16) & 0xFF);
        jpeg_encoder_emit_byte(encoder, c);
        if (c == 0xFF) {
            jpeg_encoder_emit_byte(encoder, 0);
        }
        encoder->bit_buffer <<= 8;
        encoder->bits_in -= 8;
    }
}

static void jpeg_encoder_emit_byte(jpeg_encoder_t* encoder, uint8_t i) {
    if (encoder->out_buf_left == 0) {
        jpeg_encoder_flush_output_buffer(encoder);
    }
    *encoder->out_buf_ptr++ = i;
    encoder->out_buf_left--;
}

static void jpeg_encoder_emit_word(jpeg_encoder_t* encoder, uint_t i) {
    jpeg_encoder_emit_byte(encoder, (uint8_t)(i >> 8));
    jpeg_encoder_emit_byte(encoder, (uint8_t)(i & 0xFF));
}

static void jpeg_encoder_emit_marker(jpeg_encoder_t* encoder, int marker) {
    jpeg_encoder_emit_byte(encoder, 0xFF);
    jpeg_encoder_emit_byte(encoder, (uint8_t)marker);
}

static void jpeg_encoder_emit_jfif_app0(jpeg_encoder_t* encoder) {
    jpeg_encoder_emit_marker(encoder, M_APP0);
    jpeg_encoder_emit_word(encoder, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1);
    jpeg_encoder_emit_byte(encoder, 0x4A); jpeg_encoder_emit_byte(encoder, 0x46); 
    jpeg_encoder_emit_byte(encoder, 0x49); jpeg_encoder_emit_byte(encoder, 0x46); /* "JFIF" */
    jpeg_encoder_emit_byte(encoder, 0);
    jpeg_encoder_emit_byte(encoder, 1); /* Major version */
    jpeg_encoder_emit_byte(encoder, 1); /* Minor version */
    jpeg_encoder_emit_byte(encoder, 0); /* Pixel units */
    jpeg_encoder_emit_word(encoder, 1); /* X density */
    jpeg_encoder_emit_word(encoder, 1); /* Y density */
    jpeg_encoder_emit_byte(encoder, 0); /* Thumbnail width */
    jpeg_encoder_emit_byte(encoder, 0); /* Thumbnail height */
}

static void jpeg_encoder_compute_quant_table(jpeg_encoder_t* encoder, int32_t* dst, const int16_t* src) {
    int32_t q;
    if (encoder->params.quality < 50) {
        q = 5000 / encoder->params.quality;
    } else {
        q = 200 - encoder->params.quality * 2;
    }
    
    for (int i = 0; i < 64; i++) {
        int32_t j = *src++; j = (j * q + 50L) / 100L;
        *dst++ = JPGE_MIN(JPGE_MAX(j, 1L), 255L);
    }
}

static void jpeg_encoder_emit_dqt(jpeg_encoder_t* encoder) {
    for (int i = 0; i < ((encoder->num_components == 3) ? 2 : 1); i++) {
        jpeg_encoder_emit_marker(encoder, M_DQT);
        jpeg_encoder_emit_word(encoder, 64 + 1 + 2);
        jpeg_encoder_emit_byte(encoder, (uint8_t)i);
        for (int j = 0; j < 64; j++) {
            jpeg_encoder_emit_byte(encoder, (uint8_t)encoder->quantization_tables[i][s_zag[j]]);
        }
    }
}

static void jpeg_encoder_emit_sof(jpeg_encoder_t* encoder) {
    jpeg_encoder_emit_marker(encoder, M_SOF0);
    jpeg_encoder_emit_word(encoder, 3 * encoder->num_components + 2 + 5 + 1);
    jpeg_encoder_emit_byte(encoder, 8); /* precision */
    jpeg_encoder_emit_word(encoder, encoder->image_y);
    jpeg_encoder_emit_word(encoder, encoder->image_x);
    jpeg_encoder_emit_byte(encoder, encoder->num_components);
    for (int i = 0; i < encoder->num_components; i++) {
        jpeg_encoder_emit_byte(encoder, (uint8_t)(i + 1)); /* component ID */
        jpeg_encoder_emit_byte(encoder, (encoder->comp_h_samp[i] << 4) + encoder->comp_v_samp[i]); /* h and v sampling */
        jpeg_encoder_emit_byte(encoder, i > 0); /* quant. table num */
    }
}

static bool jpeg_encoder_jpg_open(jpeg_encoder_t* encoder, int width, int height, int src_channels) {
    encoder->image_x = width;
    encoder->image_y = height;
    encoder->image_bpp = src_channels;
    encoder->image_bpl = width * src_channels;
    encoder->image_bpl_xlt = width * encoder->num_components;
    encoder->image_bpl_mcu = (encoder->image_bpl_xlt + 15) & ~15;
    
    encoder->num_components = (src_channels == 3) ? 3 : 1;
    
    if (encoder->num_components == 1) {
        encoder->comp_h_samp[0] = 1; encoder->comp_v_samp[0] = 1;
    } else {
        switch (encoder->params.subsampling) {
            case JPEG_Y_ONLY:
                encoder->comp_h_samp[0] = 1; encoder->comp_v_samp[0] = 1;
                break;
            case JPEG_H1V1:
                encoder->comp_h_samp[0] = 1; encoder->comp_v_samp[0] = 1;
                encoder->comp_h_samp[1] = 1; encoder->comp_v_samp[1] = 1;
                encoder->comp_h_samp[2] = 1; encoder->comp_v_samp[2] = 1;
                break;
            case JPEG_H2V1:
                encoder->comp_h_samp[0] = 2; encoder->comp_v_samp[0] = 1;
                encoder->comp_h_samp[1] = 1; encoder->comp_v_samp[1] = 1;
                encoder->comp_h_samp[2] = 1; encoder->comp_v_samp[2] = 1;
                break;
            default:
            case JPEG_H2V2:
                encoder->comp_h_samp[0] = 2; encoder->comp_v_samp[0] = 2;
                encoder->comp_h_samp[1] = 1; encoder->comp_v_samp[1] = 1;
                encoder->comp_h_samp[2] = 1; encoder->comp_v_samp[2] = 1;
                break;
        }
    }
    
    encoder->image_x_mcu = (encoder->image_x + 8 * encoder->comp_h_samp[0] - 1) & (~(8 * encoder->comp_h_samp[0] - 1));
    encoder->image_y_mcu = (encoder->image_y + 8 * encoder->comp_v_samp[0] - 1) & (~(8 * encoder->comp_v_samp[0] - 1));
    encoder->mcus_per_row = encoder->image_x_mcu >> 3;
    
    /* Allocate MCU line buffers */
    for (int i = 0; i < encoder->comp_v_samp[0]; i++) {
        encoder->mcu_lines[i] = (uint8_t*)jpeg_malloc(encoder->image_bpl_mcu);
        if (!encoder->mcu_lines[i]) {
            return false;
        }
    }
    
    /* Initialize quantization tables */
    if (encoder->last_quality != encoder->params.quality) {
        jpeg_encoder_compute_quant_table(encoder, encoder->quantization_tables[0], s_std_lum_quant);
        jpeg_encoder_compute_quant_table(encoder, encoder->quantization_tables[1], s_std_croma_quant);
        encoder->last_quality = encoder->params.quality;
    }
    
    /* Initialize output buffer */
    encoder->out_buf_ptr = encoder->out_buf;
    encoder->out_buf_left = 512;
    encoder->bit_buffer = 0;
    encoder->bits_in = 0;
    encoder->all_stream_writes_succeeded = true;
    
    /* Emit JPEG headers */
    jpeg_encoder_emit_marker(encoder, M_SOI);
    jpeg_encoder_emit_jfif_app0(encoder);
    jpeg_encoder_emit_dqt(encoder);
    jpeg_encoder_emit_sof(encoder);
    
    return true;
}

/* Simplified implementations for remaining functions */
static void jpeg_encoder_load_mcu(jpeg_encoder_t* encoder, const void* src) {
    const uint8_t* p = (const uint8_t*)src;
    if (encoder->num_components == 1) {
        memcpy(encoder->mcu_lines[encoder->mcu_y_ofs], p, encoder->image_bpl);
    } else {
        /* Convert RGB to YCbCr */
        RGB_to_YCC(encoder->mcu_lines[encoder->mcu_y_ofs], p, encoder->image_x);
    }
}

static void jpeg_encoder_process_mcu_row(jpeg_encoder_t* encoder) {
    /* Simplified MCU processing - in a full implementation, this would perform DCT, quantization, and Huffman encoding */
    encoder->mcu_x = 0;
}

static bool jpeg_encoder_process_end_of_image(jpeg_encoder_t* encoder) {
    /* Flush any remaining bits */
    if (encoder->bits_in) {
        jpeg_encoder_put_bits(encoder, 0x7F, 7);
    }
    
    /* Flush output buffer */
    jpeg_encoder_flush_output_buffer(encoder);
    
    /* Emit EOI marker */
    jpeg_encoder_emit_marker(encoder, M_EOI);
    jpeg_encoder_flush_output_buffer(encoder);
    
    return encoder->all_stream_writes_succeeded;
}