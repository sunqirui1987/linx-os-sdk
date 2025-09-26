/**
 * @file linx_demo.c
 * @brief Linx SDK 完整语音对话演示程序
 * @author Linx Team
 * @date 2024
 * 
 * 本演示程序实现了完整的语音对话功能：
 * - 实时音频录制和播放
 * - WebSocket 连接和协议处理
 * - Opus 音频编解码
 * - TTS 语音合成播放
 * - MCP 工具调用支持
 * - 多线程音频处理
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

// 引入 Linx SDK
#include "linx_sdk.h"
#include "protocols/linx_protocol.h"
#include "audio/audio_interface.h"
#include "board/mac/audio/portaudio_mac.h"
#include "codecs/audio_codec.h"
#include "codecs/opus_codec.h"
#include "play/linx_player.h"
#include "mcp/mcp_server.h"
#include "log/linx_log.h"

// 全局变量和结构体定义
typedef struct {
    LinxSdk* sdk;
    AudioInterface* audio_interface;
    audio_codec_t* opus_encoder;
    audio_codec_t* opus_decoder;
    linx_player_t* player;  // 使用linx_player模块
    mcp_server_t* mcp_server;
    
    bool running;
    bool connected;
    bool recording;
    bool playing;
    bool tts_data_complete;  // TTS数据传输是否完成
    
    pthread_t audio_thread;
    pthread_t websocket_thread;
    pthread_mutex_t audio_mutex;
    pthread_cond_t audio_cond;
    
    char server_url[256];
    int sample_rate;
    int channels;
    int frame_size;
} LinxDemo;

static LinxDemo g_demo = {0};

// TTS播放完成检测相关变量
struct timeval g_last_audio_time = {0, 0};
bool g_has_audio_data = false;

// 配置参数
#define DEFAULT_SERVER_URL "ws://xrobo-io.qiniuapi.com/v1/ws/"
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_CHANNELS 1
#define DEFAULT_FRAME_SIZE 320  // 20ms at 16kHz
#define AUDIO_BUFFER_SIZE 4096

// 函数声明
static void signal_handler(int sig);
static void event_handler(const LinxEvent* event, void* user_data);
static void player_event_callback(player_state_t old_state, player_state_t new_state, void* user_data);
static bool init_demo(const char* server_url);
static void cleanup_demo(void);
static void* audio_thread_func(void* arg);
static void* websocket_thread_func(void* arg);
static void start_recording(void);
static void stop_recording(void);
static void play_audio(const uint8_t* data, size_t size);
static bool is_play_buffer_empty(void);
static void check_tts_playback_complete(void);
static void setup_mcp_tools(void);
static void interactive_mode(void);
static void print_usage(const char* program_name);

// 播放器事件回调函数
static void player_event_callback(player_state_t old_state, player_state_t new_state, void* user_data) {
    (void)old_state; // 避免未使用参数警告
    (void)user_data; // 避免未使用参数警告
    
    switch (new_state) {
        case PLAYER_STATE_PLAYING:
            LOG_INFO("🔊 播放器开始播放");
            g_demo.playing = true;
            break;
        case PLAYER_STATE_STOPPED:
        case PLAYER_STATE_IDLE:
            LOG_INFO("🔇 播放器停止播放");
            g_demo.playing = false;
            g_demo.tts_data_complete = false;
            break;
        case PLAYER_STATE_PAUSED:
            LOG_INFO("⏸️ 播放器暂停");
            break;
        case PLAYER_STATE_ERROR:
            LOG_ERROR("❌ 播放器错误");
            g_demo.playing = false;
            break;
        default:
            break;
    }
}

// MCP工具回调函数
static mcp_return_value_t weather_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    LOG_INFO("🌤️  获取天气信息");
    return mcp_return_string("{\"temperature\": \"22°C\", \"condition\": \"晴天\"}");
}

static mcp_return_value_t calculator_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    LOG_INFO("🧮 计算器调用");
    return mcp_return_string("{\"result\": \"42\"}");
}

static mcp_return_value_t file_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    LOG_INFO("📁 文件操作");
    return mcp_return_string("{\"status\": \"success\", \"message\": \"文件操作完成\"}");
}

/**
 * 信号处理函数
 */
