/**
 * @file simple_audio_test.c
 * @brief 音频服务简化测试demo
 * @details 专注于测试音频服务的核心功能，不依赖复杂的外部组件
 */

#include "../audio_service.h"
#include "../../common/log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#define SIMPLE_TEST_TAG "SimpleAudioTest"

// 全局变量
static AudioService* g_service = NULL;
static bool g_running = true;
static int g_callback_count = 0;

// 信号处理
void signal_handler(int sig) {
    printf("收到信号 %d，停止测试\n", sig);
    g_running = false;
}

// 简单的回调函数
void simple_send_queue_callback(void* user_data) {
    g_callback_count++;
    printf("发送队列回调被调用，次数: %d\n", g_callback_count);
}

void simple_wake_word_callback(const char* wake_word, void* user_data) {
    printf("检测到唤醒词: %s\n", wake_word ? wake_word : "NULL");
}

void simple_vad_callback(bool speaking, void* user_data) {
    printf("VAD状态变化: %s\n", speaking ? "说话中" : "静音");
}

// 创建简单的模拟编解码器
audio_codec_t* create_simple_codec() {
    audio_codec_t* codec = (audio_codec_t*)calloc(1, sizeof(audio_codec_t));
    if (!codec) {
        return NULL;
    }
    
    // 设置基本格式
    codec->format.sample_rate = 16000;
    codec->format.channels = 1;
    codec->format.bits_per_sample = 16;
    codec->format.frame_size_ms = 60;
    
    printf("创建简单编解码器成功\n");
    return codec;
}

// 测试基本创建和销毁
int test_basic_creation() {
    printf("\n=== 测试基本创建和销毁 ===\n");
    
    // 创建音频服务
    AudioService* service = audio_service_create();
    if (!service) {
        printf("❌ 创建音频服务失败\n");
        return -1;
    }
    printf("✅ 创建音频服务成功\n");
    
    // 创建编解码器
    audio_codec_t* codec = create_simple_codec();
    if (!codec) {
        printf("❌ 创建编解码器失败\n");
        audio_service_destroy(service);
        return -1;
    }
    printf("✅ 创建编解码器成功\n");
    
    // 初始化音频服务
    int result = audio_service_initialize(service, codec);
    if (result != 0) {
        printf("❌ 初始化音频服务失败，错误码: %d\n", result);
        free(codec);
        audio_service_destroy(service);
        return -1;
    }
    printf("✅ 初始化音频服务成功\n");
    
    // 销毁服务
    audio_service_destroy(service);
    free(codec);
    printf("✅ 销毁音频服务成功\n");
    
    return 0;
}

// 测试回调设置
int test_callbacks() {
    printf("\n=== 测试回调设置 ===\n");
    
    AudioService* service = audio_service_create();
    if (!service) {
        printf("❌ 创建音频服务失败\n");
        return -1;
    }
    
    // 设置回调
    AudioServiceCallbacks callbacks = {
        .on_send_queue_available = simple_send_queue_callback,
        .on_wake_word_detected = simple_wake_word_callback,
        .on_vad_change = simple_vad_callback,
        .user_data = service
    };
    
    audio_service_set_callbacks(service, &callbacks);
    printf("✅ 设置回调函数成功\n");
    
    // 测试回调调用（模拟）
    if (callbacks.on_wake_word_detected) {
        callbacks.on_wake_word_detected("测试唤醒词", service);
        printf("✅ 唤醒词回调测试成功\n");
    }
    
    if (callbacks.on_vad_change) {
        callbacks.on_vad_change(true, service);
        callbacks.on_vad_change(false, service);
        printf("✅ VAD回调测试成功\n");
    }
    
    audio_service_destroy(service);
    return 0;
}

// 测试状态查询
int test_status_queries() {
    printf("\n=== 测试状态查询 ===\n");
    
    AudioService* service = audio_service_create();
    if (!service) {
        printf("❌ 创建音频服务失败\n");
        return -1;
    }
    
    audio_codec_t* codec = create_simple_codec();
    if (!codec || audio_service_initialize(service, codec) != 0) {
        printf("❌ 初始化失败\n");
        if (codec) free(codec);
        audio_service_destroy(service);
        return -1;
    }
    
    // 查询各种状态
    bool voice_detected = audio_service_is_voice_detected(service);
    bool is_idle = audio_service_is_idle(service);
    bool wake_word_running = audio_service_is_wake_word_running(service);
    bool audio_processor_running = audio_service_is_audio_processor_running(service);
    
    printf("状态查询结果:\n");
    printf("  语音检测: %s\n", voice_detected ? "是" : "否");
    printf("  服务空闲: %s\n", is_idle ? "是" : "否");
    printf("  唤醒词运行: %s\n", wake_word_running ? "是" : "否");
    printf("  音频处理器运行: %s\n", audio_processor_running ? "是" : "否");
    
    const char* last_wake_word = audio_service_get_last_wake_word(service);
    printf("  最后唤醒词: %s\n", last_wake_word ? last_wake_word : "无");
    
    printf("✅ 状态查询测试成功\n");
    
    free(codec);
    audio_service_destroy(service);
    return 0;
}

