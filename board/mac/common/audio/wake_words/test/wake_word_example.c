#include "wake_word_porcupine.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/**
 * @file wake_word_example.c
 * @brief Porcupine唤醒词使用示例
 * @details 展示如何使用基于Porcupine的Mac版本唤醒词检测
 */

static volatile bool running = true;

// 信号处理函数
void signal_handler(int sig) {
    running = false;
    printf("\n正在退出...\n");
}

// 唤醒词检测回调函数
void wake_word_detected_callback(const char* wake_word, void* user_data) {
    printf("检测到唤醒词: %s\n", wake_word);
    
    // 这里可以添加唤醒词检测后的处理逻辑
    // 例如：启动语音识别、播放提示音等
}

int main(int argc, char* argv[]) {
    printf("Porcupine唤醒词检测示例\n");
    printf("使用方法: %s <access_key> [keyword1] [keyword2] ...\n", argv[0]);
    
    if (argc < 2) {
        printf("错误: 请提供Picovoice访问密钥\n");
        printf("您可以从 https://console.picovoice.ai/ 获取免费的访问密钥\n");
        return -1;
    }
    
    const char* access_key = argv[1];
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建Porcupine配置
    porcupine_config_t config;
    if (porcupine_config_set_default(&config, access_key) != 0) {
        printf("错误: 无法设置默认配置\n");
        return -1;
    }
    
    // 添加关键词
    if (argc > 2) {
        // 使用命令行参数指定的关键词
        for (int i = 2; i < argc; i++) {
            char* keyword_path = porcupine_get_default_keyword_path(argv[i]);
            if (keyword_path) {
                if (porcupine_config_add_keyword(&config, keyword_path, 0.5f) != 0) {
                    printf("警告: 无法添加关键词 %s\n", argv[i]);
                }
                free(keyword_path);
            }
        }
    } else {
        // 使用默认关键词
        const char* default_keywords[] = {"porcupine", "picovoice", "bumblebee"};
        int num_default = sizeof(default_keywords) / sizeof(default_keywords[0]);
        
        for (int i = 0; i < num_default; i++) {
            char* keyword_path = porcupine_get_default_keyword_path(default_keywords[i]);
            if (keyword_path) {
                if (porcupine_config_add_keyword(&config, keyword_path, 0.5f) == 0) {
                    printf("添加关键词: %s\n", default_keywords[i]);
                }
                free(keyword_path);
            }
        }
    }
    
    if (config.num_keywords == 0) {
        printf("错误: 没有有效的关键词\n");
        porcupine_config_destroy(&config);
        return -1;
    }
    
    // 创建唤醒词接口
    WakeWordInterface* wake_word = wake_word_porcupine_create(&config);
    if (!wake_word) {
        printf("错误: 无法创建唤醒词接口\n");
        porcupine_config_destroy(&config);
        return -1;
    }
    
    // 初始化
    if (wake_word_interface_initialize(wake_word, NULL, NULL) != 0) {
        printf("错误: 无法初始化唤醒词接口\n");
        wake_word_interface_destroy(wake_word);
        porcupine_config_destroy(&config);
        return -1;
    }
    
    // 设置回调函数
    wake_word_interface_set_callback(wake_word, wake_word_detected_callback, NULL);
    
    // 启动检测
    wake_word_interface_start(wake_word);
    
    printf("唤醒词检测已启动，支持的关键词:\n");
    for (int i = 0; i < config.num_keywords; i++) {
        printf("  - %s\n", config.keyword_paths[i]);
    }
    printf("按 Ctrl+C 退出\n\n");
    
    // 模拟音频数据输入
    // 在实际应用中，这里应该从麦克风或音频文件读取数据
    size_t frame_size = wake_word_interface_get_feed_size(wake_word);
    int16_t* audio_frame = (int16_t*)calloc(frame_size, sizeof(int16_t));
    
    while (running) {
        // 这里应该从实际音频源获取数据
        // 现在使用静音数据作为示例
        wake_word_interface_feed(wake_word, audio_frame, frame_size);
        
        // 模拟音频采样率 (16kHz, 每帧10ms)
        usleep(10000); // 10ms
    }
    
    // 清理资源
    free(audio_frame);
    wake_word_interface_stop(wake_word);
    wake_word_interface_destroy(wake_word);
    porcupine_config_destroy(&config);
    
    printf("程序已退出\n");
    return 0;
}

/**
 * 编译说明:
 * 
 * 1. 安装Porcupine库:
 *    brew install picovoice-porcupine
 * 
 * 2. 编译命令:
 *    gcc -o wake_word_example wake_word_example.c wake_word_porcupine.c \
 *        -lpv_porcupine -lpthread \
 *        -I/usr/local/include/picovoice \
 *        -L/usr/local/lib
 * 
 * 3. 运行示例:
 *    ./wake_word_example YOUR_ACCESS_KEY porcupine picovoice
 * 
 * 注意事项:
 * - 需要从 https://console.picovoice.ai/ 获取免费的访问密钥
 * - 确保关键词文件(.ppn)存在于指定路径
 * - 在实际应用中需要集成真实的音频输入源
 */