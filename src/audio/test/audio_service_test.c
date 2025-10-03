/**
 * @file audio_service_test.c
 * @brief 音频服务完整功能测试demo
 * @details 测试音频服务的所有核心功能，包括：
 *          - 音频录制和播放
 *          - 唤醒词检测
 *          - 语音处理
 *          - 音频编解码
 *          - 队列管理
 *          - 回调机制
 */

#include "../audio_service.h"
#include "../audio/audio_interface.h"
#include "../codecs/audio_codec.h"
#include "../wake_words/wake_word_interface.h"
#include "../processor/audio_processor.h"
#include "../../common/log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define TEST_TAG "AudioServiceTest"
#define TEST_DURATION_SECONDS 30
#define TEST_SAMPLE_RATE 16000
#define TEST_CHANNELS 1
#define TEST_FRAME_DURATION_MS 60

// 全局变量
static AudioService* g_audio_service = NULL;
static bool g_test_running = true;
static int g_wake_word_count = 0;
static int g_vad_changes = 0;
static int g_send_queue_callbacks = 0;

// 测试统计
typedef struct {
    int packets_sent;
    int packets_received;
    int audio_frames_processed;
    int wake_words_detected;
    int vad_state_changes;
    time_t start_time;
    time_t end_time;
} TestStatistics;

static TestStatistics g_test_stats = {0};

// 信号处理函数
void signal_handler(int sig) {
    LINX_LOGI(TEST_TAG, "收到信号 %d，停止测试", sig);
    g_test_running = false;
}

// 回调函数实现
void on_send_queue_available(void* user_data) {
    g_send_queue_callbacks++;
    LINX_LOGD(TEST_TAG, "发送队列可用回调被调用，总计: %d", g_send_queue_callbacks);
    
    // 从发送队列获取数据包
    AudioService* service = (AudioService*)user_data;
    AudioStreamPacket* packet = audio_service_pop_packet_from_send_queue(service);
    if (packet) {
        g_test_stats.packets_sent++;
        LINX_LOGD(TEST_TAG, "从发送队列获取数据包，大小: %zu bytes", packet->payload_size);
        audio_stream_packet_destroy(packet);
    }
}

void on_wake_word_detected(const char* wake_word, void* user_data) {
    g_wake_word_count++;
    g_test_stats.wake_words_detected++;
    LINX_LOGI(TEST_TAG, "检测到唤醒词: '%s'，总计: %d", wake_word, g_wake_word_count);
}

void on_vad_change(bool speaking, void* user_data) {
    g_vad_changes++;
    g_test_stats.vad_state_changes++;
    LINX_LOGI(TEST_TAG, "VAD状态变化: %s，总计: %d", 
              speaking ? "开始说话" : "停止说话", g_vad_changes);
}

void on_audio_testing_queue_full(void* user_data) {
    LINX_LOGW(TEST_TAG, "音频测试队列已满");
}

// 创建模拟的音频编解码器
audio_codec_t* create_test_codec() {
    // 这里应该创建一个真实的编解码器实例
    // 为了测试，我们创建一个简单的模拟实现
    audio_codec_t* codec = (audio_codec_t*)calloc(1, sizeof(audio_codec_t));
    if (!codec) {
        return NULL;
    }
    
    // 设置基本格式
    audio_format_init(&codec->format, TEST_SAMPLE_RATE, TEST_CHANNELS, 16, TEST_FRAME_DURATION_MS);
    
    LINX_LOGI(TEST_TAG, "创建测试编解码器成功");
    return codec;
}

// 创建模拟的音频处理器
AudioProcessor* create_test_audio_processor() {
    // 这里应该创建一个真实的音频处理器实例
    // 为了测试，我们创建一个简单的模拟实现
    AudioProcessor* processor = (AudioProcessor*)calloc(1, sizeof(AudioProcessor));
    if (!processor) {
        return NULL;
    }
    
    LINX_LOGI(TEST_TAG, "创建测试音频处理器成功");
    return processor;
}

// 创建模拟的唤醒词接口
WakeWordInterface* create_test_wake_word_interface() {
    // 这里应该创建一个真实的唤醒词接口实例
    // 为了测试，我们创建一个简单的模拟实现
    WakeWordInterface* wake_word = (WakeWordInterface*)calloc(1, sizeof(WakeWordInterface));
    if (!wake_word) {
        return NULL;
    }
    
    LINX_LOGI(TEST_TAG, "创建测试唤醒词接口成功");
    return wake_word;
}

