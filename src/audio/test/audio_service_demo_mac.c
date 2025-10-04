/**
 * @file audio_service_demo_mac.c
 * @brief AudioService 最佳使用流程演示 (Mac平台)
 * @details 展示 AudioService 的完整生命周期和各种功能特性
 * 
 * 演示内容：
 * 1. AudioService 标准初始化流程
 * 2. 组件设置和功能配置
 * 3. 音频录制、处理、编解码、播放完整流程
 * 4. VAD (语音活动检测) 功能演示
 * 5. 唤醒词检测功能演示
 * 6. 音频测试模式演示
 * 7. 错误处理和资源清理
 * 
 * 使用方法：
 * ./audio_service_demo_mac [选项]
 *   --mode <mode>     演示模式: basic|vad|wakeword|test|full (默认: basic)
 *   --duration <sec>  运行时长，秒 (默认: 10)
 *   --real-audio      使用真实音频设备 (默认: 模拟音频)
 *   --help           显示帮助信息
 * 
 * @author AudioService Team
 * @version 1.0
 */

#include "../audio_service.h"
#include "../audio_packet_queue.h"
#include "../../common/log/linx_log.h"

// Mac平台特定组件 (条件编译)
#ifdef __APPLE__
#include "../../../board/mac/common/audio/audio/portaudio_mac.h"
#include "../../../board/mac/common/audio/processor/audio_processor_mac.h"
// 注意：使用标准的唤醒词接口，而不是平台特定的实现
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>

// ============================================================================
// 常量定义
// ============================================================================

#define DEMO_TAG "AudioServiceDemo"
#define DEFAULT_DURATION_SEC 10
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_DURATION_MS 60
#define FRAME_SIZE ((SAMPLE_RATE * FRAME_DURATION_MS) / 1000)

// ============================================================================
// 演示模式枚举
// ============================================================================

typedef enum {
    DEMO_MODE_BASIC = 0,    /**< 基础音频录制播放 */
    DEMO_MODE_VAD,          /**< VAD语音活动检测 */
    DEMO_MODE_WAKEWORD,     /**< 唤醒词检测 */
    DEMO_MODE_TEST,         /**< 音频测试模式 */
    DEMO_MODE_FULL          /**< 完整功能演示 */
} DemoMode;

// ============================================================================
// 全局变量
// ============================================================================

static AudioService* g_audio_service = NULL;
static AudioInterface* g_audio_interface = NULL;
static AudioProcessor* g_audio_processor = NULL;
static WakeWordInterface* g_wake_word_interface = NULL;
static audio_codec_t* g_opus_encoder = NULL;
static audio_codec_t* g_opus_decoder = NULL;
static bool g_demo_running = false;
static bool g_use_real_audio = false;
static bool g_enable_wakeword = true;  // 默认启用唤醒词
static DemoMode g_demo_mode = DEMO_MODE_BASIC;
static int g_duration_sec = DEFAULT_DURATION_SEC;

// 统计信息
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t vad_speech_events;
    uint32_t vad_silence_events;
    uint32_t wake_word_detections;
    time_t start_time;
} DemoStatistics;

static DemoStatistics g_stats = {0};

// ============================================================================
// 信号处理
// ============================================================================

void signal_handler(int sig) {
    LINX_LOGI(DEMO_TAG, "收到信号 %d，正在停止演示...", sig);
    g_demo_running = false;
}

// ============================================================================
// AudioService 回调函数
// ============================================================================

/**
 * @brief 发送队列可用回调
 */
