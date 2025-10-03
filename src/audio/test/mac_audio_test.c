/**
 * @file mac_audio_test.c
 * @brief Mac平台音频服务测试demo
 * @details 基于Mac平台特定实现的音频服务测试，包含PortAudio和Mac音频处理器
 */

#include "../audio_service.h"
#include "../../../board/mac/common/audio/audio/portaudio_mac.h"
#include "../../../board/mac/common/audio/processor/audio_processor_mac.h"
#include "../../common/log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define MAC_TEST_TAG "MacAudioTest"
#define TEST_DURATION_SECONDS 30
#define TEST_SAMPLE_RATE 16000
#define TEST_CHANNELS 1
#define TEST_FRAME_DURATION_MS 60

// 全局变量
static AudioService* g_audio_service = NULL;
static AudioInterface* g_audio_interface = NULL;
static AudioProcessor* g_audio_processor = NULL;
static bool g_test_running = true;
static int g_vad_changes = 0;
static int g_processed_frames = 0;

// 测试统计
typedef struct {
    int vad_speech_count;
    int vad_silence_count;
    int audio_frames_processed;
    int processor_callbacks;
    time_t start_time;
    time_t end_time;
    float total_energy;
    float max_energy;
    float min_energy;
} MacTestStatistics;

static MacTestStatistics g_test_stats = {0};

// 信号处理函数
void mac_signal_handler(int sig) {
    LINX_LOGI(MAC_TEST_TAG, "收到信号 %d，停止测试", sig);
    g_test_running = false;
}

// Mac音频处理器回调函数
void mac_processor_output_callback(const int16_t* data, size_t size, void* user_data) {
    g_test_stats.processor_callbacks++;
    g_processed_frames++;
    
    // 计算音频能量统计
    float energy = 0.0f;
    for (size_t i = 0; i < size; i++) {
        float sample = (float)data[i] / 32768.0f;
        energy += sample * sample;
    }
    energy /= size;
    
    g_test_stats.total_energy += energy;
    if (energy > g_test_stats.max_energy) {
        g_test_stats.max_energy = energy;
    }
    if (g_test_stats.min_energy == 0 || energy < g_test_stats.min_energy) {
        g_test_stats.min_energy = energy;
    }
    
    LINX_LOGD(MAC_TEST_TAG, "处理器输出回调，帧大小: %zu, 能量: %.6f", size, energy);
}

void mac_processor_vad_callback(bool speaking, void* user_data) {
    g_vad_changes++;
    
    if (speaking) {
        g_test_stats.vad_speech_count++;
        LINX_LOGI(MAC_TEST_TAG, "VAD检测到语音活动，总计语音帧: %d", g_test_stats.vad_speech_count);
    } else {
        g_test_stats.vad_silence_count++;
        LINX_LOGI(MAC_TEST_TAG, "VAD检测到静音，总计静音帧: %d", g_test_stats.vad_silence_count);
    }
}

// 音频服务回调函数
void mac_send_queue_callback(void* user_data) {
    LINX_LOGD(MAC_TEST_TAG, "发送队列可用回调");
    
    // 从发送队列获取数据包
    AudioService* service = (AudioService*)user_data;
    AudioStreamPacket* packet = audio_service_pop_packet_from_send_queue(service);
    if (packet) {
        LINX_LOGD(MAC_TEST_TAG, "从发送队列获取数据包，大小: %zu bytes", packet->payload_size);
        audio_stream_packet_destroy(packet);
    }
}

void mac_wake_word_callback(const char* wake_word, void* user_data) {
    LINX_LOGI(MAC_TEST_TAG, "检测到唤醒词: '%s'", wake_word);
}

void mac_vad_change_callback(bool speaking, void* user_data) {
    LINX_LOGI(MAC_TEST_TAG, "音频服务VAD状态变化: %s", speaking ? "说话中" : "静音");
}

// 创建Mac音频接口
AudioInterface* create_mac_audio_interface() {
    LINX_LOGI(MAC_TEST_TAG, "创建Mac PortAudio接口");
    
    AudioInterface* interface = portaudio_mac_create();
    if (!interface) {
        LINX_LOGE(MAC_TEST_TAG, "创建PortAudio接口失败");
        return NULL;
    }
    
    // 配置音频参数
    interface->vtable->set_config(interface, TEST_SAMPLE_RATE, 
                                 (TEST_SAMPLE_RATE * TEST_FRAME_DURATION_MS) / 1000,
                                 TEST_CHANNELS, 4, 4096, 1024);
    
    // 初始化接口
    if (interface->vtable->init(interface) != 0) {
        LINX_LOGE(MAC_TEST_TAG, "初始化PortAudio接口失败");
        interface->vtable->destroy(interface);
        return NULL;
    }
    
    LINX_LOGI(MAC_TEST_TAG, "Mac PortAudio接口创建成功");
    return interface;
}