// 测试数据包队列操作
int test_packet_queue_operations() {
    printf("\n=== 测试数据包队列操作 ===\n");
    
    AudioService* service = audio_service_create();
    if (!service) {
        printf("❌ 创建音频服务失败\n");
        return -1;
    }
    
    audio_codec_t* codec = create_simple_codec();
    if (!codec || audio_service_initialize(service, codec) != 0) {
        printf("❌ 初始化失败\n");
        if (codec) free(codec);
        audio_service_destroy(service);
        return -1;
    }
    
    // 创建测试数据包
    AudioStreamPacket* packet = audio_stream_packet_create();
    if (!packet) {
        printf("❌ 创建数据包失败\n");
        free(codec);
        audio_service_destroy(service);
        return -1;
    }
    
    // 设置数据包属性
    packet->sample_rate = 16000;
    packet->frame_duration = 60;
    packet->timestamp = time(NULL);
    
    // 添加一些测试数据
    const char* test_data = "这是测试音频数据";
    size_t data_len = strlen(test_data);
    if (data_len < packet->payload_capacity) {
        memcpy(packet->payload, test_data, data_len);
        packet->payload_size = data_len;
    }
    
    printf("✅ 创建测试数据包成功，大小: %zu bytes\n", packet->payload_size);
    
    // 测试推送到解码队列
    bool push_result = audio_service_push_packet_to_decode_queue(service, packet, false);
    if (push_result) {
        printf("✅ 推送数据包到解码队列成功\n");
    } else {
        printf("⚠️  推送数据包到解码队列失败（可能是队列满或服务未启动）\n");
        audio_stream_packet_destroy(packet);
    }
    
    // 测试从发送队列弹出（应该为空）
    AudioStreamPacket* popped = audio_service_pop_packet_from_send_queue(service);
    if (popped) {
        printf("✅ 从发送队列弹出数据包，大小: %zu bytes\n", popped->payload_size);
        audio_stream_packet_destroy(popped);
    } else {
        printf("ℹ️  发送队列为空（正常情况）\n");
    }
    
    printf("✅ 数据包队列操作测试完成\n");
    
    free(codec);
    audio_service_destroy(service);
    return 0;
}

// 测试启动和停止
int test_start_stop() {
    printf("\n=== 测试启动和停止 ===\n");
    
    AudioService* service = audio_service_create();
    if (!service) {
        printf("❌ 创建音频服务失败\n");
        return -1;
    }
    
    audio_codec_t* codec = create_simple_codec();
    if (!codec || audio_service_initialize(service, codec) != 0) {
        printf("❌ 初始化失败\n");
        if (codec) free(codec);
        audio_service_destroy(service);
        return -1;
    }
    
    // 启动服务
    int start_result = audio_service_start(service);
    if (start_result == 0) {
        printf("✅ 启动音频服务成功\n");
        
        // 运行一小段时间
        printf("服务运行中...\n");
        sleep(2);
        
        // 停止服务
        audio_service_stop(service);
        printf("✅ 停止音频服务成功\n");
    } else {
        printf("❌ 启动音频服务失败，错误码: %d\n", start_result);
    }
    
    free(codec);
    audio_service_destroy(service);
    return 0;
}

// 主函数
int main(int argc, char* argv[]) {
    printf("=== 音频服务简化测试开始 ===\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int failed_tests = 0;
    
    // 运行各项测试
    if (test_basic_creation() != 0) {
        failed_tests++;
    }
    
    if (test_callbacks() != 0) {
        failed_tests++;
    }
    
    if (test_status_queries() != 0) {
        failed_tests++;
    }
    
    if (test_packet_queue_operations() != 0) {
        failed_tests++;
    }
    
    if (test_start_stop() != 0) {
        failed_tests++;
    }
    
    // 输出测试结果
    printf("\n=== 测试结果汇总 ===\n");
    if (failed_tests == 0) {
        printf("🎉 所有测试通过！\n");
    } else {
        printf("❌ %d 个测试失败\n", failed_tests);
    }
    
    printf("=== 音频服务简化测试结束 ===\n");
    return failed_tests;
}