/**
 * @file simple_playback.c
 * @brief 简单音频播放示例
 * @details 演示如何使用LinxOS音频系统播放正弦波
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include "../core/audio_manager.h"
#include "../drivers/audio_driver.h"
#include "../pipeline/audio_pipeline.h"
#include "../plugins/builtin/builtin_plugins.h"
#include "../utils/audio_utils.h"

// ============================================================================
// 全局变量
// ============================================================================

static volatile bool g_running = true;
static linx_audio_manager_t* g_audio_manager = NULL;
static linx_audio_pipeline_t* g_pipeline = NULL;
static float g_phase = 0.0f;

// ============================================================================
// 信号处理
// ============================================================================

static void signal_handler(int sig) {
    (void)sig;
    printf("\n收到退出信号，正在停止...\n");
    g_running = false;
}

// ============================================================================
// 音频回调函数
// ============================================================================

static void output_callback(float* buffer, size_t frame_count, void* user_data) {
    (void)user_data;
    
    // 生成440Hz正弦波
    const float frequency = 440.0f;
    const float sample_rate = 44100.0f;
    const float amplitude = 0.3f;
    
    for (size_t i = 0; i < frame_count; i++) {
        float sample = amplitude * sinf(g_phase);
        
        // 立体声输出
        buffer[i * 2] = sample;     // 左声道
        buffer[i * 2 + 1] = sample; // 右声道
        
        // 更新相位
        g_phase += 2.0f * M_PI * frequency / sample_rate;
        if (g_phase >= 2.0f * M_PI) {
            g_phase -= 2.0f * M_PI;
        }
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    printf("LinxOS音频系统 - 简单播放示例\n");
    printf("================================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    linx_audio_result_t result;
    
    // 1. 初始化音频管理器
    printf("1. 初始化音频管理器...\n");
    
    linx_audio_manager_config_t manager_config = {0};
    manager_config.max_pipelines = 4;
    manager_config.max_plugins = 16;
    manager_config.event_queue_size = 256;
    manager_config.thread_priority = LINX_AUDIO_THREAD_PRIORITY_HIGH;
    
    g_audio_manager = linx_audio_manager_create(&manager_config);
    if (!g_audio_manager) {
        fprintf(stderr, "错误: 无法创建音频管理器\n");
        return 1;
    }
    
    result = linx_audio_manager_initialize(g_audio_manager);
    if (result != LINX_AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 音频管理器初始化失败: %d\n", result);
        goto cleanup;
    }
    
    // 2. 创建音频驱动
    printf("2. 创建音频驱动...\n");
    
    linx_audio_driver_t* driver = linx_audio_driver_create_default();
    if (!driver) {
        fprintf(stderr, "错误: 无法创建音频驱动\n");
        goto cleanup;
    }
    
    // 配置驱动
    linx_audio_driver_config_t driver_config;
    linx_audio_driver_get_default_config(&driver_config);
    
    driver_config.format.sample_rate = 44100;
    driver_config.format.channels = 2;
    driver_config.format.format = LINX_AUDIO_FORMAT_FLOAT32;
    driver_config.format.channel_layout = LINX_AUDIO_CHANNEL_LAYOUT_STEREO;
    driver_config.buffer_size = 512;
    driver_config.buffer_count = 4;
    driver_config.enable_output = true;
    driver_config.enable_input = false;
    
    result = driver->vtable->initialize(driver, &driver_config);
    if (result != LINX_AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 音频驱动初始化失败: %d\n", result);
        linx_audio_driver_destroy(driver);
        goto cleanup;
    }
    
    // 设置输出回调
    result = driver->vtable->set_output_callback(driver, output_callback, NULL);
    if (result != LINX_AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 设置输出回调失败: %d\n", result);
        linx_audio_driver_destroy(driver);
        goto cleanup;
    }
    
    // 3. 创建音频管道
    printf("3. 创建音频管道...\n");
    
    linx_audio_pipeline_config_t pipeline_config = {0};
    pipeline_config.max_nodes = 8;
    pipeline_config.max_connections = 16;
    pipeline_config.processing_mode = LINX_AUDIO_PROCESSING_MODE_REALTIME;
    pipeline_config.thread_priority = LINX_AUDIO_THREAD_PRIORITY_HIGH;
    
    g_pipeline = linx_audio_pipeline_create(&pipeline_config);
    if (!g_pipeline) {
        fprintf(stderr, "错误: 无法创建音频管道\n");
        linx_audio_driver_destroy(driver);
        goto cleanup;
    }
    
    // 4. 初始化内置插件系统
    printf("4. 初始化插件系统...\n");
    
    result = linx_builtin_plugins_initialize();
    if (result != LINX_AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 插件系统初始化失败: %d\n", result);
        linx_audio_driver_destroy(driver);
        goto cleanup;
    }
    
    // 5. 启动音频驱动
    printf("5. 启动音频播放...\n");
    
    result = driver->vtable->start(driver);
    if (result != LINX_AUDIO_SUCCESS) {
        fprintf(stderr, "错误: 启动音频驱动失败: %d\n", result);
        linx_audio_driver_destroy(driver);
        goto cleanup;
    }
    
    printf("\n正在播放440Hz正弦波...\n");
    printf("按Ctrl+C停止播放\n\n");
    
    // 6. 主循环
    while (g_running) {
        // 显示统计信息
        linx_audio_driver_stats_t stats;
        if (driver->vtable->get_stats(driver, &stats) == LINX_AUDIO_SUCCESS) {
            printf("\r回调次数: %lu, 欠载次数: %lu", 
                   stats.callback_count, stats.underrun_count);
            fflush(stdout);
        }
        
        usleep(100000); // 100ms
    }
    
    printf("\n\n6. 停止音频播放...\n");
    
    // 停止驱动
    driver->vtable->stop(driver);
    
    // 清理资源
    linx_audio_driver_destroy(driver);
    linx_builtin_plugins_cleanup();
    
cleanup:
    if (g_pipeline) {
        linx_audio_pipeline_destroy(g_pipeline);
    }
    
    if (g_audio_manager) {
        linx_audio_manager_deinitialize(g_audio_manager);
        linx_audio_manager_destroy(g_audio_manager);
    }
    
    printf("示例程序结束\n");
    return 0;
}