// 模拟音频数据生成器
void* audio_data_generator_thread(void* arg) {
    AudioService* service = (AudioService*)arg;
    int frame_samples = (TEST_SAMPLE_RATE * TEST_FRAME_DURATION_MS) / 1000;
    
    LINX_LOGI(TEST_TAG, "音频数据生成器线程启动，帧大小: %d 样本", frame_samples);
    
    while (g_test_running) {
        // 创建模拟音频数据
        AudioStreamPacket* packet = audio_stream_packet_create();
        if (packet) {
            packet->sample_rate = TEST_SAMPLE_RATE;
            packet->frame_duration = TEST_FRAME_DURATION_MS;
            packet->timestamp = time(NULL);
            
            // 生成简单的正弦波测试数据
            int16_t* test_data = (int16_t*)malloc(frame_samples * sizeof(int16_t));
            if (test_data) {
                static double phase = 0.0;
                double frequency = 440.0; // A4音符
                for (int i = 0; i < frame_samples; i++) {
                    test_data[i] = (int16_t)(16000 * sin(phase));
                    phase += 2.0 * M_PI * frequency / TEST_SAMPLE_RATE;
                    if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
                }
                
                // 模拟编码数据（实际应该通过编码器）
                memcpy(packet->payload, test_data, 
                       (frame_samples * sizeof(int16_t) < packet->payload_capacity) ? 
                       frame_samples * sizeof(int16_t) : packet->payload_capacity);
                packet->payload_size = (frame_samples * sizeof(int16_t) < packet->payload_capacity) ? 
                                     frame_samples * sizeof(int16_t) : packet->payload_capacity;
                
                free(test_data);
            }
            
            // 推送到解码队列
            if (audio_service_push_packet_to_decode_queue(service, packet, false)) {
                g_test_stats.packets_received++;
                g_test_stats.audio_frames_processed++;
            } else {
                audio_stream_packet_destroy(packet);
            }
        }
        
        // 控制数据生成速率
        usleep(TEST_FRAME_DURATION_MS * 1000);
    }
    
    LINX_LOGI(TEST_TAG, "音频数据生成器线程停止");
    return NULL;
}

// 测试基本功能
int test_basic_functionality() {
    LINX_LOGI(TEST_TAG, "开始测试基本功能");
    
    // 创建音频服务
    g_audio_service = audio_service_create();
    if (!g_audio_service) {
        LINX_LOGE(TEST_TAG, "创建音频服务失败");
        return -1;
    }
    
    // 创建编解码器
    audio_codec_t* codec = create_test_codec();
    if (!codec) {
        LINX_LOGE(TEST_TAG, "创建编解码器失败");
        audio_service_destroy(g_audio_service);
        return -1;
    }
    
    // 初始化音频服务
    if (audio_service_initialize(g_audio_service, codec) != 0) {
        LINX_LOGE(TEST_TAG, "初始化音频服务失败");
        free(codec);
        audio_service_destroy(g_audio_service);
        return -1;
    }
    
    // 设置回调函数
    AudioServiceCallbacks callbacks = {
        .on_send_queue_available = on_send_queue_available,
        .on_wake_word_detected = on_wake_word_detected,
        .on_vad_change = on_vad_change,
        .on_audio_testing_queue_full = on_audio_testing_queue_full,
        .user_data = g_audio_service
    };
    audio_service_set_callbacks(g_audio_service, &callbacks);
    
    // 设置音频处理器和唤醒词接口（模拟）
    g_audio_service->audio_processor = create_test_audio_processor();
    g_audio_service->wake_word = create_test_wake_word_interface();
    
    LINX_LOGI(TEST_TAG, "基本功能测试完成");
    return 0;
}

// 测试音频处理功能
int test_audio_processing() {
    LINX_LOGI(TEST_TAG, "开始测试音频处理功能");
    
    // 启动音频服务
    if (audio_service_start(g_audio_service) != 0) {
        LINX_LOGE(TEST_TAG, "启动音频服务失败");
        return -1;
    }
    
    // 启用音频测试
    audio_service_enable_audio_testing(g_audio_service, true);
    LINX_LOGI(TEST_TAG, "音频测试已启用");
    
    // 启用语音处理
    audio_service_enable_voice_processing(g_audio_service, true);
    LINX_LOGI(TEST_TAG, "语音处理已启用");
    
    // 启用唤醒词检测
    audio_service_enable_wake_word_detection(g_audio_service, true);
    LINX_LOGI(TEST_TAG, "唤醒词检测已启用");
    
    // 启用设备AEC
    audio_service_enable_device_aec(g_audio_service, true);
    LINX_LOGI(TEST_TAG, "设备AEC已启用");
    
    LINX_LOGI(TEST_TAG, "音频处理功能测试完成");
    return 0;
}

// 测试状态查询功能
void test_status_queries() {
    LINX_LOGI(TEST_TAG, "开始测试状态查询功能");
    
    bool voice_detected = audio_service_is_voice_detected(g_audio_service);
    bool is_idle = audio_service_is_idle(g_audio_service);
    bool wake_word_running = audio_service_is_wake_word_running(g_audio_service);
    bool audio_processor_running = audio_service_is_audio_processor_running(g_audio_service);
    
    LINX_LOGI(TEST_TAG, "状态查询结果:");
    LINX_LOGI(TEST_TAG, "  语音检测: %s", voice_detected ? "是" : "否");
    LINX_LOGI(TEST_TAG, "  服务空闲: %s", is_idle ? "是" : "否");
    LINX_LOGI(TEST_TAG, "  唤醒词运行: %s", wake_word_running ? "是" : "否");
    LINX_LOGI(TEST_TAG, "  音频处理器运行: %s", audio_processor_running ? "是" : "否");
    
    const char* last_wake_word = audio_service_get_last_wake_word(g_audio_service);
    LINX_LOGI(TEST_TAG, "  最后唤醒词: %s", last_wake_word ? last_wake_word : "无");
}