// 创建Mac音频处理器
AudioProcessor* create_mac_audio_processor() {
    LINX_LOGI(MAC_TEST_TAG, "创建Mac音频处理器");
    
    AudioProcessor* processor = audio_processor_mac_create();
    if (!processor) {
        LINX_LOGE(MAC_TEST_TAG, "创建Mac音频处理器失败");
        return NULL;
    }
    
    // 配置音频处理器
    audio_processor_config_t config;
    config.sample_rate = TEST_SAMPLE_RATE;
    config.channels = TEST_CHANNELS;
    config.frame_duration_ms = TEST_FRAME_DURATION_MS;
    config.frame_size = (TEST_SAMPLE_RATE * TEST_FRAME_DURATION_MS) / 1000;
    config.enable_aec = true;
    config.enable_ns = true;
    config.enable_vad = true;
    config.vad_threshold = 0.01f;
    config.models_list = NULL;
    
    // 初始化处理器
    if (audio_processor_initialize(processor, &config, NULL) != AUDIO_PROCESSOR_SUCCESS) {
        LINX_LOGE(MAC_TEST_TAG, "初始化Mac音频处理器失败");
        audio_processor_destroy(processor);
        return NULL;
    }
    
    // 设置回调函数
    audio_processor_set_output_callback(processor, mac_processor_output_callback, NULL);
    audio_processor_set_vad_callback(processor, mac_processor_vad_callback, NULL);
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频处理器创建成功");
    return processor;
}

// 创建模拟编解码器
audio_codec_t* create_mac_test_codec() {
    audio_codec_t* codec = (audio_codec_t*)calloc(1, sizeof(audio_codec_t));
    if (!codec) {
        return NULL;
    }
    
    // 设置基本格式
    codec->format.sample_rate = TEST_SAMPLE_RATE;
    codec->format.channels = TEST_CHANNELS;
    codec->format.bits_per_sample = 16;
    codec->format.frame_size_ms = TEST_FRAME_DURATION_MS;
    
    LINX_LOGI(MAC_TEST_TAG, "创建测试编解码器成功");
    return codec;
}

// 音频数据生成器线程
void* mac_audio_generator_thread(void* arg) {
    AudioService* service = (AudioService*)arg;
    int frame_samples = (TEST_SAMPLE_RATE * TEST_FRAME_DURATION_MS) / 1000;
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频数据生成器线程启动，帧大小: %d 样本", frame_samples);
    
    while (g_test_running) {
        // 生成测试音频数据（正弦波 + 噪声）
        int16_t* test_data = (int16_t*)malloc(frame_samples * sizeof(int16_t));
        if (test_data) {
            static double phase = 0.0;
            double frequency = 440.0; // A4音符
            
            for (int i = 0; i < frame_samples; i++) {
                // 生成正弦波
                double sine_wave = sin(phase) * 0.3;
                
                // 添加随机噪声
                double noise = ((double)rand() / RAND_MAX - 0.5) * 0.1;
                
                // 合成信号
                double signal = sine_wave + noise;
                
                test_data[i] = (int16_t)(signal * 16000);
                
                phase += 2.0 * M_PI * frequency / TEST_SAMPLE_RATE;
                if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
            }
            
            // 输入到音频处理器
            if (g_audio_processor) {
                audio_processor_feed(g_audio_processor, test_data, frame_samples);
                g_test_stats.audio_frames_processed++;
            }
            
            free(test_data);
        }
        
        // 控制数据生成速率
        usleep(TEST_FRAME_DURATION_MS * 1000);
    }
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频数据生成器线程停止");
    return NULL;
}

// 测试Mac音频处理器功能
int test_mac_audio_processor() {
    LINX_LOGI(MAC_TEST_TAG, "开始测试Mac音频处理器功能");
    
    // 创建音频处理器
    g_audio_processor = create_mac_audio_processor();
    if (!g_audio_processor) {
        LINX_LOGE(MAC_TEST_TAG, "创建Mac音频处理器失败");
        return -1;
    }
    
    // 启动音频处理器
    if (audio_processor_start(g_audio_processor) != AUDIO_PROCESSOR_SUCCESS) {
        LINX_LOGE(MAC_TEST_TAG, "启动Mac音频处理器失败");
        return -1;
    }
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频处理器启动成功");
    
    // 测试各种功能
    LINX_LOGI(MAC_TEST_TAG, "测试VAD状态: %s", 
              audio_processor_get_vad_status(g_audio_processor) ? "语音" : "静音");
    
    LINX_LOGI(MAC_TEST_TAG, "测试处理延迟: %d ms", 
              audio_processor_get_delay_ms(g_audio_processor));
    
    LINX_LOGI(MAC_TEST_TAG, "测试期望输入大小: %zu 样本", 
              audio_processor_get_feed_size(g_audio_processor));
    
    // 测试AEC开关
    audio_processor_enable_device_aec(g_audio_processor, true);
    LINX_LOGI(MAC_TEST_TAG, "启用设备AEC");
    
    return 0;
}

