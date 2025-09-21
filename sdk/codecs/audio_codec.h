#ifndef _AUDIO_CODEC_H
#define _AUDIO_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 音频编解码器错误码
typedef enum {
    CODEC_SUCCESS = 0,
    CODEC_INVALID_PARAMETER,
    CODEC_INITIALIZATION_FAILED,
    CODEC_ENCODING_FAILED,
    CODEC_DECODING_FAILED,
    CODEC_BUFFER_TOO_SMALL,
    CODEC_UNSUPPORTED_FORMAT,
    CODEC_MEMORY_ALLOCATION_FAILED
} codec_error_t;

// 音频格式配置
typedef struct {
    int sample_rate;        // 采样率 (Hz)
    int channels;           // 声道数
    int bits_per_sample;    // 每样本位数
    int frame_size_ms;      // 帧大小 (毫秒)
} audio_format_t;

// 前向声明
typedef struct audio_codec audio_codec_t;

// 音频编解码器虚函数表
typedef struct {
    // 初始化编码器
    codec_error_t (*init_encoder)(audio_codec_t* codec, const audio_format_t* format);
    
    // 初始化解码器
    codec_error_t (*init_decoder)(audio_codec_t* codec, const audio_format_t* format);
    
    // 编码音频数据
    // input: 输入的PCM音频数据 (16位有符号整数)
    // input_size: 输入数据大小（样本数）
    // output: 编码后的数据缓冲区
    // output_size: 输出缓冲区大小（字节数）
    // encoded_size: 实际编码的字节数
    codec_error_t (*encode)(audio_codec_t* codec, const int16_t* input, size_t input_size,
                           uint8_t* output, size_t output_size, size_t* encoded_size);
    
    // 解码音频数据
    // input: 编码的音频数据
    // input_size: 输入数据大小（字节数）
    // output: 解码后的PCM音频数据缓冲区 (16位有符号整数)
    // output_size: 输出缓冲区大小（样本数）
    // decoded_size: 实际解码的样本数
    codec_error_t (*decode)(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                           int16_t* output, size_t output_size, size_t* decoded_size);
    
    // 获取编码器名称
    const char* (*get_codec_name)(const audio_codec_t* codec);
    
    // 重置编解码器状态
    codec_error_t (*reset)(audio_codec_t* codec);
    
    // 获取建议的输入帧大小（样本数）
    int (*get_input_frame_size)(const audio_codec_t* codec);
    
    // 获取最大输出缓冲区大小（字节数）
    int (*get_max_output_size)(const audio_codec_t* codec);
    
    // 销毁编解码器
    void (*destroy)(audio_codec_t* codec);
} audio_codec_vtable_t;

// 音频编解码器基础结构
struct audio_codec {
    const audio_codec_vtable_t* vtable;
    void* impl_data;  // 实现特定的数据
    
    audio_format_t format;
    bool encoder_initialized;
    bool decoder_initialized;
};



// 基类接口函数
codec_error_t audio_codec_init_encoder(audio_codec_t* codec, const audio_format_t* format);
codec_error_t audio_codec_init_decoder(audio_codec_t* codec, const audio_format_t* format);
codec_error_t audio_codec_encode(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                uint8_t* output, size_t output_size, size_t* encoded_size);
codec_error_t audio_codec_decode(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                int16_t* output, size_t output_size, size_t* decoded_size);
const char* audio_codec_get_name(const audio_codec_t* codec);
codec_error_t audio_codec_reset(audio_codec_t* codec);
int audio_codec_get_input_frame_size(const audio_codec_t* codec);
int audio_codec_get_max_output_size(const audio_codec_t* codec);
void audio_codec_destroy(audio_codec_t* codec);

// 便利函数
static inline void audio_format_init(audio_format_t* format, int sample_rate, 
                                    int channels, int bits_per_sample, int frame_size_ms) {
    format->sample_rate = sample_rate;
    format->channels = channels;
    format->bits_per_sample = bits_per_sample;
    format->frame_size_ms = frame_size_ms;
}

static inline void audio_format_default(audio_format_t* format) {
    audio_format_init(format, 16000, 1, 16, 20);
}

#ifdef __cplusplus
}
#endif

#endif // _AUDIO_CODEC_H