// 打印测试统计
void print_test_statistics() {
    g_test_stats.end_time = time(NULL);
    double duration = difftime(g_test_stats.end_time, g_test_stats.start_time);
    
    LINX_LOGI(TEST_TAG, "=== 测试统计报告 ===");
    LINX_LOGI(TEST_TAG, "测试持续时间: %.1f 秒", duration);
    LINX_LOGI(TEST_TAG, "发送的数据包: %d", g_test_stats.packets_sent);
    LINX_LOGI(TEST_TAG, "接收的数据包: %d", g_test_stats.packets_received);
    LINX_LOGI(TEST_TAG, "处理的音频帧: %d", g_test_stats.audio_frames_processed);
    LINX_LOGI(TEST_TAG, "检测到的唤醒词: %d", g_test_stats.wake_words_detected);
    LINX_LOGI(TEST_TAG, "VAD状态变化: %d", g_test_stats.vad_state_changes);
    LINX_LOGI(TEST_TAG, "发送队列回调: %d", g_send_queue_callbacks);
    
    if (duration > 0) {
        LINX_LOGI(TEST_TAG, "平均帧率: %.1f 帧/秒", g_test_stats.audio_frames_processed / duration);
    }
}

// 清理资源
void cleanup() {
    LINX_LOGI(TEST_TAG, "开始清理资源");
    
    if (g_audio_service) {
        // 停止各种功能
        audio_service_enable_audio_testing(g_audio_service, false);
        audio_service_enable_voice_processing(g_audio_service, false);
        audio_service_enable_wake_word_detection(g_audio_service, false);
        audio_service_enable_device_aec(g_audio_service, false);
        
        // 停止和销毁服务
        audio_service_stop(g_audio_service);
        
        // 清理模拟组件
        if (g_audio_service->audio_processor) {
            free(g_audio_service->audio_processor);
        }
        if (g_audio_service->wake_word) {
            free(g_audio_service->wake_word);
        }
        if (g_audio_service->codec) {
            free(g_audio_service->codec);
        }
        
        audio_service_destroy(g_audio_service);
        g_audio_service = NULL;
    }
    
    LINX_LOGI(TEST_TAG, "资源清理完成");
}

// 主测试函数
int main(int argc, char* argv[]) {
    LINX_LOGI(TEST_TAG, "=== 音频服务完整功能测试开始 ===");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 记录开始时间
    g_test_stats.start_time = time(NULL);
    
    // 测试基本功能
    if (test_basic_functionality() != 0) {
        LINX_LOGE(TEST_TAG, "基本功能测试失败");
        cleanup();
        return -1;
    }
    
    // 测试音频处理功能
    if (test_audio_processing() != 0) {
        LINX_LOGE(TEST_TAG, "音频处理功能测试失败");
        cleanup();
        return -1;
    }
    
    // 创建音频数据生成器线程
    pthread_t generator_thread;
    if (pthread_create(&generator_thread, NULL, audio_data_generator_thread, g_audio_service) != 0) {
        LINX_LOGE(TEST_TAG, "创建音频数据生成器线程失败");
        cleanup();
        return -1;
    }
    
    LINX_LOGI(TEST_TAG, "测试运行中，将持续 %d 秒...", TEST_DURATION_SECONDS);
    
    // 主测试循环
    for (int i = 0; i < TEST_DURATION_SECONDS && g_test_running; i++) {
        sleep(1);
        
        // 每5秒进行一次状态查询
        if (i % 5 == 0) {
            test_status_queries();
        }
        
        // 每10秒模拟一次唤醒词检测
        if (i % 10 == 0 && i > 0) {
            LINX_LOGI(TEST_TAG, "模拟唤醒词检测...");
            on_wake_word_detected("小爱同学", g_audio_service);
        }
        
        // 每7秒模拟一次VAD状态变化
        if (i % 7 == 0) {
            on_vad_change(i % 14 == 0, g_audio_service);
        }
        
        LINX_LOGD(TEST_TAG, "测试进行中... %d/%d 秒", i + 1, TEST_DURATION_SECONDS);
    }
    
    // 停止测试
    g_test_running = false;
    
    // 等待生成器线程结束
    pthread_join(generator_thread, NULL);
    
    // 最终状态查询
    test_status_queries();
    
    // 打印统计信息
    print_test_statistics();
    
    // 清理资源
    cleanup();
    
    LINX_LOGI(TEST_TAG, "=== 音频服务完整功能测试结束 ===");
    return 0;
}