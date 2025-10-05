#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// 包含Mac平台的音频处理器头文件
#include "../../board/mac/common/audio/processor/audio_processor_mac.h"
#include "../../src/audio/processor/audio_processor.h"

// 输出回调函数
void output_callback(const int16_t* data, size_t size, void* user_data) {
    printf("输出回调: 处理了 %zu 个样本\n", size);
}

// VAD状态回调函数
void vad_callback(bool speaking, void* user_data) {
    printf("VAD状态变化: %s\n", speaking ? "检测到语音" : "静音");
}

int main() {
    printf("Mac音频处理器测试\n");
    printf("==================\n");

    // 创建Mac音频处理器
    AudioProcessor* processor = audio_processor_mac_create();
    if (!processor) {
        fprintf(stderr, "错误: 无法创建Mac音频处理器\n");
        return -1;
    }
    printf("✓ 成功创建Mac音频处理器\n");

    // 初始化配置
    audio_processor_config_t config;
    audio_processor_config_init_default(&config, 16000, 1, 60);
    
    // 启用所有功能
    config.enable_aec = true;
    config.enable_ns = true;
    config.enable_vad = true;
    config.vad_threshold = 0.01f;
    
    printf("配置参数:\n");
    printf("  采样率: %d Hz\n", config.sample_rate);
    printf("  声道数: %d\n", config.channels);
    printf("  帧时长: %d ms\n", config.frame_duration_ms);
    printf("  启用AEC: %s\n", config.enable_aec ? "是" : "否");
    printf("  启用NS: %s\n", config.enable_ns ? "是" : "否");
    printf("  启用VAD: %s\n", config.enable_vad ? "是" : "否");

    // 初始化处理器
    audio_processor_error_t err = audio_processor_initialize(processor, &config, NULL);
    if (err != AUDIO_PROCESSOR_SUCCESS) {
        fprintf(stderr, "错误: 初始化处理器失败 - %s\n", audio_processor_error_to_string(err));
        audio_processor_destroy(processor);
        return -1;
    }
    printf("✓ 成功初始化音频处理器\n");

    // 设置回调函数
    audio_processor_set_output_callback(processor, output_callback, NULL);
    audio_processor_set_vad_callback(processor, vad_callback, NULL);
    printf("✓ 设置回调函数\n");

    // 启动处理器
    err = audio_processor_start(processor);
    if (err != AUDIO_PROCESSOR_SUCCESS) {
        fprintf(stderr, "错误: 启动处理器失败 - %s\n", audio_processor_error_to_string(err));
        audio_processor_destroy(processor);
        return -1;
    }
    printf("✓ 成功启动音频处理器\n");

    // 获取输入数据大小
    size_t feed_size = audio_processor_get_feed_size(processor);
    printf("输入数据大小: %zu 样本\n", feed_size);

    // 模拟音频数据处理
    printf("\n开始处理音频数据...\n");
    int16_t* test_data = (int16_t*)calloc(feed_size, sizeof(int16_t));
    if (!test_data) {
        fprintf(stderr, "错误: 无法分配测试数据内存\n");
        audio_processor_stop(processor);
        audio_processor_destroy(processor);
        return -1;
    }

    // 生成一些测试数据（模拟音频信号）
    for (size_t i = 0; i < feed_size; i++) {
        // 生成简单的正弦波信号（440Hz）
        test_data[i] = (int16_t)(10000.0 * sin(2.0 * M_PI * 440.0 * i / config.sample_rate));
    }

    // 处理几帧数据
    for (int frame = 0; frame < 10; frame++) {
        err = audio_processor_feed(processor, test_data, feed_size);
        if (err != AUDIO_PROCESSOR_SUCCESS) {
            fprintf(stderr, "错误: 处理音频数据失败 - %s\n", audio_processor_error_to_string(err));
            break;
        }
        
        // 获取VAD状态
        bool vad_status = audio_processor_get_vad_status(processor);
        printf("帧 %d: VAD状态 = %s\n", frame, vad_status ? "语音活动" : "静音");
        
        // 模拟处理间隔
        usleep(10000); // 10ms
    }

    // 清理资源
    free(test_data);
    audio_processor_stop(processor);
    audio_processor_destroy(processor);
    
    printf("\n测试完成\n");
    return 0;
}