void on_send_queue_available(void* user_data) {
    LINX_LOGD(DEMO_TAG, "发送队列有新数据可用");
    
    // 从发送队列获取编码后的音频数据
    AudioStreamPacket* packet = audio_service_pop_packet_from_send_queue(g_audio_service);
    if (packet) {
        g_stats.packets_sent++;
        LINX_LOGI(DEMO_TAG, "获取到编码数据包，大小: %zu 字节 (总计: %u)", 
                  packet->payload_size, g_stats.packets_sent);
        
        // 在实际应用中，这里会将数据发送到网络
        // 为了演示，我们将数据放回解码队列进行本地回放
        if (g_demo_mode == DEMO_MODE_BASIC || g_demo_mode == DEMO_MODE_FULL) {
            if (audio_service_push_packet_to_decode_queue(g_audio_service, packet, false)) {
                g_stats.packets_received++;
                LINX_LOGD(DEMO_TAG, "数据包已放入解码队列进行回放");
            } else {
                LINX_LOGW(DEMO_TAG, "解码队列已满，丢弃数据包");
                audio_stream_packet_destroy(packet);
            }
        } else {
            audio_stream_packet_destroy(packet);
        }
    }
}

/**
 * @brief 唤醒词检测回调
 */
void on_wake_word_detected(const char* wake_word, void* user_data) {
    g_stats.wake_word_detections++;
    LINX_LOGI(DEMO_TAG, "🎯 检测到唤醒词: '%s' (总计: %u次)", 
              wake_word, g_stats.wake_word_detections);
}

/**
 * @brief VAD状态变化回调
 */
void on_vad_change(bool speaking, void* user_data) {
    if (speaking) {
        g_stats.vad_speech_events++;
        LINX_LOGI(DEMO_TAG, "🗣️  检测到语音活动 (语音事件: %u)", g_stats.vad_speech_events);
    } else {
        g_stats.vad_silence_events++;
        LINX_LOGI(DEMO_TAG, "🔇 语音活动结束 (静音事件: %u)", g_stats.vad_silence_events);
    }
}

/**
 * @brief 音频测试队列满回调
 */
void on_audio_testing_queue_full(void* user_data) {
    LINX_LOGI(DEMO_TAG, "📊 音频测试完成，队列已满");
}

// ============================================================================
// 组件创建函数
// ============================================================================

/**
 * @brief 创建测试用的音频编解码器
 */
audio_codec_t* create_test_codec(bool is_encoder) {
    // 这里应该创建真实的Opus编解码器
    // 为了演示，我们创建一个简单的测试编解码器
    audio_codec_t* codec = (audio_codec_t*)calloc(1, sizeof(audio_codec_t));
    if (!codec) {
        return NULL;
    }
    
    // 初始化编解码器配置
    audio_format_default(&codec->format);
    codec->format.sample_rate = SAMPLE_RATE;
    codec->format.channels = CHANNELS;
    
    LINX_LOGI(DEMO_TAG, "创建%s成功", is_encoder ? "编码器" : "解码器");
    return codec;
}

#ifdef __APPLE__
/**
 * @brief 创建Mac平台音频接口
 */
AudioInterface* create_mac_audio_interface() {
    if (!g_use_real_audio) {
        LINX_LOGI(DEMO_TAG, "使用模拟音频接口");
        return NULL; // 使用模拟音频
    }
    
    // 创建PortAudio接口
    AudioInterface* interface = portaudio_mac_create();
    if (!interface) {
        LINX_LOGE(DEMO_TAG, "创建PortAudio接口失败");
        return NULL;
    }
    
    LINX_LOGI(DEMO_TAG, "创建PortAudio接口成功");
    return interface;
}

/**
 * @brief 创建Mac平台音频处理器
 */
AudioProcessor* create_mac_audio_processor() {
    AudioProcessor* processor = audio_processor_mac_create();
    if (!processor) {
        LINX_LOGE(DEMO_TAG, "创建Mac音频处理器失败");
        return NULL;
    }
    
    LINX_LOGI(DEMO_TAG, "创建Mac音频处理器成功");
    return processor;
}

/**
 * @brief 创建Mac平台唤醒词接口
 */
