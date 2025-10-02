#include "audio_codec.h"
#include "../log/linx_log.h"

codec_error_t audio_codec_init_encoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->vtable || !codec->vtable->init_encoder) {
        LOG_ERROR("Invalid codec or vtable");
        return CODEC_INVALID_PARAMETER;
    }
    
    if (!format) {
        LOG_ERROR("Invalid format parameter");
        return CODEC_INVALID_PARAMETER;
    }
    
    return codec->vtable->init_encoder(codec, format);
}

codec_error_t audio_codec_init_decoder(audio_codec_t* codec, const audio_format_t* format) {
    if (!codec || !codec->vtable || !codec->vtable->init_decoder) {
        LOG_ERROR("Invalid codec or vtable");
        return CODEC_INVALID_PARAMETER;
    }
    
    if (!format) {
        LOG_ERROR("Invalid format parameter");
        return CODEC_INVALID_PARAMETER;
    }
    
    return codec->vtable->init_decoder(codec, format);
}

codec_error_t audio_codec_encode(audio_codec_t* codec, const int16_t* input, size_t input_size,
                                uint8_t* output, size_t output_size, size_t* encoded_size) {
    if (!codec || !codec->vtable || !codec->vtable->encode) {
        LOG_ERROR("Invalid codec or vtable");
        return CODEC_INVALID_PARAMETER;
    }
    
    if (!input || !output || !encoded_size) {
        LOG_ERROR("Invalid parameters");
        return CODEC_INVALID_PARAMETER;
    }
    
    return codec->vtable->encode(codec, input, input_size, output, output_size, encoded_size);
}

codec_error_t audio_codec_decode(audio_codec_t* codec, const uint8_t* input, size_t input_size,
                                int16_t* output, size_t output_size, size_t* decoded_size) {
    if (!codec || !codec->vtable || !codec->vtable->decode) {
        LOG_ERROR("Invalid codec or vtable");
        return CODEC_INVALID_PARAMETER;
    }
    
    if (!input || !output || !decoded_size) {
        LOG_ERROR("Invalid parameters");
        return CODEC_INVALID_PARAMETER;
    }
    
    return codec->vtable->decode(codec, input, input_size, output, output_size, decoded_size);
}

const char* audio_codec_get_name(const audio_codec_t* codec) {
    if (!codec || !codec->vtable || !codec->vtable->get_codec_name) {
        LOG_ERROR("Invalid codec or vtable");
        return "Unknown";
    }
    
    return codec->vtable->get_codec_name(codec);
}

codec_error_t audio_codec_reset(audio_codec_t* codec) {
    if (!codec || !codec->vtable || !codec->vtable->reset) {
        LOG_ERROR("Invalid codec or vtable");
        return CODEC_INVALID_PARAMETER;
    }
    
    return codec->vtable->reset(codec);
}

int audio_codec_get_input_frame_size(const audio_codec_t* codec) {
    if (!codec || !codec->vtable || !codec->vtable->get_input_frame_size) {
        LOG_ERROR("Invalid codec or vtable");
        return -1;
    }
    
    return codec->vtable->get_input_frame_size(codec);
}

int audio_codec_get_max_output_size(const audio_codec_t* codec) {
    if (!codec || !codec->vtable || !codec->vtable->get_max_output_size) {
        LOG_ERROR("Invalid codec or vtable");
        return -1;
    }
    
    return codec->vtable->get_max_output_size(codec);
}

void audio_codec_destroy(audio_codec_t* codec) {
    if (!codec) {
        return;
    }
    
    if (codec->vtable && codec->vtable->destroy) {
        codec->vtable->destroy(codec);
    }
}