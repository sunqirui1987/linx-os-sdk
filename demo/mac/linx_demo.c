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
#include "audio/portaudio_mac.h"
#include "codecs/audio_codec.h"
#include "codecs/opus_codec.h"
#include "mcp/mcp_server.h"
#include "log/linx_log.h"

// 全局变量和结构体定义
typedef struct {
    LinxSdk* sdk;
    AudioInterface* audio_interface;
    audio_codec_t* opus_encoder;
    audio_codec_t* opus_decoder;
    mcp_server_t* mcp_server;
    
    bool running;
    bool connected;
    bool recording;
    bool playing;
    
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

// 配置参数
#define DEFAULT_SERVER_URL "ws://xrobo-io.qiniuapi.com/v1/ws/"
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_CHANNELS 1
#define DEFAULT_FRAME_SIZE 320  // 20ms at 16kHz
#define AUDIO_BUFFER_SIZE 4096

// 函数声明
static void signal_handler(int sig);
static void event_handler(const LinxEvent* event, void* user_data);
static bool init_demo(const char* server_url);
static void cleanup_demo(void);
static void* audio_thread_func(void* arg);
static void* websocket_thread_func(void* arg);
static void start_recording(void);
static void stop_recording(void);
static void play_audio(const uint8_t* data, size_t size);
static void setup_mcp_tools(void);
static void interactive_mode(void);
static void print_usage(const char* program_name);

// MCP工具回调函数
static mcp_return_value_t weather_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    printf("🌤️  获取天气信息\n");
    return mcp_return_string("{\"temperature\": \"22°C\", \"condition\": \"晴天\"}");
}

static mcp_return_value_t calculator_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    printf("🧮 计算器调用\n");
    return mcp_return_string("{\"result\": \"42\"}");
}

static mcp_return_value_t file_tool_callback(const struct mcp_property_list* properties) {
    (void)properties; // 避免未使用参数警告
    printf("📁 文件操作\n");
    return mcp_return_string("{\"status\": \"success\", \"message\": \"文件操作完成\"}");
}

/**
 * 信号处理函数
 */
static void signal_handler(int sig) {
    printf("\n收到信号 %d，正在退出...\n", sig);
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
    if (!event) return;
    
    switch (event->type) {
        case LINX_EVENT_WEBSOCKET_CONNECTED:
            printf("✓ 已连接到服务器\n");
            g_demo.connected = true;
            break;
            
        case LINX_EVENT_WEBSOCKET_DISCONNECTED:
            printf("✗ 与服务器断开连接\n");
            g_demo.connected = false;
            break;
            
        case LINX_EVENT_ERROR:
            printf("✗ 错误: %s\n", event->data.error.message);
            break;
            
        case LINX_EVENT_AUDIO_DATA:
            printf("♪ 收到音频数据: %zu 字节\n", event->data.audio_data.size);
            play_audio(event->data.audio_data.data, event->data.audio_data.size);
            break;
            
        case LINX_EVENT_TEXT_MESSAGE:
            printf("💬 AI回复: %s\n", event->data.text_message.text);
            break;
            
        case LINX_EVENT_MCP_MESSAGE:
            printf("🔧 MCP工具调用: %s\n", event->data.mcp_message.message);
            if (g_demo.mcp_server) {
                // 处理MCP工具调用
                mcp_server_parse_message(g_demo.mcp_server, event->data.mcp_message.message);
            }
            break;
            
        case LINX_EVENT_TTS_STARTED:
            printf("🔊 开始TTS播放\n");
            g_demo.playing = true;
            break;
            
        case LINX_EVENT_TTS_STOPPED:
            printf("🔇 TTS播放完成\n");
            g_demo.playing = false;
            break;
            
        default:
            printf("? 未知事件类型: %d\n", event->type);
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
        printf("✗ 音频互斥锁初始化失败\n");
        return false;
    }
    
    if (pthread_cond_init(&g_demo.audio_cond, NULL) != 0) {
        printf("✗ 音频条件变量初始化失败\n");
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
        printf("✗ 创建SDK实例失败\n");
        return false;
    }
    
    linx_sdk_set_event_callback(g_demo.sdk, event_handler, NULL);
    
    // 初始化音频接口 - 使用PortAudio Mac实现
    g_demo.audio_interface = portaudio_mac_create();
    if (!g_demo.audio_interface) {
        printf("✗ 创建音频接口失败\n");
        return false;
    }
    
    // 配置音频参数
    audio_interface_set_config(g_demo.audio_interface, g_demo.sample_rate, g_demo.frame_size, 
                              g_demo.channels, 2, 1024, 256);
    
    audio_interface_init(g_demo.audio_interface);
    
    // 初始化Opus编解码器
    audio_format_t format = {0};
    audio_format_init(&format, g_demo.sample_rate, g_demo.channels, 16, 20);
    
    g_demo.opus_encoder = opus_codec_create();
    g_demo.opus_decoder = opus_codec_create();
    
    if (!g_demo.opus_encoder || !g_demo.opus_decoder) {
        printf("✗ Opus编解码器创建失败\n");
        return false;
    }
    
    // 初始化编解码器
    if (audio_codec_init_encoder(g_demo.opus_encoder, &format) != CODEC_SUCCESS ||
        audio_codec_init_decoder(g_demo.opus_decoder, &format) != CODEC_SUCCESS) {
        printf("✗ 初始化Opus编解码器失败\n");
        return false;
    }
    
    // 设置MCP工具
    setup_mcp_tools();
    
    printf("✓ 演示程序初始化成功\n");
    return true;
}

