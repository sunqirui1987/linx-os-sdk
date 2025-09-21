#include "codec_stub.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// 前向声明
static codec_error_t stub_init_encoder(audio_codec_t* codec, const audio_format_t* format);
static codec_error_t stub_init_decoder(audio_codec_t* codec, const audio_format_t* format);
static codec_error_t stub_encode(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                uint8_t* output, size_t output_size, size_t* encoded_size);
static codec_error_t stub_decode(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                int16_t* output, size_t output_size, size_t* decoded_size);
static const char* stub_get_codec_name(const audio_codec_t* codec);
static codec_error_t stub_reset(audio_codec_t* codec);
static int stub_get_input_frame_size(const audio_codec_t* codec);
static int stub_get_max_output_size(const audio_codec_t* codec);
static void stub_destroy(audio_codec_t* codec);

// Stub 编解码器虚函数表
static const audio_codec_vtable_t stub_vtable = {
    .init_encoder = stub_init_encoder,
    .init_decoder = stub_init_decoder,
    .encode = stub_encode,
    .decode = stub_decode,
    .get_codec_name = stub_get_codec_name,
    .reset = stub_reset,
    .get_input_frame_size = stub_get_input_frame_size,
    .get_max_output_size = stub_get_max_output_size,
    .destroy = stub_destroy
};

// 创建 stub 编解码器实例
audio_codec_t* codec_stub_create(void) {
    audio_codec_t* codec = (audio_codec_t*)malloc(sizeof(audio_codec_t));
    if (!codec) {
        LOG_ERROR("Failed to allocate memory for stub codec");
        return NULL;
    }

    CodecStubData* impl = (CodecStubData*)malloc(sizeof(CodecStubData));
    if (!impl) {
        LOG_ERROR("Failed to allocate memory for stub codec implementation");
        free(codec);
        return NULL;
    }

    // 初始化结构体
    memset(codec, 0, sizeof(audio_codec_t));
    memset(impl, 0, sizeof(CodecStubData));

    codec->vtable = &stub_vtable;
    codec->impl_data = impl;
    codec->encoder_initialized = false;
    codec->decoder_initialized = false;

    impl->initialized = false;
    impl->encoder_ready = false;
    impl->decoder_ready = false;
    impl->frame_count = 0;
    impl->total_encoded_bytes = 0;
    impl->total_decoded_samples = 0;

    // 设置默认音频格式
    audio_format_default(&codec->format);

    LOG_INFO("Stub codec created successfully");
    return codec;
}

// 初始化编码器
static codec_error_t stub_init_encoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->impl_data || !format) {
        return CODEC_INVALID_PARAMETER;
    }

    CodecStubData* impl = (CodecStubData*)codec->impl_data;

    LOG_INFO("Initializing stub encoder - sample_rate: %d, channels: %d, bits: %d", 
             format->sample_rate, format->channels, format->bits_per_sample);

    impl->encoder_ready = true;
    codec->encoder_initialized = true;
    codec->format = *format;

    return CODEC_SUCCESS;
}

// 初始化解码器
static codec_error_t stub_init_decoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->impl_data || !format) {
        return CODEC_INVALID_PARAMETER;
    }

    CodecStubData* impl = (CodecStubData*)codec->impl_data;

    LOG_INFO("Initializing stub decoder - sample_rate: %d, channels: %d, bits: %d", 
             format->sample_rate, format->channels, format->bits_per_sample);

    impl->decoder_ready = true;
    codec->decoder_initialized = true;
    codec->format = *format;

    return CODEC_SUCCESS;
}