static void signal_handler(int sig) {
    LOG_INFO("\n收到信号 %d，正在退出...", sig);
    g_demo.running = false;
    
    if (g_demo.recording) {
        stop_recording();
    }
    
    if (g_demo.sdk && g_demo.connected) {
        linx_sdk_disconnect(g_demo.sdk);
    }
}

/**
 * 事件处理函数
 */
static void event_handler(const LinxEvent* event, void* user_data) {
    (void)user_data; // 避免未使用参数警告
    if (!event) return;
    
    switch (event->type) {
        case LINX_EVENT_WEBSOCKET_CONNECTED:
            LOG_INFO("✓ 已连接到服务器");
            g_demo.connected = true;
            break;
            
        case LINX_EVENT_WEBSOCKET_DISCONNECTED:
            LOG_WARN("✗ 与服务器断开连接");
            g_demo.connected = false;
            break;
        case LINX_EVENT_SESSION_ESTABLISHED:
            LOG_INFO("✓ 会话已建立");
            break;
        case LINX_EVENT_LISTENING_STARTED:
            LOG_INFO("✓ 会话开始");
            break;
        case LINX_EVENT_SESSION_ENDED:
            LOG_WARN("✗ 会话已结束");
            break;
            
        case LINX_EVENT_ERROR:
            LOG_ERROR("✗ 错误: %s", event->data.error.message);
            break;
            
        case LINX_EVENT_AUDIO_DATA:
            LOG_INFO("♪ 收到音频数据: %zu 字节", event->data.audio_data.value->payload_size);
            play_audio(event->data.audio_data.value->payload, event->data.audio_data.value->payload_size);
            break;
            
        case LINX_EVENT_TEXT_MESSAGE:
            LOG_INFO("💬 AI回复: %s", event->data.text_message.text);
            break;
            
        case LINX_EVENT_MCP_MESSAGE:
            LOG_INFO("🔧 MCP工具调用: %s", event->data.mcp_message.message);
            if (g_demo.mcp_server) {
                // 处理MCP工具调用
                mcp_server_parse_message(g_demo.mcp_server, event->data.mcp_message.message);
            }
            break;
            
        case LINX_EVENT_TTS_STARTED:
            LOG_INFO("🔊 开始TTS播放");
            g_demo.playing = true;
            g_demo.tts_data_complete = false;
            break;
            
        case LINX_EVENT_TTS_STOPPED:
            LOG_INFO("🔇 TTS数据传输完成，等待播放缓冲区清空...");
            g_demo.tts_data_complete = true;
            // 不立即设置playing=false，让音频缓冲区中的数据播放完毕
            break;
        case LINX_EVENT_STATE_CHANGED:
            LOG_INFO("🔧 状态改变: 老状态 %d 新状态 %d", event->data.state_changed.old_state, event->data.state_changed.new_state);
            break;
        case LINX_EVENT_EMOTION_MESSAGE:
            LOG_INFO("😊 表情消息: %s", event->data.emotion.value);
            break;
        default:
            LOG_WARN("? 未知事件类型: %d", event->type);
            break;
    }
}

/**
 * 初始化演示程序
 */