WakeWordInterface* create_mac_wake_word_interface() {
    // 检查是否设置了Picovoice访问密钥
    const char* access_key = getenv("PICOVOICE_ACCESS_KEY");
    if (!access_key) {
        LINX_LOGW(DEMO_TAG, "未设置PICOVOICE_ACCESS_KEY环境变量");
        LINX_LOGW(DEMO_TAG, "唤醒词功能需要有效的Picovoice访问密钥");
        LINX_LOGW(DEMO_TAG, "请访问 https://picovoice.ai/ 获取访问密钥");
        return NULL;
    }
    
    // 在实际实现中，这里应该调用平台特定的唤醒词创建函数
    // 例如：wake_word_porcupine_create()
    // 为了演示目的，我们创建一个简单的模拟接口
    
    LINX_LOGW(DEMO_TAG, "唤醒词功能暂时使用模拟实现");
    LINX_LOGW(DEMO_TAG, "要启用真实的Porcupine唤醒词检测，请：");
    LINX_LOGW(DEMO_TAG, "1. 确保已安装Picovoice Porcupine库");
    LINX_LOGW(DEMO_TAG, "2. 在编译时链接Porcupine库");
    LINX_LOGW(DEMO_TAG, "3. 取消注释Porcupine特定的代码");
    
    // 返回NULL表示唤醒词功能不可用
    // 在完整实现中，这里应该返回真实的WakeWordInterface实例
    return NULL;
}
#endif

// ============================================================================
// 演示模式实现
// ============================================================================

/**
 * @brief 基础模式：音频录制和播放
 */
int demo_basic_mode() {
    LINX_LOGI(DEMO_TAG, "🎵 启动基础音频录制播放演示");
    
    AudioServiceFeatures features = {0};
    features.voice_processing = true;
    
    if (audio_service_configure_features(g_audio_service, &features) != 0) {
        LINX_LOGE(DEMO_TAG, "配置基础功能失败");
        return -1;
    }
    
    LINX_LOGI(DEMO_TAG, "基础模式配置完成，开始录制和播放...");
    return 0;
}

/**
 * @brief VAD模式：语音活动检测
 */
int demo_vad_mode() {
    LINX_LOGI(DEMO_TAG, "🗣️  启动VAD语音活动检测演示");
    
    AudioServiceFeatures features = {0};
    features.voice_processing = true;
    features.voice_activity_detection = true;
    
    if (audio_service_configure_features(g_audio_service, &features) != 0) {
        LINX_LOGE(DEMO_TAG, "配置VAD功能失败");
        return -1;
    }
    
    LINX_LOGI(DEMO_TAG, "VAD模式配置完成，请说话测试语音检测...");
    return 0;
}

/**
 * @brief 唤醒词模式：唤醒词检测
 */
int demo_wakeword_mode() {
    LINX_LOGI(DEMO_TAG, "🎯 启动唤醒词检测演示");
    
    AudioServiceFeatures features = {0};
    features.wake_word_detection = true;
    
    if (audio_service_configure_features(g_audio_service, &features) != 0) {
        LINX_LOGE(DEMO_TAG, "配置唤醒词功能失败");
        return -1;
    }
    
    LINX_LOGI(DEMO_TAG, "唤醒词模式配置完成，请说出唤醒词...");
    return 0;
}

/**
 * @brief 测试模式：音频测试
 */
int demo_test_mode() {
    LINX_LOGI(DEMO_TAG, "📊 启动音频测试演示");
    
    AudioServiceFeatures features = {0};
    features.audio_testing = true;
    
    if (audio_service_configure_features(g_audio_service, &features) != 0) {
        LINX_LOGE(DEMO_TAG, "配置测试功能失败");
        return -1;
    }
    
    LINX_LOGI(DEMO_TAG, "测试模式配置完成，正在收集音频数据...");
    return 0;
}

/**
 * @brief 完整模式：所有功能
 */
int demo_full_mode() {
    LINX_LOGI(DEMO_TAG, "🚀 启动完整功能演示");
    
    AudioServiceFeatures features = {0};
    features.voice_processing = true;
    features.voice_activity_detection = true;
    features.wake_word_detection = true;
    features.device_aec = true;
    features.noise_suppression = true;
    
    if (audio_service_configure_features(g_audio_service, &features) != 0) {
        LINX_LOGE(DEMO_TAG, "配置完整功能失败");
        return -1;
    }
    
    LINX_LOGI(DEMO_TAG, "完整模式配置完成，所有功能已启用...");
    return 0;
}