/**
 * 设置MCP工具
 */
static void setup_mcp_tools(void) {
    g_demo.mcp_server = mcp_server_create("LinxDemo", "1.0.0");
    if (!g_demo.mcp_server) {
        printf("✗ MCP服务器创建失败\n");
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
    
    printf("✓ MCP工具设置完成\n");
}

/**
 * 音频线程函数
 */
static void* audio_thread_func(void* arg) {
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
        bool read_success = audio_interface_read(g_demo.audio_interface, 
                                               audio_buffer, g_demo.frame_size);
        
        if (read_success && g_demo.connected) {
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
        usleep(1000); // 1ms
    }
    return NULL;
}

/**
 * 开始录音
 */
static void start_recording(void) {
    if (g_demo.recording) {
        printf("! 已在录音中\n");
        return;
    }
    
    if (!g_demo.connected) {
        printf("✗ 未连接到服务器\n");
        return;
    }
    
    pthread_mutex_lock(&g_demo.audio_mutex);
    g_demo.recording = true;
    pthread_cond_signal(&g_demo.audio_cond);
    pthread_mutex_unlock(&g_demo.audio_mutex);
    
    audio_interface_record(g_demo.audio_interface);
    printf("🎤 开始录音...\n");
}

/**
 * 停止录音
 */
static void stop_recording(void) {
    if (!g_demo.recording) {
        printf("! 未在录音\n");
        return;
    }
    
    pthread_mutex_lock(&g_demo.audio_mutex);
    g_demo.recording = false;
    pthread_mutex_unlock(&g_demo.audio_mutex);
    
    printf("🎤 停止录音\n");
}

/**
 * 播放音频
 */
static void play_audio(const uint8_t* data, size_t size) {
    if (!data || size == 0) return;
    
    short decoded_buffer[AUDIO_BUFFER_SIZE];
    size_t decoded_size = 0;
    
    // 解码音频
    if (audio_codec_decode(g_demo.opus_decoder, data, size,
                         (int16_t*)decoded_buffer, sizeof(decoded_buffer)/sizeof(int16_t), &decoded_size) == CODEC_SUCCESS) {
        
        // 播放解码后的音频
        audio_interface_write(g_demo.audio_interface, decoded_buffer, decoded_size);
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
        } else if (strcmp(input, "/tools") == 0) {
            if (g_demo.mcp_server) {
                char* tools_json = mcp_server_get_tools_list_json(g_demo.mcp_server, NULL, false);
                printf("可用工具:\n%s\n", tools_json);
                free(tools_json);
            }
        } else if (strcmp(input, "/help") == 0) {
            print_usage("linx_demo");
        } else if (input[0] != '/') {
            if (g_demo.connected) {
                linx_sdk_send_text(g_demo.sdk, input);
                printf("✓ 文本已发送\n");
            } else {
                printf("✗ 未连接到服务器\n");
            }
        } else {
            printf("✗ 未知命令: %s\n", input);
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
    
    if (g_demo.mcp_server) {
        mcp_server_destroy(g_demo.mcp_server);
    }
    
    pthread_mutex_destroy(&g_demo.audio_mutex);
    pthread_cond_destroy(&g_demo.audio_cond);
    
    printf("✓ 资源清理完成\n");
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
                printf("✗ 缺少服务器地址参数\n");
                return 1;
            }
        }
    }
    
    // 初始化演示程序
    if (!init_demo(server_url)) {
        printf("✗ 演示程序初始化失败\n");
        return 1;
    }
    
    // 连接到服务器
    printf("正在连接到服务器: %s\n", server_url);
    LinxSdkError result = linx_sdk_connect(g_demo.sdk);
    if (result != LINX_SDK_SUCCESS) {
        printf("✗ 连接失败: %d\n", result);
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
        printf("✗ 连接超时\n");
        cleanup_demo();
        return 1;
    }
    
    // 进入交互模式
    interactive_mode();
    
    // 清理资源
    cleanup_demo();
    
    printf("程序退出\n");
    return 0;
}