static bool init_demo(const char* server_url) {
    memset(&g_demo, 0, sizeof(g_demo));
    
    // 设置基本参数
    strncpy(g_demo.server_url, server_url, sizeof(g_demo.server_url) - 1);
    g_demo.sample_rate = DEFAULT_SAMPLE_RATE;
    g_demo.channels = DEFAULT_CHANNELS;
    g_demo.frame_size = DEFAULT_FRAME_SIZE;
    g_demo.running = true;
    
    // 初始化互斥锁和条件变量
    if (pthread_mutex_init(&g_demo.audio_mutex, NULL) != 0) {
        LOG_ERROR("✗ 音频互斥锁初始化失败");
        return false;
    }
    
    if (pthread_cond_init(&g_demo.audio_cond, NULL) != 0) {
        LOG_ERROR("✗ 音频条件变量初始化失败");
        pthread_mutex_destroy(&g_demo.audio_mutex);
        return false;
    }
    
    // 初始化SDK
    LinxSdkConfig config = {0};
    strncpy(config.server_url, server_url, sizeof(config.server_url) - 1);
    config.sample_rate = g_demo.sample_rate;
    config.channels = g_demo.channels;
    config.timeout_ms = 5000;
    config.listening_mode = LINX_LISTENING_MODE_REALTIME;
    
    // WebSocket连接配置
    strncpy(config.auth_token, "test-token", sizeof(config.auth_token) - 1);
    strncpy(config.device_id, "98:a3:16:f9:d9:34", sizeof(config.device_id) - 1);
    strncpy(config.client_id, "test-client", sizeof(config.client_id) - 1);
    config.protocol_version = 1;
    
    g_demo.sdk = linx_sdk_create(&config);
    if (!g_demo.sdk) {
        LOG_ERROR("✗ 创建SDK实例失败");
        return false;
    }
    
    linx_sdk_set_event_callback(g_demo.sdk, event_handler, NULL);
    
    // 初始化音频接口 - 使用PortAudio Mac实现
    g_demo.audio_interface = portaudio_mac_create();
    if (!g_demo.audio_interface) {
        LOG_ERROR("✗ 创建音频接口失败");
        return false;
    }
    
    // 先初始化音频接口（初始化PortAudio）
    audio_interface_init(g_demo.audio_interface);
    
    // 再配置音频参数（需要在PortAudio初始化后才能获取默认设备）
    audio_interface_set_config(g_demo.audio_interface, g_demo.sample_rate, g_demo.frame_size, 
                              g_demo.channels, 4, 8192, 2048);

    
    // 初始化Opus编解码器
    audio_format_t format = {0};
    audio_format_init(&format, g_demo.sample_rate, g_demo.channels, 16, 20);

    g_demo.opus_encoder = opus_codec_create();
    g_demo.opus_decoder = opus_codec_create();
    
    if (!g_demo.opus_encoder || !g_demo.opus_decoder) {
        LOG_ERROR("✗ Opus编解码器创建失败");
        return false;
    }
    
    // 初始化编解码器
    if (audio_codec_init_encoder(g_demo.opus_encoder, &format) != CODEC_SUCCESS ||
        audio_codec_init_decoder(g_demo.opus_decoder, &format) != CODEC_SUCCESS) {
        LOG_ERROR("✗ 初始化Opus编解码器失败");
        return false;
    }
    
    // 创建并初始化播放器
    g_demo.player = linx_player_create(g_demo.audio_interface, g_demo.opus_decoder);
    if (!g_demo.player) {
        LOG_ERROR("✗ 创建播放器失败");
        return false;
    }
    
    // 配置播放器
    player_audio_config_t player_config = {
        .sample_rate = g_demo.sample_rate,
        .channels = g_demo.channels,
        .frame_size = g_demo.frame_size,
        .buffer_size = 8192
    };
    
    if (linx_player_init(g_demo.player, &player_config) != PLAYER_SUCCESS) {
        LOG_ERROR("✗ 初始化播放器失败");
        return false;
    }
    
    // 设置播放器事件回调
    linx_player_set_event_callback(g_demo.player, player_event_callback, NULL);
    
    // 启动播放器，让其保持运行状态
    if (linx_player_start(g_demo.player) != PLAYER_SUCCESS) {
        LOG_ERROR("✗ 启动播放器失败");
        return false;
    }
    LOG_INFO("✓ 播放器已启动并保持运行状态");
    
    // 设置MCP工具
    setup_mcp_tools();
    
    LOG_INFO("✓ 演示程序初始化成功");
    return true;
}

/**
 * 设置MCP工具
 */