// 测试Mac音频接口功能
int test_mac_audio_interface() {
    LINX_LOGI(MAC_TEST_TAG, "开始测试Mac音频接口功能");
    
    // 创建音频接口
    g_audio_interface = create_mac_audio_interface();
    if (!g_audio_interface) {
        LINX_LOGE(MAC_TEST_TAG, "创建Mac音频接口失败");
        return -1;
    }
    
    // 测试录音功能
    if (g_audio_interface->vtable->record(g_audio_interface) == 0) {
        LINX_LOGI(MAC_TEST_TAG, "音频录音启动成功");
    } else {
        LINX_LOGW(MAC_TEST_TAG, "音频录音启动失败");
    }
    
    // 测试播放功能
    if (g_audio_interface->vtable->init_play(g_audio_interface) == 0) {
        LINX_LOGI(MAC_TEST_TAG, "音频播放初始化成功");
    } else {
        LINX_LOGW(MAC_TEST_TAG, "音频播放初始化失败");
    }
    
    LINX_LOGI(MAC_TEST_TAG, "播放缓冲区状态: %s", 
              g_audio_interface->vtable->is_play_buffer_empty(g_audio_interface) ? "空" : "非空");
    
    return 0;
}

// 测试完整的音频服务
int test_mac_audio_service() {
    LINX_LOGI(MAC_TEST_TAG, "开始测试Mac音频服务");
    
    // 创建音频服务
    g_audio_service = audio_service_create();
    if (!g_audio_service) {
        LINX_LOGE(MAC_TEST_TAG, "创建音频服务失败");
        return -1;
    }
    
    // 创建编解码器
    audio_codec_t* codec = create_mac_test_codec();
    if (!codec) {
        LINX_LOGE(MAC_TEST_TAG, "创建编解码器失败");
        audio_service_destroy(g_audio_service);
        return -1;
    }
    
    // 初始化音频服务
    if (audio_service_initialize(g_audio_service, codec) != 0) {
        LINX_LOGE(MAC_TEST_TAG, "初始化音频服务失败");
        free(codec);
        audio_service_destroy(g_audio_service);
        return -1;
    }
    
    // 设置Mac音频处理器
    g_audio_service->audio_processor = g_audio_processor;
    
    // 设置回调函数
    AudioServiceCallbacks callbacks = {
        .on_send_queue_available = mac_send_queue_callback,
        .on_wake_word_detected = mac_wake_word_callback,
        .on_vad_change = mac_vad_change_callback,
        .user_data = g_audio_service
    };
    audio_service_set_callbacks(g_audio_service, &callbacks);
    
    // 启动音频服务
    if (audio_service_start(g_audio_service) != 0) {
        LINX_LOGE(MAC_TEST_TAG, "启动音频服务失败");
        free(codec);
        audio_service_destroy(g_audio_service);
        return -1;
    }
    
    // 启用各种功能
    audio_service_enable_voice_processing(g_audio_service, true);
    audio_service_enable_device_aec(g_audio_service, true);
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频服务启动成功");
    return 0;
}

// 打印测试统计
void print_mac_test_statistics() {
    g_test_stats.end_time = time(NULL);
    double duration = difftime(g_test_stats.end_time, g_test_stats.start_time);
    
    LINX_LOGI(MAC_TEST_TAG, "=== Mac音频测试统计报告 ===");
    LINX_LOGI(MAC_TEST_TAG, "测试持续时间: %.1f 秒", duration);
    LINX_LOGI(MAC_TEST_TAG, "处理的音频帧: %d", g_test_stats.audio_frames_processed);
    LINX_LOGI(MAC_TEST_TAG, "处理器回调次数: %d", g_test_stats.processor_callbacks);
    LINX_LOGI(MAC_TEST_TAG, "VAD语音检测: %d 次", g_test_stats.vad_speech_count);
    LINX_LOGI(MAC_TEST_TAG, "VAD静音检测: %d 次", g_test_stats.vad_silence_count);
    LINX_LOGI(MAC_TEST_TAG, "VAD状态变化: %d 次", g_vad_changes);
    
    if (g_test_stats.processor_callbacks > 0) {
        float avg_energy = g_test_stats.total_energy / g_test_stats.processor_callbacks;
        LINX_LOGI(MAC_TEST_TAG, "平均音频能量: %.6f", avg_energy);
        LINX_LOGI(MAC_TEST_TAG, "最大音频能量: %.6f", g_test_stats.max_energy);
        LINX_LOGI(MAC_TEST_TAG, "最小音频能量: %.6f", g_test_stats.min_energy);
    }
    
    if (duration > 0) {
        LINX_LOGI(MAC_TEST_TAG, "平均处理帧率: %.1f 帧/秒", 
                  g_test_stats.audio_frames_processed / duration);
    }
}

