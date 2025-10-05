#include "../audio_interface.h"
#include "../audio_stub.h"
#include "mac/common/audio/audio/portaudio_mac.h"
#include "common/log/linx_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static volatile int running = 1;

/**
 * 信号处理函数
 */
void signal_handler(int sig) {
    running = 0;
    printf("\n正在停止音频测试...\n");
}

/**
 * 测试音频录制和播放功能（回声测试）
 */
int test_audio_record_play() {
    printf("测试音频录制和播放功能...\n");
    
    // 创建PortAudio Mac实现
    AudioInterface* audio = portaudio_mac_create();
    if (!audio) {
        printf("创建音频接口失败\n");
        return -1;
    }
    
    // 初始化音频接口
    int result = audio_interface_init(audio);
    if (result != AUDIO_SUCCESS) {
        printf("初始化音频接口失败，错误码: %d\n", result);
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 设置音频配置
    // 16kHz, 1024帧大小, 1通道(单声道), 4周期, 8192缓冲区大小, 2048周期大小
    audio_interface_set_config(audio, 16000, 1024, 1, 4, 8192, 2048);
    
    printf("音频配置:\n");
    printf("  采样率: %u Hz\n", audio->sample_rate);
    printf("  通道数: %d\n", audio->channels);
    printf("  帧大小: %d\n", audio->frame_size);
    printf("  缓冲区大小: %d\n", audio->buffer_size);
    
    // 启动音频接口
    printf("启动音频接口...\n");
    result = audio_interface_start(audio);
    if (result != AUDIO_SUCCESS) {
        printf("启动音频接口失败，错误码: %d\n", result);
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 检查输入输出是否启用
    if (!audio_interface_input_enabled(audio)) {
        printf("音频输入未启用\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    if (!audio_interface_output_enabled(audio)) {
        printf("音频输出未启用\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 设置音量
    result = audio_interface_set_output_volume(audio, 100); // 设置为50%音量
    if (result == AUDIO_SUCCESS) {
        printf("输出音量设置为: %d%%\n", audio_interface_output_volume(audio));
    }
    
    // 分配音频缓冲区
    size_t buffer_size = audio->frame_size * audio->channels;
    int16_t* audio_buffer = (int16_t*)malloc(buffer_size * sizeof(int16_t));
    if (!audio_buffer) {
        printf("分配音频缓冲区失败\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    printf("开始录制和播放音频... 按Ctrl+C停止\n");
    printf("你应该能听到麦克风输入通过扬声器/耳机播放出来\n");
    
    // 主循环：从麦克风读取并写入扬声器
    int frames_processed = 0;
    int read_errors = 0;
    int write_errors = 0;
    
    while (running) {
        // 使用高级接口读取音频数据
        int samples_read = audio_interface_input_data(audio, audio_buffer, buffer_size);
        if (samples_read > 0) {
            // 使用高级接口写入音频数据（回声）
            int samples_written = audio_interface_output_data(audio, audio_buffer, samples_read);
            if (samples_written > 0) {
                frames_processed++;
                if (frames_processed % 100 == 0) {
                    printf("已处理 %d 帧\n", frames_processed);
                }
            } else {
                write_errors++;
                if (write_errors % 10 == 1) {
                    printf("写入音频数据失败，错误次数: %d\n", write_errors);
                }
            }
        } else {
            read_errors++;
            if (read_errors % 10 == 1) {
                printf("读取音频数据失败，错误次数: %d\n", read_errors);
            }
            usleep(1000); // 休眠1ms避免忙等待
        }
    }
    
    printf("总共处理了 %d 帧\n", frames_processed);
    printf("读取错误: %d 次，写入错误: %d 次\n", read_errors, write_errors);
    
    // 清理资源
    free(audio_buffer);
    audio_interface_destroy(audio);
    
    printf("音频测试成功完成\n");
    return 0;
}

/**
 * 测试基本音频接口功能
 */
int test_audio_basic() {
    printf("测试基本音频接口功能...\n");
    
    // 创建PortAudio Mac实现
    AudioInterface* audio = portaudio_mac_create();
    if (!audio) {
        printf("创建音频接口失败\n");
        return -1;
    }
    
    printf("✓ 音频接口创建成功\n");
    
    // 测试初始化
    int result = audio_interface_init(audio);
    if (result == AUDIO_SUCCESS) {
        printf("✓ 音频接口初始化成功\n");
    } else {
        printf("✗ 音频接口初始化失败，错误码: %d\n", result);
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 测试配置
    audio_interface_set_config(audio, 16000, 512, 1, 2, 4096, 1024);
    if (audio->sample_rate == 16000 && audio->channels == 1) {
        printf("✓ 音频配置设置正确\n");
    } else {
        printf("✗ 音频配置失败\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 测试启动
    result = audio_interface_start(audio);
    if (result == AUDIO_SUCCESS) {
        printf("✓ 音频接口启动成功\n");
    } else {
        printf("✗ 音频接口启动失败，错误码: %d\n", result);
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 测试音量控制
    result = audio_interface_set_output_volume(audio, 75);
    if (result == AUDIO_SUCCESS && audio_interface_output_volume(audio) == 75) {
        printf("✓ 音量控制功能正常\n");
    } else {
        printf("✗ 音量控制功能失败\n");
    }
    
    // 测试输入输出启用状态
    if (audio_interface_input_enabled(audio) && audio_interface_output_enabled(audio)) {
        printf("✓ 输入输出启用状态正常\n");
    } else {
        printf("✗ 输入输出启用状态异常\n");
    }
    
    // 测试getter函数
    printf("音频接口属性:\n");
    printf("  支持全双工: %s\n", audio_interface_duplex(audio) ? "是" : "否");
    printf("  输入采样率: %d Hz\n", audio_interface_input_sample_rate(audio));
    printf("  输出采样率: %d Hz\n", audio_interface_output_sample_rate(audio));
    printf("  输入通道数: %d\n", audio_interface_input_channels(audio));
    printf("  输出通道数: %d\n", audio_interface_output_channels(audio));
    
    // 清理资源
    audio_interface_destroy(audio);
    printf("✓ 音频接口销毁成功\n");
    
    printf("基本音频测试成功完成\n");
    return 0;
}

/**
 * 测试音频桩实现
 */
int test_audio_stub() {
    printf("测试音频桩实现...\n");
    
    // 创建音频桩实现
    AudioInterface* audio = audio_stub_create();
    if (!audio) {
        printf("创建音频桩失败\n");
        return -1;
    }
    
    printf("✓ 音频桩创建成功\n");
    
    // 测试初始化
    int result = audio_interface_init(audio);
    if (result == AUDIO_SUCCESS) {
        printf("✓ 音频桩初始化成功\n");
    } else {
        printf("✗ 音频桩初始化失败\n");
        audio_interface_destroy(audio);
        return -1;
    }
    
    // 测试配置
    audio_interface_set_config(audio, 44100, 1024, 2, 4, 8192, 2048);
    printf("✓ 音频桩配置设置完成\n");
    
    // 测试启动
    result = audio_interface_start(audio);
    if (result == AUDIO_SUCCESS) {
        printf("✓ 音频桩启动成功\n");
    } else {
        printf("✗ 音频桩启动失败\n");
    }
    
    // 测试数据处理
    int16_t test_data[1024];
    memset(test_data, 0, sizeof(test_data));
    
    // 测试输出数据
    int samples_written = audio_interface_output_data(audio, test_data, 1024);
    if (samples_written > 0) {
        printf("✓ 音频桩输出数据测试成功\n");
    } else {
        printf("✗ 音频桩输出数据测试失败\n");
    }
    
    // 测试输入数据
    int samples_read = audio_interface_input_data(audio, test_data, 1024);
    if (samples_read > 0) {
        printf("✓ 音频桩输入数据测试成功\n");
    } else {
        printf("✗ 音频桩输入数据测试失败\n");
    }
    
    // 清理资源
    audio_interface_destroy(audio);
    printf("✓ 音频桩销毁成功\n");
    
    printf("音频桩测试成功完成\n");
    return 0;
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    printf("=== LINX 音频测试套件 ===\n\n");

    // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_INFO;  // 设置为INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        printf("日志系统初始化失败\n");
        return 1;
    }
    
    // 设置信号处理器
    signal(SIGINT, signal_handler);
    
    // 首先运行音频桩测试
    printf("1. 运行音频桩测试...\n");
    if (test_audio_stub() != 0) {
        printf("音频桩测试失败\n");
        return 1;
    }
    
    printf("\n");
    
    // 运行基本测试
    printf("2. 运行基本音频测试...\n");
    if (test_audio_basic() != 0) {
        printf("基本音频测试失败\n");
        return 1;
    }
    
    printf("\n");
    
    // 询问用户是否要运行交互式测试
    if (argc > 1 && strcmp(argv[1], "--interactive") == 0) {
        printf("3. 运行交互式音频测试...\n");
        printf("请确保你已连接麦克风和扬声器/耳机\n");
        printf("按回车键继续或Ctrl+C跳过...");
        getchar();
        
        if (test_audio_record_play() != 0) {
            printf("交互式音频测试失败\n");
            return 1;
        }
    } else {
        printf("要运行交互式测试（录制/播放），请使用: %s --interactive\n", argv[0]);
    }
    
    printf("\n=== 所有测试完成 ===\n");
    return 0;
}