/**
 * @file example_linx_websocket.c
 * @brief linx_websocket 长连接应用示例
 * 
 * 这个示例展示了如何使用 linx_websocket 创建一个持续运行的长连接应用，
 * 类似于完整的语音交互客户端，包括多线程处理、状态管理和优雅退出。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include "linx_websocket.h"

// ==================== 全局状态管理 ====================

/**
 * @brief 应用程序状态结构体
 */
typedef struct {
    bool running;                    // 应用程序运行状态
    bool connected;                  // WebSocket连接状态
    char* session_id;               // 会话ID
    char* listen_state;             // 监听状态: "start" 或 "stop"
    char* tts_state;                // TTS状态: "start", "stop", "idle"
    pthread_mutex_t state_mutex;    // 状态访问互斥锁
    FILE* input_file;               // 输入音频文件句柄
    FILE* output_file;              // 输出音频文件句柄
    char* input_file_path;          // 输入文件路径
    char* output_file_path;         // 输出文件路径
    size_t audio_packet_counter;    // 音频包计数器
} app_state_t;

// 全局状态实例
static app_state_t g_app_state = {
    .running = true,
    .connected = false,
    .session_id = NULL,
    .listen_state = NULL,
    .tts_state = NULL,
    .state_mutex = PTHREAD_MUTEX_INITIALIZER,
    .input_file = NULL,
    .output_file = NULL,
    .input_file_path = NULL,
    .output_file_path = NULL,
    .audio_packet_counter = 0
};

// 全局WebSocket协议实例
static linx_websocket_protocol_t* g_ws_protocol = NULL;

// ==================== 状态管理函数 ====================

static void set_app_running(bool running) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    g_app_state.running = running;
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

static bool is_app_running() {
    pthread_mutex_lock(&g_app_state.state_mutex);
    bool running = g_app_state.running;
    pthread_mutex_unlock(&g_app_state.state_mutex);
    return running;
}

static void set_session_id(const char* session_id) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    if (g_app_state.session_id) {
        free(g_app_state.session_id);
    }
    g_app_state.session_id = session_id ? strdup(session_id) : NULL;
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

static void set_listen_state(const char* state) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    if (g_app_state.listen_state) {
        free(g_app_state.listen_state);
    }
    g_app_state.listen_state = state ? strdup(state) : NULL;
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

static void set_tts_state(const char* state) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    if (g_app_state.tts_state) {
        free(g_app_state.tts_state);
    }
    g_app_state.tts_state = state ? strdup(state) : NULL;
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

static bool open_input_file(const char* file_path) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    
    if (g_app_state.input_file) {
        fclose(g_app_state.input_file);
    }
    
    g_app_state.input_file = fopen(file_path, "rb");
    if (!g_app_state.input_file) {
        printf("❌ 无法打开输入文件: %s\n", file_path);
        pthread_mutex_unlock(&g_app_state.state_mutex);
        return false;
    }
    
    if (g_app_state.input_file_path) {
        free(g_app_state.input_file_path);
    }
    g_app_state.input_file_path = strdup(file_path);
    
    printf("✅ 成功打开输入文件: %s\n", file_path);
    pthread_mutex_unlock(&g_app_state.state_mutex);
    return true;
}

static bool open_output_file(const char* file_path) {
    pthread_mutex_lock(&g_app_state.state_mutex);
    
    if (g_app_state.output_file) {
        fclose(g_app_state.output_file);
    }
    
    g_app_state.output_file = fopen(file_path, "wb");
    if (!g_app_state.output_file) {
        printf("❌ 无法创建输出文件: %s\n", file_path);
        pthread_mutex_unlock(&g_app_state.state_mutex);
        return false;
    }
    
    if (g_app_state.output_file_path) {
        free(g_app_state.output_file_path);
    }
    g_app_state.output_file_path = strdup(file_path);
    
    printf("✅ 成功创建输出文件: %s\n", file_path);
    pthread_mutex_unlock(&g_app_state.state_mutex);
    return true;
}