// 清理资源
void cleanup_mac_test() {
    LINX_LOGI(MAC_TEST_TAG, "开始清理Mac测试资源");
    
    if (g_audio_service) {
        audio_service_stop(g_audio_service);
        if (g_audio_service->codec) {
            free(g_audio_service->codec);
        }
        audio_service_destroy(g_audio_service);
        g_audio_service = NULL;
    }
    
    if (g_audio_processor) {
        audio_processor_stop(g_audio_processor);
        audio_processor_destroy(g_audio_processor);
        g_audio_processor = NULL;
    }
    
    if (g_audio_interface) {
        g_audio_interface->vtable->destroy(g_audio_interface);
        g_audio_interface = NULL;
    }
    
    LINX_LOGI(MAC_TEST_TAG, "Mac测试资源清理完成");
}

// 主函数
int main(int argc, char* argv[]) {
    LINX_LOGI(MAC_TEST_TAG, "=== Mac平台音频服务测试开始 ===");
    
    // 设置信号处理
    signal(SIGINT, mac_signal_handler);
    signal(SIGTERM, mac_signal_handler);
    
    // 记录开始时间
    g_test_stats.start_time = time(NULL);
    
    int failed_tests = 0;
    
    // 测试Mac音频处理器
    if (test_mac_audio_processor() != 0) {
        LINX_LOGE(MAC_TEST_TAG, "Mac音频处理器测试失败");
        failed_tests++;
    }
    
    // 测试Mac音频接口
    if (test_mac_audio_interface() != 0) {
        LINX_LOGE(MAC_TEST_TAG, "Mac音频接口测试失败");
        failed_tests++;
    }
    
    // 测试完整音频服务
    if (test_mac_audio_service() != 0) {
        LINX_LOGE(MAC_TEST_TAG, "Mac音频服务测试失败");
        failed_tests++;
    }
    
    if (failed_tests > 0) {
        LINX_LOGE(MAC_TEST_TAG, "初始化测试失败，退出");
        cleanup_mac_test();
        return failed_tests;
    }
    
    // 创建音频数据生成器线程
    pthread_t generator_thread;
    if (pthread_create(&generator_thread, NULL, mac_audio_generator_thread, g_audio_service) != 0) {
        LINX_LOGE(MAC_TEST_TAG, "创建音频数据生成器线程失败");
        cleanup_mac_test();
        return -1;
    }
    
    LINX_LOGI(MAC_TEST_TAG, "Mac音频测试运行中，将持续 %d 秒...", TEST_DURATION_SECONDS);
    
    // 主测试循环
    for (int i = 0; i < TEST_DURATION_SECONDS && g_test_running; i++) {
        sleep(1);
        
        // 每5秒输出状态
        if (i % 5 == 0) {
            LINX_LOGI(MAC_TEST_TAG, "测试进行中... %d/%d 秒, 已处理帧: %d, VAD变化: %d", 
                      i + 1, TEST_DURATION_SECONDS, g_processed_frames, g_vad_changes);
            
            // 查询音频服务状态
            if (g_audio_service) {
                bool voice_detected = audio_service_is_voice_detected(g_audio_service);
                bool is_idle = audio_service_is_idle(g_audio_service);
                bool processor_running = audio_service_is_audio_processor_running(g_audio_service);
                
                LINX_LOGI(MAC_TEST_TAG, "音频服务状态 - 语音检测: %s, 空闲: %s, 处理器运行: %s",
                          voice_detected ? "是" : "否",
                          is_idle ? "是" : "否", 
                          processor_running ? "是" : "否");
            }
        }
        
        // 每10秒重置处理器
        if (i % 10 == 0 && i > 0 && g_audio_processor) {
            LINX_LOGI(MAC_TEST_TAG, "重置音频处理器...");
            audio_processor_reset(g_audio_processor);
        }
    }
    
    // 停止测试
    g_test_running = false;
    
    // 等待生成器线程结束
    pthread_join(generator_thread, NULL);
    
    // 打印统计信息
    print_mac_test_statistics();
    
    // 清理资源
    cleanup_mac_test();
    
    LINX_LOGI(MAC_TEST_TAG, "=== Mac平台音频服务测试结束 ===");
    
    if (failed_tests == 0) {
        LINX_LOGI(MAC_TEST_TAG, "🎉 所有测试通过！");
    } else {
        LINX_LOGE(MAC_TEST_TAG, "❌ %d 个测试失败", failed_tests);
    }
    
    return failed_tests;
}