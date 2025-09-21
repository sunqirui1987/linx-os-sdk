#include "opus_codec.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// 前向声明
static codec_error_t opus_init_encoder(audio_codec_t* codec, const audio_format_t* format);
static codec_error_t opus_init_decoder(audio_codec_t* codec, const audio_format_t* format);
static codec_error_t opus_codec_encode_impl(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                            uint8_t* output, size_t output_size, size_t* encoded_size);
static codec_error_t opus_codec_decode_impl(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                            int16_t* output, size_t output_size, size_t* decoded_size);
static const char* opus_get_codec_name(const audio_codec_t* codec);
static codec_error_t opus_reset(audio_codec_t* codec);
static int opus_get_input_frame_size(const audio_codec_t* codec);
static int opus_get_max_output_size(const audio_codec_t* codec);
static void opus_destroy(audio_codec_t* codec);

// Opus编解码器虚函数表
static const audio_codec_vtable_t opus_vtable = {
    .init_encoder = opus_init_encoder,
    .init_decoder = opus_init_decoder,
    .encode = opus_codec_encode_impl,
    .decode = opus_codec_decode_impl,
    .get_codec_name = opus_get_codec_name,
    .reset = opus_reset,
    .get_input_frame_size = opus_get_input_frame_size,
    .get_max_output_size = opus_get_max_output_size,
    .destroy = opus_destroy
};

// 创建Opus编解码器实例
audio_codec_t* opus_codec_create(void) {
    audio_codec_t* codec = (audio_codec_t*)malloc(sizeof(audio_codec_t));
    if (!codec) {
        LOG_ERROR("Failed to allocate memory for Opus codec");
        return NULL;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)malloc(sizeof(opus_codec_impl_t));
    if (!impl) {
        LOG_ERROR("Failed to allocate memory for Opus codec implementation");
        free(codec);
        return NULL;
    }

    // 初始化结构体
    memset(codec, 0, sizeof(audio_codec_t));
    memset(impl, 0, sizeof(opus_codec_impl_t));

    codec->vtable = &opus_vtable;
    codec->impl_data = impl;
    codec->encoder_initialized = false;
    codec->decoder_initialized = false;

    // 设置默认参数
    impl->application = OPUS_APPLICATION_VOIP;
    impl->bitrate = 64000;  // 64 kbps
    impl->complexity = 10;
    impl->signal_type = OPUS_AUTO;
    impl->vbr = 1;
    impl->vbr_constraint = 0;
    impl->force_channels = OPUS_AUTO;
    impl->max_bandwidth = OPUS_BANDWIDTH_FULLBAND;
    impl->packet_loss_perc = 0;
    impl->lsb_depth = 24;
    impl->prediction_disabled = 0;
    impl->use_inband_fec = 0;
    impl->use_dtx = 0;

    // 设置默认音频格式
    audio_format_default(&codec->format);

    LOG_INFO("Opus codec created successfully");
    return codec;
}

// 初始化编码器
static codec_error_t opus_init_encoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->impl_data || !format) {
        LOG_ERROR("Invalid parameters for Opus encoder initialization");
        return CODEC_INVALID_PARAMETER;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    int error;

    // 如果编码器已经初始化，先销毁
    if (impl->encoder) {
        opus_encoder_destroy(impl->encoder);
        impl->encoder = NULL;
    }

    // 创建编码器
    impl->encoder = opus_encoder_create(format->sample_rate, format->channels, 
                                       impl->application, &error);
    if (error != OPUS_OK || !impl->encoder) {
        LOG_ERROR("Failed to create Opus encoder: %s", opus_strerror(error));
        return CODEC_INITIALIZATION_FAILED;
    }

    // 设置编码器参数
    opus_encoder_ctl(impl->encoder, OPUS_SET_BITRATE(impl->bitrate));
    opus_encoder_ctl(impl->encoder, OPUS_SET_COMPLEXITY(impl->complexity));
    opus_encoder_ctl(impl->encoder, OPUS_SET_SIGNAL(impl->signal_type));
    opus_encoder_ctl(impl->encoder, OPUS_SET_VBR(impl->vbr));
    opus_encoder_ctl(impl->encoder, OPUS_SET_VBR_CONSTRAINT(impl->vbr_constraint));
    opus_encoder_ctl(impl->encoder, OPUS_SET_FORCE_CHANNELS(impl->force_channels));
    opus_encoder_ctl(impl->encoder, OPUS_SET_MAX_BANDWIDTH(impl->max_bandwidth));
    opus_encoder_ctl(impl->encoder, OPUS_SET_PACKET_LOSS_PERC(impl->packet_loss_perc));
    opus_encoder_ctl(impl->encoder, OPUS_SET_LSB_DEPTH(impl->lsb_depth));
    opus_encoder_ctl(impl->encoder, OPUS_SET_PREDICTION_DISABLED(impl->prediction_disabled));
    opus_encoder_ctl(impl->encoder, OPUS_SET_INBAND_FEC(impl->use_inband_fec));
    opus_encoder_ctl(impl->encoder, OPUS_SET_DTX(impl->use_dtx));

    codec->format = *format;
    codec->encoder_initialized = true;

    LOG_INFO("Opus encoder initialized: %d Hz, %d channels, %d kbps", 
             format->sample_rate, format->channels, impl->bitrate / 1000);
    return CODEC_SUCCESS;
}