// ============================================================================
// 主要功能函数
// ============================================================================

/**
 * @brief 初始化AudioService
 */
int setup_audio_service() {
    LINX_LOGI(DEMO_TAG, "🔧 开始初始化AudioService...");
    
    // 1. 创建AudioService实例
    AudioServiceConfig config;
    audio_service_config_init_default(&config);
    
    g_audio_service = audio_service_create(&config);
    if (!g_audio_service) {
        LINX_LOGE(DEMO_TAG, "创建AudioService失败");
        return -1;
    }
    LINX_LOGI(DEMO_TAG, "✅ AudioService实例创建成功");
    
    // 2. 创建组件
#ifdef __APPLE__
    g_audio_interface = create_mac_audio_interface();
    g_audio_processor = create_mac_audio_processor();
    
    // 根据配置决定是否创建唤醒词接口
    if (g_enable_wakeword) {
        g_wake_word_interface = create_mac_wake_word_interface();
        if (!g_wake_word_interface) {
            LINX_LOGW(DEMO_TAG, "唤醒词接口创建失败，将禁用唤醒词功能");
            g_enable_wakeword = false;
        }
    } else {
        LINX_LOGI(DEMO_TAG, "唤醒词功能已禁用");
    }
#endif
    
    g_opus_encoder = create_test_codec(true);
    g_opus_decoder = create_test_codec(false);
    
    if (!g_opus_encoder || !g_opus_decoder) {
        LINX_LOGE(DEMO_TAG, "创建编解码器失败");
        return -1;
    }
    LINX_LOGI(DEMO_TAG, "✅ 音频组件创建成功");
    
    // 3. 设置组件
    audio_service_set_components(g_audio_service,
                                g_audio_interface,
                                g_audio_processor,
                                g_wake_word_interface, // 使用真实的唤醒词接口
                                g_opus_encoder,
                                g_opus_decoder);
    LINX_LOGI(DEMO_TAG, "✅ 音频组件设置完成");
    
    // 4. 设置回调函数
    AudioServiceCallbacks callbacks = {
        .on_send_queue_available = on_send_queue_available,
        .on_wake_word_detected = on_wake_word_detected,
        .on_vad_change = on_vad_change,
        .on_audio_testing_queue_full = on_audio_testing_queue_full,
        .user_data = NULL
    };
    audio_service_set_callbacks(g_audio_service, &callbacks);
    LINX_LOGI(DEMO_TAG, "✅ 回调函数设置完成");
    
    // 5. 初始化服务
    if (audio_service_initialize(g_audio_service, g_opus_encoder) != 0) {
        LINX_LOGE(DEMO_TAG, "初始化AudioService失败");
        return -1;
    }
    LINX_LOGI(DEMO_TAG, "✅ AudioService初始化完成");
    
    return 0;
}

/**
 * @brief 运行演示
 */
