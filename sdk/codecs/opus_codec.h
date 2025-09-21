#ifndef OPUS_CODEC_H
#define OPUS_CODEC_H

#include "audio_codec.h"
#include <opus.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opus编解码器实现数据
typedef struct {
    OpusEncoder* encoder;
    OpusDecoder* decoder;
    int application;        // OPUS_APPLICATION_VOIP, OPUS_APPLICATION_AUDIO, etc.
    int bitrate;           // 比特率
    int complexity;        // 复杂度 (0-10)
    int signal_type;       // OPUS_SIGNAL_VOICE, OPUS_SIGNAL_MUSIC, OPUS_AUTO
    int vbr;              // 可变比特率
    int vbr_constraint;   // 约束可变比特率
    int force_channels;   // 强制声道数
    int max_bandwidth;    // 最大带宽
    int packet_loss_perc; // 预期丢包率
    int lsb_depth;        // LSB深度
    int prediction_disabled; // 预测禁用
    int use_inband_fec;   // 使用带内FEC
    int use_dtx;          // 使用DTX
} opus_codec_impl_t;

// 创建Opus编解码器实例
audio_codec_t* opus_codec_create(void);

// Opus编解码器特定函数
codec_error_t opus_codec_set_bitrate(audio_codec_t* codec, int bitrate);
codec_error_t opus_codec_set_complexity(audio_codec_t* codec, int complexity);
codec_error_t opus_codec_set_signal_type(audio_codec_t* codec, int signal_type);
codec_error_t opus_codec_set_vbr(audio_codec_t* codec, int vbr);
codec_error_t opus_codec_set_vbr_constraint(audio_codec_t* codec, int vbr_constraint);
codec_error_t opus_codec_set_force_channels(audio_codec_t* codec, int force_channels);
codec_error_t opus_codec_set_max_bandwidth(audio_codec_t* codec, int max_bandwidth);
codec_error_t opus_codec_set_packet_loss_perc(audio_codec_t* codec, int packet_loss_perc);
codec_error_t opus_codec_set_lsb_depth(audio_codec_t* codec, int lsb_depth);
codec_error_t opus_codec_set_prediction_disabled(audio_codec_t* codec, int prediction_disabled);
codec_error_t opus_codec_set_inband_fec(audio_codec_t* codec, int use_inband_fec);
codec_error_t opus_codec_set_dtx(audio_codec_t* codec, int use_dtx);

// 获取Opus编解码器参数
int opus_codec_get_bitrate(const audio_codec_t* codec);
int opus_codec_get_complexity(const audio_codec_t* codec);
int opus_codec_get_signal_type(const audio_codec_t* codec);
int opus_codec_get_vbr(const audio_codec_t* codec);
int opus_codec_get_vbr_constraint(const audio_codec_t* codec);
int opus_codec_get_force_channels(const audio_codec_t* codec);
int opus_codec_get_max_bandwidth(const audio_codec_t* codec);
int opus_codec_get_packet_loss_perc(const audio_codec_t* codec);
int opus_codec_get_lsb_depth(const audio_codec_t* codec);
int opus_codec_get_prediction_disabled(const audio_codec_t* codec);
int opus_codec_get_inband_fec(const audio_codec_t* codec);
int opus_codec_get_dtx(const audio_codec_t* codec);

#ifdef __cplusplus
}
#endif

#endif // OPUS_CODEC_H