// 编码音频数据（模拟编码过程）
static codec_error_t stub_encode(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                uint8_t* output, size_t output_size, size_t* encoded_size) {
    if (!codec || !codec->impl_data || !input || !output || !encoded_size) {
        return CODEC_INVALID_PARAMETER;
    }

    CodecStubData* impl = (CodecStubData*)codec->impl_data;
    
    if (!impl->encoder_ready) {
        return CODEC_INITIALIZATION_FAILED;
    }

    // 模拟编码过程 - 简单地将 PCM 数据复制到输出缓冲区
    // 在实际实现中，这里会进行真正的编码
    size_t input_bytes = input_size * sizeof(int16_t);
    
    if (input_bytes > output_size) {
        LOG_WARN("Stub encoder: output buffer too small (%zu > %zu)", input_bytes, output_size);
        return CODEC_BUFFER_TOO_SMALL;
    }

    // 模拟编码延迟和处理
    memcpy(output, input, input_bytes);
    *encoded_size = input_bytes;

    // 更新统计信息
    impl->frame_count++;
    impl->total_encoded_bytes += *encoded_size;

    LOG_DEBUG("Stub encoder: processed %zu samples -> %zu bytes (frame %d)", 
              input_size, *encoded_size, impl->frame_count);

    return CODEC_SUCCESS;
}

// 解码音频数据（模拟解码过程）
static codec_error_t stub_decode(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                int16_t* output, size_t output_size, size_t* decoded_size) {
    if (!codec || !codec->impl_data || !input || !output || !decoded_size) {
        return CODEC_INVALID_PARAMETER;
    }

    CodecStubData* impl = (CodecStubData*)codec->impl_data;
    
    if (!impl->decoder_ready) {
        return CODEC_INITIALIZATION_FAILED;
    }

    // 模拟解码过程 - 简单地将输入数据复制到 PCM 输出缓冲区
    // 在实际实现中，这里会进行真正的解码
    size_t output_samples = input_size / sizeof(int16_t);
    
    if (output_samples > output_size) {
        LOG_WARN("Stub decoder: output buffer too small (%zu > %zu)", output_samples, output_size);
        return CODEC_BUFFER_TOO_SMALL;
    }

    // 模拟解码延迟和处理
    memcpy(output, input, input_size);
    *decoded_size = output_samples;

    // 更新统计信息
    impl->frame_count++;
    impl->total_decoded_samples += *decoded_size;

    LOG_DEBUG("Stub decoder: processed %zu bytes -> %zu samples (frame %d)", 
              input_size, *decoded_size, impl->frame_count);

    return CODEC_SUCCESS;
}

// 获取编解码器名称
static const char* stub_get_codec_name(const audio_codec_t* codec) {
    (void)codec;
    return "Stub Codec (No-op)";
}

// 重置编解码器状态
static codec_error_t stub_reset(audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }

    CodecStubData* impl = (CodecStubData*)codec->impl_data;

    LOG_INFO("Resetting stub codec - processed %d frames, %d encoded bytes, %d decoded samples",
             impl->frame_count, impl->total_encoded_bytes, impl->total_decoded_samples);

    impl->encoder_ready = false;
    impl->decoder_ready = false;
    impl->frame_count = 0;
    impl->total_encoded_bytes = 0;
    impl->total_decoded_samples = 0;
    
    codec->encoder_initialized = false;
    codec->decoder_initialized = false;

    return CODEC_SUCCESS;
}

// 获取建议的输入帧大小
static int stub_get_input_frame_size(const audio_codec_t* codec) {
    if (!codec) {
        return -1;
    }
    
    // 返回基于音频格式的标准帧大小
    return codec->format.sample_rate * codec->format.frame_size_ms / 1000;
}

// 获取最大输出缓冲区大小
static int stub_get_max_output_size(const audio_codec_t* codec) {
    if (!codec) {
        return -1;
    }
    
    // Stub 编解码器输出大小与输入相同（无压缩）
    int frame_size = stub_get_input_frame_size(codec);
    return frame_size * codec->format.channels * sizeof(int16_t);
}

// 销毁编解码器
static void stub_destroy(audio_codec_t* codec) {
    if (!codec) {
        return;
    }

    if (codec->impl_data) {
        CodecStubData* impl = (CodecStubData*)codec->impl_data;
        
        LOG_INFO("Destroying stub codec - final stats: %d frames, %d encoded bytes, %d decoded samples",
                 impl->frame_count, impl->total_encoded_bytes, impl->total_decoded_samples);
        
        free(impl);
    }

    free(codec);
    LOG_INFO("Stub codec destroyed");
}