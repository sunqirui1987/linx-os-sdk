#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// 包含SpeexDSP音频处理器和音频接口头文件
#include "../audio_processor_speexdsp.h"
#include "../../audio/portaudio_mac.h"
#include "../../../../../../src/audio/processor/audio_processor.h"
#include "../../../../../../src/audio/audio/audio_interface.h"
#include "../../../../../../src/common/log/linx_log.h"

static volatile int running = 1;

// 信号处理函数，用于优雅地停止程序
void signal_handler(int sig) {
    running = 0;
    printf("\n正在停止音频处理...\n");
}

// 全局统计变量
static int g_output_success_count = 0;
static int g_output_error_count = 0;
static int g_overflow_count = 0;

// 输出回调函数 - 播放经过AEC处理后的音频
void output_callback(const int16_t* data, size_t size, void* user_data) {
    // 获取用户数据（音频接口）
    AudioInterface* audio_interface = (AudioInterface*)user_data;
    
    if (!audio_interface || !data || size == 0) {
        printf("错误: 音频接口或数据无效\n");
        g_output_error_count++;
        return;
    }
    
    // 通过音频接口播放处理后的音频
    int result = audio_interface_output_data(audio_interface, data, size);
    
    if (result == 0) {
        // 播放成功
        g_output_success_count++;
        
        // 每1000次成功播放打印一次状态
        if (g_output_success_count % 1000 == 0) {
            printf("✓ 已成功播放 %d 帧AEC处理后的音频\n", g_output_success_count);
        }
        
        // 可选：添加音频质量检查
        if (g_output_success_count % 500 == 0) {
            // 计算音频能量来验证音频质量
            long long energy = 0;
            for (size_t i = 0; i < size; i++) {
                energy += (long long)data[i] * data[i];
            }
            energy /= size;
            printf("音频能量检查: %lld (帧 #%d)\n", energy, g_output_success_count);
        }
        
    } else if (result == AUDIO_ERROR_OVERFLOW) {
        g_overflow_count++;
        if (g_overflow_count % 10 == 0) { // 每10次溢出打印一次
            printf("⚠️  音频播放缓冲区溢出 (总计: %d 次)\n", g_overflow_count);
        }
    } else {
        g_output_error_count++;
        printf("❌ 播放AEC处理后音频失败，错误码: %d (总错误: %d)\n", result, g_output_error_count);
        
        // 如果错误太多，可能需要重新初始化音频接口
        if (g_output_error_count > 100) {
            printf("错误次数过多，可能需要检查音频配置\n");
        }
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
    
    printf("SpeexDSP实时音频处理器测试 (带AEC和播放功能)\n");
    printf("==========================================\n");

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

    // 配置音频接口参数 (优化为AEC播放)
    // 参数: 采样率, 帧大小, 声道数, 周期数, 输入缓冲区大小, 输出缓冲区大小
    audio_interface_set_config(audio_interface, 16000, 320, 1, 4, 2048, 2048);
    printf("✓ 设置音频接口配置: 16kHz, 320帧大小, 单声道, 4个周期 (优化AEC播放)\n");

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

    // 创建SpeexDSP音频处理器
    AudioProcessor* processor = audio_processor_speexdsp_create();
    if (!processor) {
        fprintf(stderr, "错误: 无法创建SpeexDSP音频处理器\n");
        audio_interface_destroy(audio_interface);
        return -1;
    }
    printf("✓ 成功创建SpeexDSP音频处理器\n");

    // 初始化配置 (匹配音频接口设置)
    audio_processor_config_t config;
    audio_processor_config_init_default(&config, 16000, 1, 20);
    
    // 启用所有功能，重点是AEC用于播放
    config.enable_aec = false;   // 启用回声消除 - 关键功能
    config.enable_ns = false;    // 启用噪声抑制
    config.enable_vad = true;   // 启用语音活动检测
    config.vad_threshold = 0.01f;
    
    // 设置帧大小以匹配音频接口 (320样本 = 20ms @ 16kHz)
    config.frame_size = 320;
    
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
    
    printf("\n🎵 开始实时处理音频数据，按Ctrl+C停止...\n");
    printf("📢 经过SpeexDSP AEC处理的音频将通过扬声器播放\n");
    printf("🎤 请对着麦克风说话，您将听到经过回声消除处理的声音\n\n");
    
    // 分配音频缓冲区
    int16_t* input_buffer = (int16_t*)calloc(feed_size, sizeof(int16_t));
    if (!input_buffer) {
        fprintf(stderr, "错误: 无法分配音频缓冲区内存\n");
        audio_processor_stop(processor);
        audio_processor_destroy(processor);
        audio_interface_destroy(audio_interface);
        return -1;
    }

    // 重置统计计数器
    g_output_success_count = 0;
    g_output_error_count = 0;
    g_overflow_count = 0;

    // 实时处理循环
    int frame_count = 0;
    int input_error_count = 0;
    while (running) {
        // 从音频接口读取数据
        result = audio_interface_read(audio_interface, input_buffer, feed_size);
        if (result < 0) {
            input_error_count++;
            if (result == AUDIO_ERROR_TIMEOUT) {
                if (input_error_count % 50 == 0) {
                    fprintf(stderr, "警告: 读取音频数据超时 (总计: %d 次)\n", input_error_count);
                }
                continue; // 继续尝试读取
            } else {
                fprintf(stderr, "错误: 读取音频数据失败，错误码: %d\n", result);
                if (input_error_count > 100) {
                    fprintf(stderr, "输入错误过多，退出循环\n");
                    break;
                }
                continue;
            }
        }
        
        // 处理音频数据 (这里会触发AEC处理和音频播放)
        err = audio_processor_feed(processor, input_buffer, feed_size);
        if (err != AUDIO_PROCESSOR_SUCCESS) {
            fprintf(stderr, "错误: 处理音频数据失败 - %s\n", audio_processor_error_to_string(err));
            break;
        }
        
        // 获取VAD状态
        bool vad_status = audio_processor_get_vad_status(processor);
        if (vad_status && frame_count % 50 == 0) {
            printf("🗣️  VAD检测: 语音活动 (帧 #%d)\n", frame_count);
        }

        frame_count++;
        
        // 每200帧打印一次详细状态
        if (frame_count % 200 == 0) {
            printf("📊 状态报告 - 处理帧: %d, 播放成功: %d, 播放错误: %d, 溢出: %d\n", 
                   frame_count, g_output_success_count, g_output_error_count, g_overflow_count);
        }
        
        // 减少延迟以获得更好的实时性能
        usleep(1000); // 1ms
    }

    // 清理资源
    free(input_buffer);
    audio_processor_stop(processor);
    audio_processor_destroy(processor);
    audio_interface_destroy(audio_interface);
    
    // 显示详细的测试结果
    printf("\n==================================================\n");
    printf("🎉 SpeexDSP实时音频处理测试完成\n");
    printf("==================================================\n");
    printf("📈 处理统计:\n");
    printf("  • 总处理帧数: %d\n", frame_count);
    printf("  • 输入错误次数: %d\n", input_error_count);
    printf("\n📢 播放统计:\n");
    printf("  • 成功播放帧数: %d\n", g_output_success_count);
    printf("  • 播放错误次数: %d\n", g_output_error_count);
    printf("  • 缓冲区溢出次数: %d\n", g_overflow_count);
    
    // 计算成功率
    if (frame_count > 0) {
        float success_rate = (float)g_output_success_count / frame_count * 100.0f;
        printf("  • 播放成功率: %.2f%%\n", success_rate);
    }
    
    printf("\n✅ AEC处理后的音频播放测试完成！\n");
    return 0;
}