static void setup_mcp_tools(void) {
    g_demo.mcp_server = mcp_server_create("LinxDemo", "1.0.0");
    if (!g_demo.mcp_server) {
        LOG_ERROR("✗ MCP服务器创建失败");
        return;
    }
    
    // 添加天气工具
    mcp_property_list_t* weather_props = mcp_property_list_create();
    mcp_property_t* location_prop = mcp_property_create_string("location", "北京", true);
    mcp_property_list_add(weather_props, location_prop);
    mcp_server_add_simple_tool(g_demo.mcp_server, "get_weather", 
                              "获取指定城市的天气信息", 
                              weather_props, weather_tool_callback);
    
    // 添加计算器工具
    mcp_property_list_t* calc_props = mcp_property_list_create();
    mcp_property_t* expression_prop = mcp_property_create_string("expression", "1+1", true);
    mcp_property_list_add(calc_props, expression_prop);
    mcp_server_add_simple_tool(g_demo.mcp_server, "calculator", 
                              "执行数学计算", 
                              calc_props, calculator_tool_callback);
    
    // 添加文件操作工具
    mcp_property_list_t* file_props = mcp_property_list_create();
    mcp_property_t* path_prop = mcp_property_create_string("path", "/tmp/test.txt", true);
    mcp_property_t* operation_prop = mcp_property_create_string("operation", "read", true);
    mcp_property_list_add(file_props, path_prop);
    mcp_property_list_add(file_props, operation_prop);
    mcp_server_add_simple_tool(g_demo.mcp_server, "file_operation", 
                              "执行文件操作", 
                              file_props, file_tool_callback);
    
    LOG_INFO("✓ MCP工具设置完成");
}

/**
 * 音频线程函数
 */
static void* audio_thread_func(void* arg) {
    (void)arg; // 避免未使用参数警告
    short audio_buffer[AUDIO_BUFFER_SIZE];
    uint8_t encoded_buffer[AUDIO_BUFFER_SIZE];
    
    while (g_demo.running) {
        pthread_mutex_lock(&g_demo.audio_mutex);
        
        while (!g_demo.recording && g_demo.running) {
            pthread_cond_wait(&g_demo.audio_cond, &g_demo.audio_mutex);
        }
        
        if (!g_demo.running) {
            pthread_mutex_unlock(&g_demo.audio_mutex);
            break;
        }
        
        pthread_mutex_unlock(&g_demo.audio_mutex);
        
        // 录制音频
        int read_success = audio_interface_read(g_demo.audio_interface, 
                                               audio_buffer, g_demo.frame_size);
        
        if (read_success == 0 && g_demo.connected) {
            // 编码音频
            size_t encoded_size = 0;
            if (audio_codec_encode(g_demo.opus_encoder, (int16_t*)audio_buffer, 
                                  g_demo.frame_size,
                                  encoded_buffer, sizeof(encoded_buffer), &encoded_size) == CODEC_SUCCESS) {
                
                // 发送编码后的音频
                linx_sdk_send_audio(g_demo.sdk, encoded_buffer, encoded_size);
            }
        }
        
        usleep(10000); // 10ms
    }
    
    return NULL;
}

/**
 * WebSocket线程函数
 */
static void* websocket_thread_func(void* arg) {
    while (g_demo.running) {
        if (g_demo.sdk) {
            linx_sdk_poll_events(g_demo.sdk, 1);
        }
        
        // 检查TTS播放是否真正完成
        if (g_demo.tts_data_complete) {
            check_tts_playback_complete();
        }
        
        usleep(1000); // 1ms
    }
    return NULL;
}

/**
 * 开始录音
 */
static void start_recording(void) {
    if (g_demo.recording) {
        LOG_WARN("! 已在录音中");
        return;
    }
    
    if (!g_demo.connected) {
        LOG_ERROR("✗ 未连接到服务器");
        return;
    }
    
    pthread_mutex_lock(&g_demo.audio_mutex);
    g_demo.recording = true;
    pthread_cond_signal(&g_demo.audio_cond);
    pthread_mutex_unlock(&g_demo.audio_mutex);
    
    int ret = audio_interface_record(g_demo.audio_interface);
    if(ret != 0) {
        LOG_ERROR("✗ 录音失败: ");
        g_demo.recording = false;
        return;
    }
    LOG_INFO("🎤 开始录音...");
}