int run_demo() {
    LINX_LOGI(DEMO_TAG, "🎬 开始运行演示...");
    
    // 根据模式配置功能
    int result = 0;
    switch (g_demo_mode) {
        case DEMO_MODE_BASIC:
            result = demo_basic_mode();
            break;
        case DEMO_MODE_VAD:
            result = demo_vad_mode();
            break;
        case DEMO_MODE_WAKEWORD:
            result = demo_wakeword_mode();
            break;
        case DEMO_MODE_TEST:
            result = demo_test_mode();
            break;
        case DEMO_MODE_FULL:
            result = demo_full_mode();
            break;
        default:
            LINX_LOGE(DEMO_TAG, "未知的演示模式: %d", g_demo_mode);
            return -1;
    }
    
    if (result != 0) {
        return result;
    }
    
    // 启动AudioService
    if (audio_service_start(g_audio_service) != 0) {
        LINX_LOGE(DEMO_TAG, "启动AudioService失败");
        return -1;
    }
    LINX_LOGI(DEMO_TAG, "✅ AudioService启动成功");
    
    // 记录开始时间
    g_stats.start_time = time(NULL);
    g_demo_running = true;
    
    // 主循环
    LINX_LOGI(DEMO_TAG, "演示运行中，持续时间: %d秒...", g_duration_sec);
    for (int i = 0; i < g_duration_sec && g_demo_running; i++) {
        sleep(1);
        
        // 每5秒打印一次状态
        if ((i + 1) % 5 == 0) {
            LINX_LOGI(DEMO_TAG, "运行状态 - 已发送: %u, 已接收: %u, VAD语音: %u, 唤醒词: %u",
                      g_stats.packets_sent, g_stats.packets_received,
                      g_stats.vad_speech_events, g_stats.wake_word_detections);
        }
    }
    
    LINX_LOGI(DEMO_TAG, "演示运行完成");
    return 0;
}

/**
 * @brief 打印最终统计信息
 */
void print_final_statistics() {
    time_t end_time = time(NULL);
    double duration = difftime(end_time, g_stats.start_time);
    
    LINX_LOGI(DEMO_TAG, "\n📊 演示统计信息:");
    LINX_LOGI(DEMO_TAG, "运行时长: %.1f 秒", duration);
    LINX_LOGI(DEMO_TAG, "发送数据包: %u", g_stats.packets_sent);
    LINX_LOGI(DEMO_TAG, "接收数据包: %u", g_stats.packets_received);
    LINX_LOGI(DEMO_TAG, "VAD语音事件: %u", g_stats.vad_speech_events);
    LINX_LOGI(DEMO_TAG, "VAD静音事件: %u", g_stats.vad_silence_events);
    LINX_LOGI(DEMO_TAG, "唤醒词检测: %u", g_stats.wake_word_detections);
    
    if (duration > 0) {
        LINX_LOGI(DEMO_TAG, "平均数据包速率: %.1f 包/秒", g_stats.packets_sent / duration);
    }
}

/**
 * @brief 清理资源
 */
void cleanup_resources() {
    LINX_LOGI(DEMO_TAG, "🧹 开始清理资源...");
    
    if (g_audio_service) {
        audio_service_stop(g_audio_service);
        audio_service_destroy(g_audio_service);
        g_audio_service = NULL;
        LINX_LOGI(DEMO_TAG, "✅ AudioService已清理");
    }
    
    if (g_audio_interface) {
        // 清理音频接口
        g_audio_interface = NULL;
    }
    
    if (g_audio_processor) {
        // 清理音频处理器
        g_audio_processor = NULL;
    }
    
    if (g_wake_word_interface) {
        // 清理唤醒词接口
        if (g_wake_word_interface->vtable && g_wake_word_interface->vtable->destroy) {
            g_wake_word_interface->vtable->destroy(g_wake_word_interface);
        }
        g_wake_word_interface = NULL;
        LINX_LOGI(DEMO_TAG, "✅ 唤醒词接口已清理");
    }
    
    if (g_opus_encoder) {
        free(g_opus_encoder);
        g_opus_encoder = NULL;
    }
    
    if (g_opus_decoder) {
        free(g_opus_decoder);
        g_opus_decoder = NULL;
    }
    
    LINX_LOGI(DEMO_TAG, "✅ 所有资源清理完成");
}

/**
 * @brief 显示帮助信息
 */
