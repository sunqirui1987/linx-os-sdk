#include "audio_codec.h"
#include "opus_codec.h"
#include "codec_stub.h"
#include "../log/linx_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_SIZE_MS 20
#define FRAME_SIZE (SAMPLE_RATE * FRAME_SIZE_MS / 1000)  // 320 samples
#define MAX_PACKET_SIZE 4000
#define TEST_DURATION_SEC 2
#define NUM_FRAMES (TEST_DURATION_SEC * 1000 / FRAME_SIZE_MS)  // 100 frames

// 生成测试音频数据（正弦波）
void generate_test_audio(int16_t* buffer, size_t samples, double frequency) {
    for (size_t i = 0; i < samples; i++) {
        double t = (double)i / SAMPLE_RATE;
        double sample = sin(2.0 * M_PI * frequency * t) * 16000.0;
        buffer[i] = (int16_t)sample;
    }
}

// 计算音频数据的RMS值
double calculate_rms(const int16_t* buffer, size_t samples) {
    double sum = 0.0;
    for (size_t i = 0; i < samples; i++) {
        double sample = (double)buffer[i];
        sum += sample * sample;
    }
    return sqrt(sum / samples);
}

// 测试编解码器创建
int test_codec_creation(void) {
    printf("Testing codec creation...\n");
    
    // 测试创建Opus编解码器
    audio_codec_t* opus_codec = opus_codec_create();
    assert(opus_codec != NULL);
    
    const char* opus_name = audio_codec_get_name(opus_codec);
    printf("Created Opus codec: %s\n", opus_name);
    assert(strcmp(opus_name, "Opus") == 0);
    
    // 测试创建Stub编解码器
    audio_codec_t* stub_codec = codec_stub_create();
    assert(stub_codec != NULL);
    
    const char* stub_name = audio_codec_get_name(stub_codec);
    printf("Created Stub codec: %s\n", stub_name);
    assert(strcmp(stub_name, "Stub Codec (No-op)") == 0);
    
    // 销毁编解码器
    audio_codec_destroy(opus_codec);
    audio_codec_destroy(stub_codec);
    
    printf("Codec creation test passed!\n\n");
    return 0;
}

// 测试Opus编解码器基本功能
int test_opus_codec_basic(void) {
    printf("Testing Opus codec basic functionality...\n");
    
    // 创建编解码器
    audio_codec_t* codec = opus_codec_create();
    assert(codec != NULL);
    
    // 设置音频格式
    audio_format_t format;
    audio_format_init(&format, SAMPLE_RATE, CHANNELS, 16, FRAME_SIZE_MS);
    
    // 初始化编码器
    codec_error_t result = audio_codec_init_encoder(codec, &format);
    assert(result == CODEC_SUCCESS);
    assert(codec->encoder_initialized == true);
    
    // 初始化解码器
    result = audio_codec_init_decoder(codec, &format);
    assert(result == CODEC_SUCCESS);
    assert(codec->decoder_initialized == true);
    
    // 测试获取参数
    int frame_size = audio_codec_get_input_frame_size(codec);
    printf("Input frame size: %d samples\n", frame_size);
    assert(frame_size == FRAME_SIZE);
    
    int max_output_size = audio_codec_get_max_output_size(codec);
    printf("Max output size: %d bytes\n", max_output_size);
    assert(max_output_size > 0);
    
    // 销毁编解码器
    audio_codec_destroy(codec);
    
    printf("Opus codec basic test passed!\n\n");
    return 0;
}

