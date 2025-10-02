#ifndef OPUS_CODEC_H
#define OPUS_CODEC_H

#include "audio_codec.h"


#ifdef __cplusplus
extern "C" {
#endif

// 创建Opus编解码器实例
audio_codec_t* opus_codec_create(void);


#ifdef __cplusplus
}
#endif

#endif // OPUS_CODEC_H