// 初始化解码器
static codec_error_t opus_init_decoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->impl_data || !format) {
        LOG_ERROR("Invalid parameters for Opus decoder initialization");
        return CODEC_INVALID_PARAMETER;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    int error;

    // 如果解码器已经初始化，先销毁
    if (impl->decoder) {
        opus_decoder_destroy(impl->decoder);
        impl->decoder = NULL;
    }

    // 创建解码器
    impl->decoder = opus_decoder_create(format->sample_rate, format->channels, &error);
    if (error != OPUS_OK || !impl->decoder) {
        LOG_ERROR("Failed to create Opus decoder: %s", opus_strerror(error));
        return CODEC_INITIALIZATION_FAILED;
    }

    codec->format = *format;
    codec->decoder_initialized = true;

    LOG_INFO("Opus decoder initialized: %d Hz, %d channels", 
             format->sample_rate, format->channels);
    return CODEC_SUCCESS;
}

// 编码音频数据
static codec_error_t opus_codec_encode_impl(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                            uint8_t* output, size_t output_size, size_t* encoded_size) {
    if (!codec || !codec->impl_data || !input || !output || !encoded_size) {
        LOG_ERROR("Invalid parameters for Opus encoding");
        return CODEC_INVALID_PARAMETER;
    }

    if (!codec->encoder_initialized) {
        LOG_ERROR("Opus encoder not initialized");
        return CODEC_INITIALIZATION_FAILED;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    
    // 计算帧大小（样本数）
    int frame_size = codec->format.sample_rate * codec->format.frame_size_ms / 1000;
    
    if ((int)input_size != frame_size * codec->format.channels) {
        LOG_ERROR("Invalid input size for Opus encoding: expected %d, got %zu", 
                  frame_size * codec->format.channels, input_size);
        return CODEC_INVALID_PARAMETER;
    }

    // 编码
    int result = opus_encode(impl->encoder, input, frame_size, output, (opus_int32)output_size);
    if (result < 0) {
        LOG_ERROR("Opus encoding failed: %s", opus_strerror(result));
        return CODEC_ENCODING_FAILED;
    }

    *encoded_size = (size_t)result;
    return CODEC_SUCCESS;
}

// 解码音频数据
static codec_error_t opus_codec_decode_impl(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                            int16_t* output, size_t output_size, size_t* decoded_size) {
    if (!codec || !codec->impl_data || !output || !decoded_size) {
        LOG_ERROR("Invalid parameters for Opus decoding");
        return CODEC_INVALID_PARAMETER;
    }

    if (!codec->decoder_initialized) {
        LOG_ERROR("Opus decoder not initialized");
        return CODEC_INITIALIZATION_FAILED;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    
    // 计算最大帧大小
    int max_frame_size = codec->format.sample_rate * codec->format.frame_size_ms / 1000;
    
    if (output_size < (size_t)(max_frame_size * codec->format.channels)) {
        LOG_ERROR("Output buffer too small for Opus decoding");
        return CODEC_BUFFER_TOO_SMALL;
    }

    // 解码
    int result = opus_decode(impl->decoder, input, (opus_int32)input_size, 
                            output, max_frame_size, 0);
    if (result < 0) {
        LOG_ERROR("Opus decoding failed: %s", opus_strerror(result));
        return CODEC_DECODING_FAILED;
    }

    *decoded_size = (size_t)(result * codec->format.channels);
    return CODEC_SUCCESS;
}

// 获取编码器名称
static const char* opus_get_codec_name(const audio_codec_t* codec) {
    (void)codec; // 避免未使用参数警告
    return "Opus";
}

// 重置编解码器状态
static codec_error_t opus_reset(audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        LOG_ERROR("Invalid parameters for Opus reset");
        return CODEC_INVALID_PARAMETER;
    }

    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_RESET_STATE);
    }
    
    if (impl->decoder && codec->decoder_initialized) {
        opus_decoder_ctl(impl->decoder, OPUS_RESET_STATE);
    }

    LOG_INFO("Opus codec reset");
    return CODEC_SUCCESS;
}