/**
 * 停止录音
 */
static void stop_recording(void) {
    if (!g_demo.recording) {
        LOG_WARN("! 未在录音");
        return;
    }
    
    pthread_mutex_lock(&g_demo.audio_mutex);
    g_demo.recording = false;
    pthread_mutex_unlock(&g_demo.audio_mutex);
    
    LOG_INFO("🎤 停止录音");
}

/**
 * 播放音频
 */
static void play_audio(const uint8_t* data, size_t size) {
    if (!data || size == 0 || !g_demo.player) return;
    
    // 使用linx_player模块播放音频（播放器已保持运行状态，只需喂数据）
    player_error_t ret = linx_player_feed_data(g_demo.player, data, size);
    if (ret != PLAYER_SUCCESS) {
        LOG_ERROR("✗ 播放失败: %s", linx_player_error_string(ret));
    } else {
        // 更新最后音频播放时间（用于TTS播放完成检测）
        gettimeofday(&g_last_audio_time, NULL);
        g_has_audio_data = true;
    }
}

/**
 * 检查播放缓冲区是否为空
 */
static bool is_play_buffer_empty(void) {
    // 使用linx_player模块检查播放缓冲区状态
    if (!g_demo.player) {
        return true;
    }
    
    // 检查播放器缓冲区是否为空且播放器状态为空闲
    bool buffer_empty = linx_player_is_buffer_empty(g_demo.player) && 
                       (linx_player_get_state(g_demo.player) == PLAYER_STATE_IDLE);
    
    // 如果缓冲区为空，重置音频数据标志
    if (buffer_empty) {
        g_has_audio_data = false;
    }
    
    return buffer_empty;
}

/**
 * 检查TTS播放是否真正完成
 */
static void check_tts_playback_complete(void) {
    if (g_demo.tts_data_complete && g_demo.playing && is_play_buffer_empty()) {
        LOG_INFO("🔇 TTS播放真正完成");
        g_demo.playing = false;
        g_demo.tts_data_complete = false;
    }
}

/**
 * 交互模式
 */
static void interactive_mode(void) {
    char input[1024];
    
    printf("\n=== Linx 语音对话演示 ===\n");
    printf("命令:\n");
    printf("  /start    - 开始录音\n");
    printf("  /stop     - 停止录音\n");
    printf("  /status   - 显示状态\n");
    printf("  /tools    - 显示MCP工具\n");
    printf("  /help     - 显示帮助\n");
    printf("  /quit     - 退出程序\n");
    printf("  其他文本  - 发送文本消息\n\n");
    
    // 启动线程
    pthread_create(&g_demo.audio_thread, NULL, audio_thread_func, NULL);
    pthread_create(&g_demo.websocket_thread, NULL, websocket_thread_func, NULL);
    
    while (g_demo.running) {
        printf("linx> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "/quit") == 0) {
            break;
        } else if (strcmp(input, "/start") == 0) {
            start_recording();
        } else if (strcmp(input, "/stop") == 0) {
            stop_recording();
        } else if (strcmp(input, "/status") == 0) {
            printf("连接状态: %s\n", g_demo.connected ? "已连接" : "未连接");
            printf("录音状态: %s\n", g_demo.recording ? "录音中" : "未录音");
            printf("播放状态: %s\n", g_demo.playing ? "播放中" : "未播放");
            if (g_demo.player) {
                player_state_t state = linx_player_get_state(g_demo.player);
                const char* state_str = "未知";
                switch (state) {
                    case PLAYER_STATE_IDLE: state_str = "空闲"; break;
                    case PLAYER_STATE_PLAYING: state_str = "播放中"; break;
                    case PLAYER_STATE_PAUSED: state_str = "暂停"; break;
                    case PLAYER_STATE_STOPPED: state_str = "停止"; break;
                    case PLAYER_STATE_ERROR: state_str = "错误"; break;
                }
                printf("播放器状态: %s\n", state_str);
                printf("缓冲区使用率: %.1f%%\n", linx_player_get_buffer_usage(g_demo.player) * 100);
            }
        } else if (strcmp(input, "/tools") == 0) {
            if (g_demo.mcp_server) {
                char* tools_json = mcp_server_get_tools_list_json(g_demo.mcp_server, NULL, false);
                printf("可用工具:\n%s\n", tools_json);
                free(tools_json);
            }
        } else if (strcmp(input, "/help") == 0) {
            print_usage("linx_demo");
        } else {
            LOG_WARN("✗ 未知命令: %s", input);
        }
    }
    
    // 等待线程结束
    g_demo.running = false;
    pthread_cond_signal(&g_demo.audio_cond);
    pthread_join(g_demo.audio_thread, NULL);
    pthread_join(g_demo.websocket_thread, NULL);
}