// 测试Opus编解码器编解码功能
int test_opus_codec_encode_decode(void) {
    printf("Testing Opus codec encode/decode functionality...\n");
    
    // 创建编解码器
    audio_codec_t* codec = opus_codec_create();
    assert(codec != NULL);
    
    // 设置音频格式
    audio_format_t format;
    audio_format_init(&format, SAMPLE_RATE, CHANNELS, 16, FRAME_SIZE_MS);
    
    // 初始化编码器和解码器
    codec_error_t result = audio_codec_init_encoder(codec, &format);
    assert(result == CODEC_SUCCESS);
    
    result = audio_codec_init_decoder(codec, &format);
    assert(result == CODEC_SUCCESS);
    
    // 分配缓冲区
    int16_t* input_buffer = (int16_t*)malloc(FRAME_SIZE * CHANNELS * sizeof(int16_t));
    uint8_t* encoded_buffer = (uint8_t*)malloc(MAX_PACKET_SIZE);
    int16_t* decoded_buffer = (int16_t*)malloc(FRAME_SIZE * CHANNELS * sizeof(int16_t));
    
    assert(input_buffer != NULL);
    assert(encoded_buffer != NULL);
    assert(decoded_buffer != NULL);
    
    double total_compression_ratio = 0.0;
    double total_quality_loss = 0.0;
    
    printf("Processing %d frames...\n", NUM_FRAMES);
    
    for (int frame = 0; frame < NUM_FRAMES; frame++) {
        // 生成测试音频（不同频率的正弦波）
        double frequency = 440.0 + (frame % 10) * 110.0;  // 440Hz到1320Hz
        generate_test_audio(input_buffer, FRAME_SIZE * CHANNELS, frequency);
        
        // 编码
        size_t encoded_size;
        result = audio_codec_encode(codec, input_buffer, FRAME_SIZE * CHANNELS,
                                   encoded_buffer, MAX_PACKET_SIZE, &encoded_size);
        assert(result == CODEC_SUCCESS);
        assert(encoded_size > 0);
        
        // 解码
        size_t decoded_size;
        result = audio_codec_decode(codec, encoded_buffer, encoded_size,
                                   decoded_buffer, FRAME_SIZE * CHANNELS, &decoded_size);
        assert(result == CODEC_SUCCESS);
        assert(decoded_size == FRAME_SIZE * CHANNELS);
        
        // 计算压缩比
        size_t original_size = FRAME_SIZE * CHANNELS * sizeof(int16_t);
        double compression_ratio = (double)original_size / encoded_size;
        total_compression_ratio += compression_ratio;
        
        // 计算质量损失（RMS差异）
        double original_rms = calculate_rms(input_buffer, FRAME_SIZE * CHANNELS);
        double decoded_rms = calculate_rms(decoded_buffer, FRAME_SIZE * CHANNELS);
        double quality_loss = fabs(original_rms - decoded_rms) / original_rms * 100.0;
        total_quality_loss += quality_loss;
        
        if (frame % 20 == 0) {
            printf("Frame %d: encoded %zu bytes, compression %.2fx, quality loss %.2f%%\n",
                   frame, encoded_size, compression_ratio, quality_loss);
        }
    }
    
    double avg_compression_ratio = total_compression_ratio / NUM_FRAMES;
    double avg_quality_loss = total_quality_loss / NUM_FRAMES;
    
    printf("Average compression ratio: %.2fx\n", avg_compression_ratio);
    printf("Average quality loss: %.2f%%\n", avg_quality_loss);
    
    // 验证压缩效果
    assert(avg_compression_ratio > 2.0);  // 至少2倍压缩
    assert(avg_quality_loss < 20.0);      // 质量损失小于20%
    
    // 清理
    free(input_buffer);
    free(encoded_buffer);
    free(decoded_buffer);
    audio_codec_destroy(codec);
    
    printf("Opus codec encode/decode test passed!\n\n");
    return 0;
}

// 测试Opus编解码器参数设置
int test_opus_codec_parameters(void) {
    printf("Testing Opus codec parameter settings...\n");
    
    // 创建编解码器
    audio_codec_t* codec = opus_codec_create();
    assert(codec != NULL);
    
    // 测试比特率设置
    codec_error_t result = opus_codec_set_bitrate(codec, 128000);
    assert(result == CODEC_SUCCESS);
    assert(opus_codec_get_bitrate(codec) == 128000);
    
    // 测试复杂度设置
    result = opus_codec_set_complexity(codec, 5);
    assert(result == CODEC_SUCCESS);
    assert(opus_codec_get_complexity(codec) == 5);
    
    // 测试VBR设置
    result = opus_codec_set_vbr(codec, 0);
    assert(result == CODEC_SUCCESS);
    assert(opus_codec_get_vbr(codec) == 0);
    
    // 测试FEC设置
    result = opus_codec_set_inband_fec(codec, 1);
    assert(result == CODEC_SUCCESS);
    assert(opus_codec_get_inband_fec(codec) == 1);
    
    // 测试DTX设置
    result = opus_codec_set_dtx(codec, 1);
    assert(result == CODEC_SUCCESS);
    assert(opus_codec_get_dtx(codec) == 1);
    
    printf("Bitrate: %d bps\n", opus_codec_get_bitrate(codec));
    printf("Complexity: %d\n", opus_codec_get_complexity(codec));
    printf("VBR: %d\n", opus_codec_get_vbr(codec));
    printf("Inband FEC: %d\n", opus_codec_get_inband_fec(codec));
    printf("DTX: %d\n", opus_codec_get_dtx(codec));
    
    // 销毁编解码器
    audio_codec_destroy(codec);
    
    printf("Opus codec parameter test passed!\n\n");
    return 0;
}

// 测试错误处理
int test_error_handling(void) {
    printf("Testing error handling...\n");
    
    // 测试NULL参数
    audio_codec_t* codec = opus_codec_create();
    assert(codec != NULL);
    
    // 测试未初始化编码器的编码
    int16_t input[FRAME_SIZE];
    uint8_t output[MAX_PACKET_SIZE];
    size_t encoded_size;
    
    codec_error_t result = audio_codec_encode(codec, input, FRAME_SIZE,
                                             output, MAX_PACKET_SIZE, &encoded_size);
    assert(result == CODEC_INITIALIZATION_FAILED);
    
    // 测试无效参数
    result = audio_codec_encode(NULL, input, FRAME_SIZE, output, MAX_PACKET_SIZE, &encoded_size);
    assert(result == CODEC_INVALID_PARAMETER);
    
    // 销毁编解码器
    audio_codec_destroy(codec);
    
    printf("Error handling test passed!\n\n");
    return 0;
}

int main(void) {
    printf("Starting Opus codec tests...\n\n");
    
    // 运行所有测试
    if (test_codec_creation() != 0) return 1;
    if (test_opus_codec_basic() != 0) return 1;
    if (test_opus_codec_encode_decode() != 0) return 1;
    if (test_opus_codec_parameters() != 0) return 1;
    if (test_error_handling() != 0) return 1;
    
    printf("All tests passed successfully!\n");
    return 0;
}