static void close_files() {
    pthread_mutex_lock(&g_app_state.state_mutex);
    
    if (g_app_state.input_file) {
        fclose(g_app_state.input_file);
        g_app_state.input_file = NULL;
    }
    
    if (g_app_state.output_file) {
        fclose(g_app_state.output_file);
        g_app_state.output_file = NULL;
    }
    
    if (g_app_state.input_file_path) {
        free(g_app_state.input_file_path);
        g_app_state.input_file_path = NULL;
    }
    
    if (g_app_state.output_file_path) {
        free(g_app_state.output_file_path);
        g_app_state.output_file_path = NULL;
    }
    
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

// ==================== 信号处理 ====================

static void signal_handler(int sig) {
    printf("\n🛑 收到信号 %d，准备退出...\n", sig);
    set_app_running(false);
}

// ==================== WebSocket 回调函数 ====================

static void on_websocket_connected(void* user_data) {
    printf("🔗 WebSocket 连接已建立\n");
    pthread_mutex_lock(&g_app_state.state_mutex);
    g_app_state.connected = true;
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

static void on_websocket_disconnected(void* user_data) {
    printf("🔌 WebSocket 连接已断开\n");
    pthread_mutex_lock(&g_app_state.state_mutex);
    g_app_state.connected = false;
    pthread_mutex_unlock(&g_app_state.state_mutex);
    set_app_running(false);
}

static void on_websocket_error(const char* error_msg, void* user_data) {
    printf("❌ WebSocket 错误: %s\n", error_msg);
}

static void on_websocket_message(const cJSON* root, void* user_data) {
    char* json_string = cJSON_Print(root);
    if (json_string) {
        printf("📨 收到消息: %s\n", json_string);
        
        // 解析消息类型
        const cJSON* type = cJSON_GetObjectItem(root, "type");
        if (type && cJSON_IsString(type)) {
            // 处理hello响应
            if (strcmp(type->valuestring, "hello") == 0) {
                const cJSON* session_id = cJSON_GetObjectItem(root, "session_id");
                if (session_id && cJSON_IsString(session_id)) {
                    set_session_id(session_id->valuestring);
                    printf("✅ 会话建立，ID: %s\n", session_id->valuestring);
                    
                    // 开始监听 - 修复：传递正确的参数
                    set_listen_state("start");
                    linx_protocol_send_start_listening((linx_protocol_t*)g_ws_protocol, LINX_LISTENING_MODE_AUTO_STOP);
                    printf("🎤 开始语音监听\n");
                }
            }
            // 处理TTS状态
            else if (strcmp(type->valuestring, "tts") == 0) {
                const cJSON* state = cJSON_GetObjectItem(root, "state");
                if (state && cJSON_IsString(state)) {
                    set_tts_state(state->valuestring);
                    printf("🔊 TTS状态: %s\n", state->valuestring);
                    
                    if (strcmp(state->valuestring, "start") == 0) {
                        // TTS开始播放，停止监听避免回音
                        set_listen_state("stop");
                        linx_protocol_send_stop_listening((linx_protocol_t*)g_ws_protocol);
                        printf("🔇 停止监听（TTS播放中）\n");
                    } else if (strcmp(state->valuestring, "stop") == 0) {
                        // TTS播放结束，重新开始监听
                        set_listen_state("start");
                        linx_protocol_send_start_listening((linx_protocol_t*)g_ws_protocol, LINX_LISTENING_MODE_AUTO_STOP);
                        printf("🎤 恢复语音监听\n");
                    }
                }
            }
            // 处理goodbye消息
            else if (strcmp(type->valuestring, "goodbye") == 0) {
                printf("👋 会话结束\n");
                set_session_id(NULL);
            }
        }
        
        free(json_string);
    }
}

static void on_websocket_audio_data(linx_audio_stream_packet_t* packet, void* user_data) {
    printf("🎵 收到音频数据: %zu 字节, 采样率: %d, 帧时长: %d\n", 
           packet->payload_size, packet->sample_rate, packet->frame_duration);
    
    // 将接收到的音频数据保存到输出文件
    pthread_mutex_lock(&g_app_state.state_mutex);
    if (g_app_state.output_file && packet->payload && packet->payload_size > 0) {
        size_t written = fwrite(packet->payload, 1, packet->payload_size, g_app_state.output_file);
        if (written == packet->payload_size) {
            g_app_state.audio_packet_counter++;
            printf("💾 已保存音频数据: %zu 字节 (包序号: %zu)\n", written, g_app_state.audio_packet_counter);
            fflush(g_app_state.output_file); // 立即刷新到文件
        } else {
            printf("❌ 保存音频数据失败: 期望 %zu 字节, 实际写入 %zu 字节\n", packet->payload_size, written);
        }
    }
    pthread_mutex_unlock(&g_app_state.state_mutex);
}

// ==================== 工作线程函数 ====================

/**
 * @brief WebSocket事件处理线程
 */
static void* websocket_event_thread(void* arg) {
    printf("🔄 WebSocket事件处理线程启动\n");
    
    while (is_app_running()) {
        if (g_ws_protocol) {
            linx_websocket_poll(g_ws_protocol, 10);
        }
        usleep(10000); // 10ms
    }
    
    printf("🔄 WebSocket事件处理线程退出\n");
    return NULL;
}

/**
 * @brief 音频文件发送线程
 */
static void* audio_record_thread(void* arg) {
    printf("🎤 音频文件发送线程启动\n");
    
    while (is_app_running()) {
        // 检查是否应该发送音频
        pthread_mutex_lock(&g_app_state.state_mutex);
        
        // // 🐛 调试打印：显示所有相关状态
        // printf("🐛 [DEBUG] g_app_state 状态检查:\n");
        // printf("🐛   - connected: %s\n", g_app_state.connected ? "true" : "false");
        // printf("🐛   - listen_state: %s\n", g_app_state.listen_state ? g_app_state.listen_state : "NULL");
        // printf("🐛   - input_file: %s\n", g_app_state.input_file ? "NOT NULL" : "NULL");
        // printf("🐛   - g_ws_protocol: %s\n", g_ws_protocol ? "NOT NULL" : "NULL");
        
        bool should_send = g_app_state.connected && 
                          g_app_state.listen_state && 
                          strcmp(g_app_state.listen_state, "start") == 0 &&
                          g_app_state.input_file;
        
       // printf("🐛   - should_send 结果: %s\n", should_send ? "true" : "false");
        
        pthread_mutex_unlock(&g_app_state.state_mutex);
        //printf("🎤 音频文件发送线程启动》〉》〉》〉》〉》〉》\n");
        
        if (should_send && g_ws_protocol) {
            //等一会再发送
            usleep(6000000); // 6000ms

            printf("📤 开始发送音频文件...\n");
            
            // 重置文件指针到开头
            pthread_mutex_lock(&g_app_state.state_mutex);
            if (g_app_state.input_file) {
                fseek(g_app_state.input_file, 0, SEEK_SET);
            }
            pthread_mutex_unlock(&g_app_state.state_mutex);
            
            // 发送整个文件
            const size_t chunk_size = 4096; // 每次读取的字节数
            uint8_t buffer[chunk_size];
            size_t total_sent = 0;
            
            while (is_app_running()) {
                pthread_mutex_lock(&g_app_state.state_mutex);
                size_t bytes_read = 0;
                if (g_app_state.input_file) {
                    bytes_read = fread(buffer, 1, chunk_size, g_app_state.input_file);
                }
                pthread_mutex_unlock(&g_app_state.state_mutex);
                
                if (bytes_read == 0) {
                    // 文件读取完毕
                    break;
                }
                
                // 创建音频包并发送
                linx_audio_stream_packet_t* packet = linx_audio_stream_packet_create(bytes_read);
                if (packet) {
                    packet->sample_rate = LINX_WEBSOCKET_AUDIO_SAMPLE_RATE;
                    packet->frame_duration = LINX_WEBSOCKET_AUDIO_FRAME_DURATION;
                    packet->timestamp = time(NULL) * 1000;
                    
                    // 复制从文件读取的数据
                    memcpy(packet->payload, buffer, bytes_read);
                    
                    // 打印音频数据信息
                    printf("📊 音频包信息:\n");
                    printf("   - 采样率: %d Hz\n", packet->sample_rate);
                    printf("   - 帧时长: %d ms\n", packet->frame_duration);
                    printf("   - 时间戳: %llu\n", packet->timestamp);
                    printf("   - 数据大小: %zu 字节\n", packet->payload_size);
                    
                    // 打印前32字节的十六进制数据（如果数据足够长）
                    size_t print_size = packet->payload_size > 32 ? 32 : packet->payload_size;
                    printf("   - 数据内容 (前%zu字节): ", print_size);
                    for (size_t i = 0; i < print_size; i++) {
                        printf("%02X ", ((unsigned char*)packet->payload)[i]);
                        if ((i + 1) % 16 == 0) printf("\n                                ");
                    }
                    printf("\n");
                    
                    if (linx_websocket_send_audio((linx_protocol_t*)g_ws_protocol, packet)) {
                        total_sent += packet->payload_size;
                        printf("🎵 发送音频数据: %zu 字节 (总计: %zu 字节)\n", packet->payload_size, total_sent);
                    } else {
                        printf("❌ 发送音频数据失败\n");
                    }
                    
                    linx_audio_stream_packet_destroy(packet);
                }
                
                usleep(LINX_WEBSOCKET_AUDIO_FRAME_DURATION * 1000); // 音频帧间隔，转换为微秒
            }
            
            printf("✅ 音频文件发送完成，总计发送: %zu 字节\n", total_sent);
            printf("⏰ 等待1分钟后再次发送...\n");
            
            // 等待1分钟
            for (int i = 0; i < 60 && is_app_running(); i++) {
                sleep(1);
            }
        } else {
            // 如果不满足发送条件，短暂等待
            usleep(1000000); // 1000ms
        }
    }
    
    printf("🎤 音频文件发送线程退出\n");
    return NULL;
}

/**
 * @brief 模拟音频播放线程
 */
static void* audio_playback_thread(void* arg) {
    printf("🔊 音频播放线程启动\n");
    
    while (is_app_running()) {
        // 这里可以添加音频播放逻辑
        // 例如：从音频缓冲区取出数据并播放
        usleep(20000); // 20ms
    }
    
    printf("🔊 音频播放线程退出\n");
    return NULL;
}

/**
 * @brief 状态监控线程
 */
static void* status_monitor_thread(void* arg) {
    printf("📊 状态监控线程启动\n");
    
    while (is_app_running()) {
        pthread_mutex_lock(&g_app_state.state_mutex);
        printf("📊 状态报告 - 连接: %s, 会话: %s, 监听: %s, TTS: %s\n",
               g_app_state.connected ? "已连接" : "未连接",
               g_app_state.session_id ? g_app_state.session_id : "无",
               g_app_state.listen_state ? g_app_state.listen_state : "无",
               g_app_state.tts_state ? g_app_state.tts_state : "无");
        pthread_mutex_unlock(&g_app_state.state_mutex);
        
        sleep(10); // 每10秒报告一次状态
    }
    
    printf("📊 状态监控线程退出\n");
    return NULL;
}

// ==================== 主函数 ====================

int main() {
    printf("🚀 Linx WebSocket 长连接应用\n");
    printf("============================\n\n");

    // 1. 设置信号处理
    printf("1️⃣ 设置信号处理...\n");
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    printf("✅ 信号处理设置完成\n\n");

    // 1.5. 打开音频文件
    printf("1️⃣.5️⃣ 打开音频文件...\n");
    const char* input_file_path = "audio.opus";
    const char* output_file_path = "received_audio.opus";
    
    if (!open_input_file(input_file_path)) {
        fprintf(stderr, "❌ 无法打开输入音频文件，程序退出\n");
        return 1;
    }
    
    if (!open_output_file(output_file_path)) {
        fprintf(stderr, "❌ 无法创建输出音频文件，程序退出\n");
        close_files();
        return 1;
    }
    printf("✅ 音频文件准备完成\n\n");

    // 2. 创建 WebSocket 协议实例
    printf("2️⃣ 创建 WebSocket 协议实例...\n");
    //"ws://114.66.50.145:8000/xiaozhi/v1/",// "
    //"ws://114.66.50.145:8000/xiaozhi/v1/",// 
    linx_websocket_config_t config = {
        .url = "ws://xrobo-io.qiniuapi.com/v1/ws/",
        .auth_token = "test-token",
        .device_id = "98:a3:16:f9:d9:34",
        .client_id = "test-client",
        .protocol_version = 1
    };
    
    g_ws_protocol = linx_websocket_protocol_create(&config);
    if (!g_ws_protocol) {
        fprintf(stderr, "❌ 创建 WebSocket 协议失败\n");
        return 1;
    }
    printf("✅ WebSocket 协议创建成功\n\n");

    // 3. 设置回调函数
    printf("3️⃣ 设置回调函数...\n");
    linx_protocol_callbacks_t callbacks = {
        .on_connected = on_websocket_connected,
        .on_disconnected = on_websocket_disconnected,
        .on_network_error = on_websocket_error,
        .on_incoming_json = on_websocket_message,
        .on_incoming_audio = on_websocket_audio_data,
        .user_data = NULL
    };
    linx_protocol_set_callbacks((linx_protocol_t*)g_ws_protocol, &callbacks);
    printf("✅ 回调函数设置完成\n\n");

    // 4. 启动 WebSocket 连接
    printf("4️⃣ 启动 WebSocket 连接...\n");
    if (!linx_websocket_start((linx_protocol_t*)g_ws_protocol)) {
        printf("❌ WebSocket 连接启动失败\n");
        linx_websocket_destroy((linx_protocol_t*)g_ws_protocol);
        return 1;
    }
    printf("✅ WebSocket 连接启动成功\n\n");

    // 5. 启动工作线程
    printf("5️⃣ 启动工作线程...\n");
    
    pthread_t websocket_thread, audio_record_thread_id, audio_playback_thread_id, status_thread;
    
    if (pthread_create(&websocket_thread, NULL, websocket_event_thread, NULL) != 0) {
        fprintf(stderr, "❌ 创建WebSocket事件线程失败\n");
        goto cleanup;
    }
    
    if (pthread_create(&audio_record_thread_id, NULL, audio_record_thread, NULL) != 0) {
        fprintf(stderr, "❌ 创建音频录制线程失败\n");
        goto cleanup;
    }
    
    if (pthread_create(&audio_playback_thread_id, NULL, audio_playback_thread, NULL) != 0) {
        fprintf(stderr, "❌ 创建音频播放线程失败\n");
        goto cleanup;
    }
    
    if (pthread_create(&status_thread, NULL, status_monitor_thread, NULL) != 0) {
        fprintf(stderr, "❌ 创建状态监控线程失败\n");
        goto cleanup;
    }
    
    printf("✅ 所有工作线程启动成功\n\n");

    // 6. 主循环 - 等待连接建立
    printf("6️⃣ 等待连接建立...\n");
    int connection_timeout = 30; // 30秒超时
    while (is_app_running() && connection_timeout > 0) {
        pthread_mutex_lock(&g_app_state.state_mutex);
        bool connected = g_app_state.connected;
        pthread_mutex_unlock(&g_app_state.state_mutex);
        
        if (connected) {
            printf("✅ WebSocket 连接已建立\n");
            break;
        }
        
        sleep(1);
        connection_timeout--;
    }
    
    if (connection_timeout <= 0) {
        printf("⏰ 连接超时，退出应用\n");
        set_app_running(false);
    }

    // 7. 主循环 - 保持应用运行
    printf("\n7️⃣ 应用运行中...\n");
    printf("💡 按 Ctrl+C 退出应用\n\n");
    
    while (is_app_running()) {
        sleep(1);
    }

    // 8. 等待所有线程结束
    printf("\n8️⃣ 等待线程结束...\n");
    
    pthread_join(websocket_thread, NULL);
    pthread_join(audio_record_thread_id, NULL);
    pthread_join(audio_playback_thread_id, NULL);
    pthread_join(status_thread, NULL);
    
    printf("✅ 所有线程已结束\n");

cleanup:
    // 9. 清理资源
    printf("\n9️⃣ 清理资源...\n");
    
    if (g_ws_protocol) {
        linx_websocket_stop(g_ws_protocol);
        linx_websocket_destroy((linx_protocol_t*)g_ws_protocol);
        g_ws_protocol = NULL;
    }
    
    // 关闭文件
    close_files();
    
    // 清理状态
    pthread_mutex_lock(&g_app_state.state_mutex);
    if (g_app_state.session_id) {
        free(g_app_state.session_id);
        g_app_state.session_id = NULL;
    }
    if (g_app_state.listen_state) {
        free(g_app_state.listen_state);
        g_app_state.listen_state = NULL;
    }
    if (g_app_state.tts_state) {
        free(g_app_state.tts_state);
        g_app_state.tts_state = NULL;
    }
    pthread_mutex_unlock(&g_app_state.state_mutex);
    
    printf("✅ 资源清理完成\n\n");
    printf("👋 Linx WebSocket 长连接应用退出\n");
    
    return 0;
}