/**
 * 清理资源
 */
static void cleanup_demo(void) {
    g_demo.running = false;
    
    if (g_demo.recording) {
        stop_recording();
    }
    
    if (g_demo.sdk) {
        if (g_demo.connected) {
            linx_sdk_disconnect(g_demo.sdk);
        }
        linx_sdk_destroy(g_demo.sdk);
    }
    
    if (g_demo.audio_interface) {
        audio_interface_destroy(g_demo.audio_interface);
    }
    
    if (g_demo.opus_encoder) {
        audio_codec_destroy(g_demo.opus_encoder);
    }
    
    if (g_demo.opus_decoder) {
        audio_codec_destroy(g_demo.opus_decoder);
    }
    
    if (g_demo.player) {
        // 显式停止播放器
        linx_player_stop(g_demo.player);
        linx_player_destroy(g_demo.player);
    }
    
    if (g_demo.mcp_server) {
        mcp_server_destroy(g_demo.mcp_server);
    }
    
    pthread_mutex_destroy(&g_demo.audio_mutex);
    pthread_cond_destroy(&g_demo.audio_cond);
    
    LOG_INFO("✓ 资源清理完成");
}

/**
 * 打印使用说明
 */
static void print_usage(const char* program_name) {
    printf("Linx SDK 完整语音对话演示程序\n");
    printf("用法: %s [选项]\n\n", program_name);
    printf("选项:\n");
    printf("  -h, --help              显示此帮助信息\n");
    printf("  -s, --server URL        WebSocket服务器地址 (默认: %s)\n", DEFAULT_SERVER_URL);
    printf("  -i, --interactive       交互模式 (默认)\n");
    printf("\n");
    printf("功能特性:\n");
    printf("  • 实时音频录制和播放\n");
    printf("  • Opus音频编解码\n");
    printf("  • WebSocket通信\n");
    printf("  • TTS语音合成\n");
    printf("  • MCP工具调用支持\n");
    printf("  • 多线程音频处理\n");
}



/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    const char* server_url = DEFAULT_SERVER_URL;

      // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_DEBUG;  // 默认INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return 0;
    }
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0) {
            if (i + 1 < argc) {
                server_url = argv[++i];
            } else {
                LOG_ERROR("✗ 缺少服务器地址参数");
                return 1;
            }
        }
    }
    
    // 初始化演示程序
    if (!init_demo(server_url)) {
        LOG_ERROR("✗ 演示程序初始化失败");
        return 1;
    }
    
    // 连接到服务器
    LOG_INFO("正在连接到服务器: %s", server_url);
    LinxSdkError result = linx_sdk_connect(g_demo.sdk);
    if (result != LINX_SDK_SUCCESS) {
        LOG_ERROR("✗ 连接失败: %d", result);
        cleanup_demo();
        return 1;
    }
    
    // 等待连接建立
    int wait_count = 0;
    while (!g_demo.connected && g_demo.running && wait_count < 50) {
        usleep(100000); // 100ms
        wait_count++;
    }
    
    if (!g_demo.connected) {
        LOG_ERROR("✗ 连接超时");
        cleanup_demo();
        return 1;
    }
    
    // 进入交互模式
    interactive_mode();
    
    // 清理资源
    cleanup_demo();
    
    LOG_INFO("程序退出");
    return 0;
}