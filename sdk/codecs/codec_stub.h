#ifndef _CODEC_STUB_H
#define _CODEC_STUB_H

#include "audio_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

// 通用编解码器 stub 实现数据
typedef struct {
    bool initialized;
    bool encoder_ready;
    bool decoder_ready;
    
    // 模拟编解码器状态
    int frame_count;
    int total_encoded_bytes;
    int total_decoded_samples;
} CodecStubData;

// 创建 stub 编解码器实例
audio_codec_t* codec_stub_create(void);

#ifdef __cplusplus
}
#endif

#endif // _CODEC_STUB_H