// 获取建议的输入帧大小（样本数）
static int opus_get_input_frame_size(const audio_codec_t* codec) {
    if (!codec) {
        return -1;
    }
    
    // 返回每个声道的样本数
    return codec->format.sample_rate * codec->format.frame_size_ms / 1000;
}

// 获取最大输出缓冲区大小（字节数）
static int opus_get_max_output_size(const audio_codec_t* codec) {
    if (!codec) {
        return -1;
    }
    
    // Opus最大包大小约为4000字节
    return 4000;
}

// 销毁编解码器
static void opus_destroy(audio_codec_t* codec) {
    if (!codec) {
        return;
    }

    if (codec->impl_data) {
        opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
        
        if (impl->encoder) {
            opus_encoder_destroy(impl->encoder);
        }
        
        if (impl->decoder) {
            opus_decoder_destroy(impl->decoder);
        }
        
        free(impl);
    }
    
    free(codec);
    LOG_INFO("Opus codec destroyed");
}

// Opus编解码器参数设置函数
codec_error_t opus_codec_set_bitrate(audio_codec_t* codec, int bitrate) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->bitrate = bitrate;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_BITRATE(bitrate));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_complexity(audio_codec_t* codec, int complexity) {
    if (!codec || !codec->impl_data || complexity < 0 || complexity > 10) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->complexity = complexity;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_COMPLEXITY(complexity));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_signal_type(audio_codec_t* codec, int signal_type) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->signal_type = signal_type;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_SIGNAL(signal_type));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_vbr(audio_codec_t* codec, int vbr) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->vbr = vbr;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_VBR(vbr));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_vbr_constraint(audio_codec_t* codec, int vbr_constraint) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->vbr_constraint = vbr_constraint;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_VBR_CONSTRAINT(vbr_constraint));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_force_channels(audio_codec_t* codec, int force_channels) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->force_channels = force_channels;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_FORCE_CHANNELS(force_channels));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_max_bandwidth(audio_codec_t* codec, int max_bandwidth) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->max_bandwidth = max_bandwidth;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_MAX_BANDWIDTH(max_bandwidth));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_packet_loss_perc(audio_codec_t* codec, int packet_loss_perc) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->packet_loss_perc = packet_loss_perc;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_lsb_depth(audio_codec_t* codec, int lsb_depth) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->lsb_depth = lsb_depth;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_LSB_DEPTH(lsb_depth));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_prediction_disabled(audio_codec_t* codec, int prediction_disabled) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->prediction_disabled = prediction_disabled;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_PREDICTION_DISABLED(prediction_disabled));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_inband_fec(audio_codec_t* codec, int use_inband_fec) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->use_inband_fec = use_inband_fec;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_INBAND_FEC(use_inband_fec));
    }
    
    return CODEC_SUCCESS;
}

codec_error_t opus_codec_set_dtx(audio_codec_t* codec, int use_dtx) {
    if (!codec || !codec->impl_data) {
        return CODEC_INVALID_PARAMETER;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    impl->use_dtx = use_dtx;
    
    if (impl->encoder && codec->encoder_initialized) {
        opus_encoder_ctl(impl->encoder, OPUS_SET_DTX(use_dtx));
    }
    
    return CODEC_SUCCESS;
}

// Opus编解码器参数获取函数
int opus_codec_get_bitrate(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->bitrate;
}

int opus_codec_get_complexity(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->complexity;
}

int opus_codec_get_signal_type(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->signal_type;
}

int opus_codec_get_vbr(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->vbr;
}

int opus_codec_get_vbr_constraint(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->vbr_constraint;
}

int opus_codec_get_force_channels(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->force_channels;
}

int opus_codec_get_max_bandwidth(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->max_bandwidth;
}

int opus_codec_get_packet_loss_perc(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->packet_loss_perc;
}

int opus_codec_get_lsb_depth(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->lsb_depth;
}

int opus_codec_get_prediction_disabled(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->prediction_disabled;
}

int opus_codec_get_inband_fec(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->use_inband_fec;
}

int opus_codec_get_dtx(const audio_codec_t* codec) {
    if (!codec || !codec->impl_data) {
        return -1;
    }
    
    opus_codec_impl_t* impl = (opus_codec_impl_t*)codec->impl_data;
    return impl->use_dtx;
}