void show_help() {
    printf("AudioService 演示程序 (Mac平台)\n\n");
    printf("用法: %s [选项]\n\n", "audio_service_demo_mac");
    printf("选项:\n");
    printf("  --mode <mode>     演示模式:\n");
    printf("                      basic    - 基础音频录制播放 (默认)\n");
    printf("                      vad      - VAD语音活动检测\n");
    printf("                      wakeword - 唤醒词检测\n");
    printf("                      test     - 音频测试模式\n");
    printf("                      full     - 完整功能演示\n");
    printf("  --duration <sec>  运行时长，秒 (默认: %d)\n", DEFAULT_DURATION_SEC);
    printf("  --real-audio      使用真实音频设备 (默认: 模拟音频)\n");
    printf("  --no-wakeword     禁用唤醒词功能 (默认: 启用)\n");
    printf("  --help           显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  %s --mode vad --duration 30\n", "audio_service_demo_mac");
    printf("  %s --mode wakeword --real-audio --duration 60\n", "audio_service_demo_mac");
    printf("  %s --mode full --real-audio --no-wakeword\n", "audio_service_demo_mac");
    printf("\n");
    printf("环境变量:\n");
    printf("  PICOVOICE_ACCESS_KEY  Picovoice访问密钥 (唤醒词功能需要)\n");
}

/**
 * @brief 解析命令行参数
 */
int parse_arguments(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"duration", required_argument, 0, 'd'},
        {"real-audio", no_argument, 0, 'r'},
        {"no-wakeword", no_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "m:d:rwh", long_options, NULL)) != -1) {
        switch (c) {
            case 'm':
                if (strcmp(optarg, "basic") == 0) {
                    g_demo_mode = DEMO_MODE_BASIC;
                } else if (strcmp(optarg, "vad") == 0) {
                    g_demo_mode = DEMO_MODE_VAD;
                } else if (strcmp(optarg, "wakeword") == 0) {
                    g_demo_mode = DEMO_MODE_WAKEWORD;
                } else if (strcmp(optarg, "test") == 0) {
                    g_demo_mode = DEMO_MODE_TEST;
                } else if (strcmp(optarg, "full") == 0) {
                    g_demo_mode = DEMO_MODE_FULL;
                } else {
                    fprintf(stderr, "错误: 未知的演示模式 '%s'\n", optarg);
                    return -1;
                }
                break;
            case 'd':
                g_duration_sec = atoi(optarg);
                if (g_duration_sec <= 0) {
                    fprintf(stderr, "错误: 无效的持续时间 '%s'\n", optarg);
                    return -1;
                }
                break;
            case 'r':
                g_use_real_audio = true;
                break;
            case 'w':
                g_enable_wakeword = false;
                break;
            case 'h':
                show_help();
                exit(0);
            case '?':
                return -1;
            default:
                return -1;
        }
    }
    
    return 0;
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    LINX_LOGI(DEMO_TAG, "🎵 AudioService 演示程序启动");
    
    // 解析命令行参数
    if (parse_arguments(argc, argv) != 0) {
        show_help();
        return 1;
    }
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 显示配置信息
    const char* mode_names[] = {"基础", "VAD", "唤醒词", "测试", "完整"};
    LINX_LOGI(DEMO_TAG, "演示配置:");
    LINX_LOGI(DEMO_TAG, "  模式: %s", mode_names[g_demo_mode]);
    LINX_LOGI(DEMO_TAG, "  持续时间: %d 秒", g_duration_sec);
    LINX_LOGI(DEMO_TAG, "  音频设备: %s", g_use_real_audio ? "真实设备" : "模拟设备");
    LINX_LOGI(DEMO_TAG, "  唤醒词功能: %s", g_enable_wakeword ? "启用" : "禁用");
    
    int result = 0;
    
    // 初始化AudioService
    if (setup_audio_service() != 0) {
        LINX_LOGE(DEMO_TAG, "AudioService初始化失败");
        result = 1;
        goto cleanup;
    }
    
    // 运行演示
    if (run_demo() != 0) {
        LINX_LOGE(DEMO_TAG, "演示运行失败");
        result = 1;
        goto cleanup;
    }
    
    // 打印统计信息
    print_final_statistics();
    
cleanup:
    // 清理资源
    cleanup_resources();
    
    LINX_LOGI(DEMO_TAG, "🎵 AudioService 演示程序结束 (退出码: %d)", result);
    return result;
}
