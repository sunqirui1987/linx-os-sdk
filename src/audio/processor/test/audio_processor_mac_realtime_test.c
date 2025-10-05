#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// 包含Mac平台的音频处理器和音频接口头文件
#include "../../board/mac/common/audio/processor/audio_processor_mac.h"
#include "../../board/mac/common/audio/audio/portaudio_mac.h"
#include "../../src/audio/processor/audio_processor.h"
#include "../../src/audio/audio/audio_interface.h"
#include "../../src/common/log/linx_log.h"

static volatile int running = 1;

// 信号处理函数，用于优雅地停止程序
void signal_handler(int sig) {
    running = 0;
    printf("\n正在停止音频处理...\n");
}

// 输出回调函数
void output_callback(const int16_t* data, size_t size, void* user_data) {
    // 获取用户数据（音频接口）
    AudioInterface* audio_interface = (AudioInterface*)user_data;
    
    // 通过音频接口播放处理后的音频
    if (audio_interface) {
        int result = audio_interface_output_data(audio_interface, data, size);
        if (result == 0) {
            // printf("播放处理后的音频数据: %zu 个样本\n", size);
        } else if (result == AUDIO_ERROR_OVERFLOW) {
            static int overflow_count = 0;
            overflow_count++;
            if (overflow_count % 50 == 0) { // 每50次只打印一次
                printf("警告: 音频播放缓冲区溢出 (次数: %d)\n", overflow_count);
            }
        } else {
            printf("播放音频数据失败，错误码: %d\n", result);
        }
    } else {
        printf("音频接口为空，无法播放音频\n");
    }
}

// VAD状态回调函数
void vad_callback(bool speaking, void* user_data) {
    printf("VAD状态变化: %s\n", speaking ? "检测到语音" : "静音");
}

int main() {
      // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_DEBUG;  // 默认INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return 0;
    }
    
    printf("Mac实时音频处理器测试 (带AEC和播放功能)\n");
    printf("=====================================\n");

    // 创建Mac音频接口（PortAudio实现）
    AudioInterface* audio_interface = portaudio_mac_create();
    if (!audio_interface) {
        fprintf(stderr, "错误: 无法创建Mac音频接口\n");
        return -1;
    }
    printf("✓ 成功创建Mac音频接口\n");

    // 初始化音频接口
    int result = audio_interface_init(audio_interface);
    if (result != AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 初始化音频接口失败，错误码: %d\n", result);
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功初始化音频接口\n");

    // 配置音频接口参数 (增大缓冲区以减少溢出)
    audio_interface_set_config(audio_interface, 16000, 256, 1, 8, 4096, 512);
    printf("✓ 设置音频接口配置: 16kHz, 256帧大小, 单声道, 8个周期\n");

    // 明确启用音频输入和输出
    result = audio_interface_enable_input(audio_interface, true);
    if (result != AUDIO_SUCCESS) {
        fprintf(stderr, "警告: 启用音频输入失败，错误码: %d\n", result);
    }
    result = audio_interface_enable_output(audio_interface, true);
    if (result != AUDIO_SUCCESS) {
        fprintf(stderr, "警告: 启用音频输出失败，错误码: %d\n", result);
    }
    
    // 设置输出音量
    result = audio_interface_set_output_volume(audio_interface, 60);
    if (result != AUDIO_SUCCESS) {
        fprintf(stderr, "警告: 设置输出音量失败，错误码: %d\n", result);
    }

    // 启动音频接口
    result = audio_interface_start(audio_interface);
    if (result != AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 启动音频接口失败，错误码: %d\n", result);
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功启动音频接口\n");

    // 创建Mac音频处理器
    AudioProcessor* processor = audio_processor_mac_create();
    if (!processor) {
        fprintf(stderr, "错误: 无法创建Mac音频处理器\n");
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功创建Mac音频处理器\n");

    // 初始化配置 (减小帧时长以减少延迟)
    audio_processor_config_t config;
    audio_processor_config_init_default(&config, 16000, 1, 20);
    
    // 启用所有功能
    config.enable_aec = true;  // 启用回声消除
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
    audio_processor_error_t err = audio_processor_initialize(processor, &config, audio_interface);
    if (err != AUDIO_PROCESSOR_SUCCESS) {
        fprintf(stderr, "错误: 初始化处理器失败 - %s\n", audio_processor_error_to_string(err));
        audio_processor_destroy(processor);
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功初始化音频处理器\n");

    // 设置回调函数
    // 将音频接口作为用户数据传递给输出回调函数，用于播放处理后的音频
    audio_processor_set_output_callback(processor, output_callback, audio_interface);
    audio_processor_set_vad_callback(processor, vad_callback, NULL);
    printf("✓ 设置回调函数\n");

    // 启动处理器
    err = audio_processor_start(processor);
    if (err != AUDIO_PROCESSOR_SUCCESS) {
        fprintf(stderr, "错误: 启动处理器失败 - %s\n", audio_processor_error_to_string(err));
        audio_processor_destroy(processor);
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功启动音频处理器\n");

    // 获取输入数据大小
    size_t feed_size = audio_processor_get_feed_size(processor);
    printf("输入数据大小: %zu 样本\n", feed_size);

    // 设置信号处理
    signal(SIGINT, signal_handler);
    
    printf("\n开始实时处理音频数据，按Ctrl+C停止...\n");
    
    // 分配音频缓冲区
    int16_t* input_buffer = (int16_t*)calloc(feed_size, sizeof(int16_t));
    if (!input_buffer) {
        fprintf(stderr, "错误: 无法分配音频缓冲区内存\n");
        audio_processor_stop(processor);
        audio_processor_destroy(processor);
        audio_interface_destroy(audio_interface);
        return -1;
    }

    // 实时处理循环
    int frame_count = 0;
    int overflow_count = 0;
    while (running) {
        // 从音频接口读取数据
        result = audio_interface_read(audio_interface, input_buffer, feed_size);
        if (result < 0) {
            if (result == AUDIO_ERROR_TIMEOUT) {
                fprintf(stderr, "警告: 读取音频数据超时\n");
                continue; // 继续尝试读取
            } else {
                fprintf(stderr, "错误: 读取音频数据失败，错误码: %d\n", result);
                break;
            }
        }
        
        // 处理音频数据
        err = audio_processor_feed(processor, input_buffer, feed_size);
        if (err != AUDIO_PROCESSOR_SUCCESS) {
            fprintf(stderr, "错误: 处理音频数据失败 - %s\n", audio_processor_error_to_string(err));
            break;
        }
        
        // 获取VAD状态并打印（如果检测到语音活动）
        bool vad_status = audio_processor_get_vad_status(processor);
        if (vad_status) {
            printf("VAD检测: 语音活动 (帧 #%d)\n", frame_count);
        }
        
        frame_count++;
        if (frame_count % 100 == 0) {
            printf("已处理 %d 帧音频数据，溢出次数: %d\n", frame_count, overflow_count);
        }
        
        // 添加小延迟以减少CPU使用率并缓解缓冲区溢出
        usleep(5000); // 5ms
    }

    // 清理资源
    free(input_buffer);
    audio_processor_stop(processor);
    audio_processor_destroy(processor);
    audio_interface_destroy(audio_interface);
    
    printf("\n实时音频处理测试完成，总共处理了 %d 帧，溢出次数: %d\n", frame_count, overflow_count);
    return 0;
}