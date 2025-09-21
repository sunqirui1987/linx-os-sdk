/**
 * @file linx_sdk.c
 * @brief Linx SDK - 智能语音交互SDK实现
 * @version 1.0.0
 * @date 2024
 * 
 * 本文件实现了 linx_sdk.h 中定义的所有接口，提供简化的智能语音交互功能。
 */

#include "linx_sdk.h"
#include "log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// ============================================================================
// 内部函数声明
// ============================================================================

static void _linx_sdk_set_state(LinxSdk* sdk, LinxDeviceState new_state);
static void _linx_sdk_emit_event(LinxSdk* sdk, const LinxEvent* event);
static void _linx_sdk_set_error(LinxSdk* sdk, const char* error_msg, int error_code);

// WebSocket回调函数
static void _linx_sdk_on_websocket_connected(void* user_data);
static void _linx_sdk_on_websocket_disconnected(void* user_data);
static void _linx_sdk_on_websocket_error(const char* error_msg, void* user_data);
static void _linx_sdk_on_websocket_message(const cJSON* root, void* user_data);
static void _linx_sdk_on_websocket_audio_data(linx_audio_stream_packet_t* packet, void* user_data);

// 事件处理线程
static void* _linx_sdk_event_thread(void* arg);

// 状态管理函数
static void _linx_sdk_set_session_id(LinxSdk* sdk, const char* session_id);
static void _linx_sdk_set_listen_state(LinxSdk* sdk, const char* state);
static void _linx_sdk_set_tts_state(LinxSdk* sdk, const char* state);

// 内部监听控制函数 (预留接口)

// MCP回调函数
static void _linx_sdk_mcp_send_callback(const char* message);

// ============================================================================
// 核心API函数实现
// ============================================================================

LinxSdk* linx_sdk_create(const LinxSdkConfig* config) {
    if (!config) {
        return NULL;
    }
    
    // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_DEBUG;  // 默认INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return NULL;
    }
    
    LOG_INFO("开始创建LinxSDK实例");
    
    LinxSdk* sdk = (LinxSdk*)calloc(1, sizeof(LinxSdk));
    if (!sdk) {
        LOG_ERROR("SDK内存分配失败");
        return NULL;
    }
    
    // 复制配置
    memcpy(&sdk->config, config, sizeof(LinxSdkConfig));
    
    // 设置默认值
    if (sdk->config.sample_rate == 0) {
        sdk->config.sample_rate = 16000;
    }
    if (sdk->config.channels == 0) {
        sdk->config.channels = 1;
    }
    if (sdk->config.timeout_ms == 0) {
        sdk->config.timeout_ms = 30000;
    }
    
    // 初始化状态
    sdk->state = LINX_DEVICE_STATE_IDLE;
    sdk->initialized = true;
    sdk->connected = false;
    sdk->event_callback = NULL;
    sdk->user_data = NULL;
    sdk->connect_time = 0;
    sdk->message_count = 0;
    
    // 初始化WebSocket相关字段
    sdk->ws_protocol = NULL;
    sdk->event_thread_running = false;
    sdk->session_id = NULL;
    sdk->listen_state = NULL;
    sdk->tts_state = NULL;
    pthread_mutex_init(&sdk->state_mutex, NULL);
    
    // 初始化MCP相关字段
    sdk->mcp_server = NULL;

    memset(sdk->last_error, 0, sizeof(sdk->last_error));
    
    // 创建MCP服务器（如果启用）
    sdk->mcp_server = mcp_server_create("LinxSDK", "1.0.0");
    if (!sdk->mcp_server) {
        LOG_WARN("MCP服务器创建失败");
    } else {
        // 设置MCP消息发送回调
        mcp_server_set_send_callback(_linx_sdk_mcp_send_callback);
        LOG_INFO("MCP服务器创建成功");
    }
    
    return sdk;
}

void linx_sdk_destroy(LinxSdk* sdk) {
    if (!sdk) {
        return;
    }
    
    // 断开连接
    if (sdk->connected) {
        linx_sdk_disconnect(sdk);
    }
    
    // 停止事件处理线程
    if (sdk->event_thread_running) {
        sdk->event_thread_running = false;
        pthread_join(sdk->event_thread, NULL);
    }
    
    // 清理WebSocket协议
    if (sdk->ws_protocol) {
        linx_websocket_destroy((linx_protocol_t*)sdk->ws_protocol);
        sdk->ws_protocol = NULL;
    }
    
    // 清理MCP服务器
    if (sdk->mcp_server) {
        mcp_server_destroy(sdk->mcp_server);
        sdk->mcp_server = NULL;
    }
    
    // 清理字符串资源
    if (sdk->session_id) {
        free(sdk->session_id);
    }
    if (sdk->listen_state) {
        free(sdk->listen_state);
    }
    if (sdk->tts_state) {
        free(sdk->tts_state);
    }
    
    // 销毁互斥锁
    pthread_mutex_destroy(&sdk->state_mutex);
    
    LOG_INFO("LinxSDK实例已销毁");
    
    // 清理日志系统
    log_cleanup();
    
    free(sdk);
}

LinxSdkError linx_sdk_set_event_callback(LinxSdk* sdk, LinxEventCallback callback, void* user_data) {
    if (!sdk) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->initialized) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
    
    sdk->event_callback = callback;
    sdk->user_data = user_data;
    
    return LINX_SDK_SUCCESS;
}

LinxSdkError linx_sdk_connect(LinxSdk* sdk) {
    if (!sdk) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->initialized) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
    
    if (sdk->connected) {
        return LINX_SDK_SUCCESS;
    }
    
    _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_CONNECTING);
    
    LOG_INFO("正在连接到服务器: %s", sdk->config.server_url);
    
    // 检查服务器URL
    if (strlen(sdk->config.server_url) == 0) {
        _linx_sdk_set_error(sdk, "服务器URL为空", LINX_SDK_ERROR_INVALID_PARAM);
        _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_ERROR);
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    // 创建WebSocket协议实例
    linx_websocket_config_t ws_config = {
        .url = sdk->config.server_url,
        .auth_token = strlen(sdk->config.auth_token) > 0 ? sdk->config.auth_token : NULL,
        .device_id = strlen(sdk->config.device_id) > 0 ? sdk->config.device_id : NULL,
        .client_id = strlen(sdk->config.client_id) > 0 ? sdk->config.client_id : NULL,
        .protocol_version = sdk->config.protocol_version > 0 ? sdk->config.protocol_version : 1
    };
    
    sdk->ws_protocol = linx_websocket_protocol_create(&ws_config);
    if (!sdk->ws_protocol) {
        _linx_sdk_set_error(sdk, "WebSocket协议创建失败", LINX_SDK_ERROR_NETWORK);
        _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_ERROR);
        return LINX_SDK_ERROR_NETWORK;
    }
    
    // 设置WebSocket回调函数
    linx_protocol_callbacks_t callbacks = {
        .on_connected = _linx_sdk_on_websocket_connected,
        .on_disconnected = _linx_sdk_on_websocket_disconnected,
        .on_network_error = _linx_sdk_on_websocket_error,
        .on_incoming_json = _linx_sdk_on_websocket_message,
        .on_incoming_audio = _linx_sdk_on_websocket_audio_data,
        .user_data = sdk
    };
    linx_protocol_set_callbacks((linx_protocol_t*)sdk->ws_protocol, &callbacks);
    
    // 启动WebSocket连接
    if (!linx_websocket_start((linx_protocol_t*)sdk->ws_protocol)) {
        _linx_sdk_set_error(sdk, "WebSocket连接启动失败", LINX_SDK_ERROR_NETWORK);
        _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_ERROR);
        linx_websocket_destroy((linx_protocol_t*)sdk->ws_protocol);
        sdk->ws_protocol = NULL;
        return LINX_SDK_ERROR_NETWORK;
    }
    
    // 启动事件处理线程
    sdk->event_thread_running = true;
    if (pthread_create(&sdk->event_thread, NULL, _linx_sdk_event_thread, sdk) != 0) {
        _linx_sdk_set_error(sdk, "事件处理线程创建失败", LINX_SDK_ERROR_UNKNOWN);
        _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_ERROR);
        sdk->event_thread_running = false;
        linx_websocket_destroy((linx_protocol_t*)sdk->ws_protocol);
        sdk->ws_protocol = NULL;
        return LINX_SDK_ERROR_UNKNOWN;
    }
    
    LOG_INFO("WebSocket连接启动成功，等待连接建立...");
    return LINX_SDK_SUCCESS;
}

LinxSdkError linx_sdk_disconnect(LinxSdk* sdk) {
    if (!sdk) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->initialized) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
    
    if (!sdk->connected) {
        return LINX_SDK_SUCCESS;
    }
    
    LOG_INFO("正在断开连接...");
    
    // 停止事件处理线程
    if (sdk->event_thread_running) {
        sdk->event_thread_running = false;
        pthread_join(sdk->event_thread, NULL);
    }
    
    // 停止WebSocket连接
    if (sdk->ws_protocol) {
        linx_websocket_stop(sdk->ws_protocol);
    }
    
    sdk->connected = false;
    sdk->connect_time = 0;
    _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_IDLE);
    
    LOG_INFO("连接已断开");
    return LINX_SDK_SUCCESS;
}

LinxSdkError linx_sdk_send_text(LinxSdk* sdk, const char* text) {
    if (!sdk || !text) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->initialized) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
    
    if (!sdk->connected) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    LOG_INFO("发送文本消息: %s", text);
    
    // 增加消息计数
    sdk->message_count++;
    
    if (!linx_websocket_send_text((linx_protocol_t*)sdk->ws_protocol, text)) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    return LINX_SDK_SUCCESS;
}

LinxSdkError linx_sdk_send_audio(LinxSdk* sdk, const uint8_t* data, size_t size) {
    if (!sdk || !data || size == 0) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->initialized) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
    
    if (!sdk->connected) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    LOG_DEBUG("发送音频数据: %zu 字节", size);
    
    // 创建音频数据包并发送
    linx_audio_stream_packet_t packet = {
        .payload = (uint8_t*)data,
        .payload_size = size
    };
    
    if (!linx_websocket_send_audio((linx_protocol_t*)sdk->ws_protocol, &packet)) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    return LINX_SDK_SUCCESS;
}

LinxDeviceState linx_sdk_get_state(LinxSdk* sdk) {
    if (!sdk) {
        return LINX_DEVICE_STATE_ERROR;
    }
    
    return sdk->state;
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 设置SDK设备状态并触发状态变化事件
 * 
 * 这是一个内部函数，用于安全地更新SDK的设备状态，并自动触发状态变化事件
 * 通知应用程序状态的改变。该函数确保状态变化的原子性和事件的一致性。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param new_state 新的设备状态
 * 
 * @note 该函数是线程安全的，可以在多线程环境中调用
 * @note 如果sdk为NULL，函数会安全返回而不执行任何操作
 * @note 状态变化会自动记录到日志系统中
 * 
 * @see LinxDeviceState
 * @see _linx_sdk_emit_event
 * @see LINX_EVENT_STATE_CHANGED
 */
static void _linx_sdk_set_state(LinxSdk* sdk, LinxDeviceState new_state) {
    if (!sdk || sdk->state == new_state) {
        return;
    }
    
    LinxDeviceState old_state = sdk->state;
    sdk->state = new_state;
    
    LOG_DEBUG("状态变化: %d -> %d", old_state, new_state);
    
    // 发送状态变化事件
    if (sdk->event_callback) {
        LinxEvent event = {0};
        event.type = LINX_EVENT_STATE_CHANGED;
        event.timestamp = time(NULL);
        event.data.state_changed.old_state = old_state;
        event.data.state_changed.new_state = new_state;
        
        _linx_sdk_emit_event(sdk, &event);
    }
}

/**
 * @brief 向应用程序发射事件通知
 * 
 * 这是一个内部函数，用于安全地向应用程序发送事件通知。该函数会检查
 * 所有必要的条件（SDK实例、事件对象、回调函数），确保事件能够正确传递。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param event 指向要发送的事件对象的指针，不能为NULL
 * 
 * @note 该函数是线程安全的，可以在多线程环境中调用
 * @note 如果sdk、event为NULL或未设置事件回调，函数会安全返回
 * @note 事件回调在调用线程的上下文中执行，应用程序需要确保回调函数的线程安全性
 * 
 * @see LinxEvent
 * @see LinxEventCallback
 * @see linx_sdk_set_event_callback
 */
static void _linx_sdk_emit_event(LinxSdk* sdk, const LinxEvent* event) {
    if (!sdk || !event || !sdk->event_callback) {
        return;
    }
    
    sdk->event_callback(event, sdk->user_data);
}

/**
 * @brief 设置SDK错误信息并触发错误事件
 * 
 * 这是一个内部函数，用于记录SDK运行过程中发生的错误，并向应用程序
 * 发送错误事件通知。该函数会安全地处理错误消息的存储和事件的发送。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param error_msg 错误消息字符串，可以为NULL
 * @param error_code 错误代码，通常对应LinxSdkError枚举值
 * 
 * @note 该函数是线程安全的，可以在多线程环境中调用
 * @note 错误消息会被截断以适应内部缓冲区大小，并确保字符串正确终止
 * @note 如果error_msg为NULL，将使用"未知错误"作为默认消息
 * @note 错误信息会自动记录到日志系统中
 * 
 * @see LinxSdkError
 * @see LINX_EVENT_ERROR
 * @see _linx_sdk_emit_event
 */
static void _linx_sdk_set_error(LinxSdk* sdk, const char* error_msg, int error_code) {
    if (!sdk) {
        return;
    }
    
    // 保存错误信息
    if (error_msg) {
        strncpy(sdk->last_error, error_msg, sizeof(sdk->last_error) - 1);
        sdk->last_error[sizeof(sdk->last_error) - 1] = '\0';
    }
    
    LOG_ERROR("错误: %s (代码: %d)", error_msg ? error_msg : "未知错误", error_code);
    
    // 发送错误事件
    if (sdk->event_callback) {
        LinxEvent event = {0};
        event.type = LINX_EVENT_ERROR;
        event.timestamp = time(NULL);
        event.data.error.message = sdk->last_error;
        event.data.error.code = error_code;
        
        _linx_sdk_emit_event(sdk, &event);
    }
}

bool linx_sdk_is_connected(LinxSdk* sdk) {
    if (!sdk) {
        return false;
    }
    
    return sdk->connected;
}

uint32_t linx_sdk_get_message_count(LinxSdk* sdk) {
    if (!sdk) {
        return 0;
    }
    
    return sdk->message_count;
}

time_t linx_sdk_get_connect_time(LinxSdk* sdk) {
    if (!sdk) {
        return 0;
    }
    return sdk->connect_time;
}

// WebSocket回调函数实现
/**
 * @brief WebSocket连接成功回调函数
 * 
 * 当WebSocket连接成功建立时，该回调函数会被调用。函数会更新SDK的连接状态，
 * 记录连接时间，设置设备状态为监听状态，并向应用程序发送连接成功事件。
 * 
 * @param user_data 用户数据指针，应该指向LinxSdk实例
 * 
 * @note 该函数在WebSocket线程上下文中被调用
 * @note 如果user_data为NULL，函数会安全返回
 * @note 连接成功后设备状态会自动切换到LINX_DEVICE_STATE_LISTENING
 * @note 连接时间会被记录用于统计和调试
 * 
 * @see LINX_EVENT_WEBSOCKET_CONNECTED
 * @see LINX_DEVICE_STATE_LISTENING
 * @see _linx_sdk_set_state
 */
static void _linx_sdk_on_websocket_connected(void* user_data) {
    LinxSdk* sdk = (LinxSdk*)user_data;
    if (!sdk) return;
    
    sdk->connected = true;
    sdk->connect_time = time(NULL);
    _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_LISTENING);
    
    // 触发连接成功事件
    LinxEvent event = {
        .type = LINX_EVENT_WEBSOCKET_CONNECTED,
        .timestamp = time(NULL)
    };
    
    if (sdk->event_callback) {
        sdk->event_callback(&event, sdk->user_data);
    }
    
    LOG_INFO("WebSocket连接成功");
}

/**
 * @brief WebSocket连接断开回调函数
 * 
 * 当WebSocket连接断开时，该回调函数会被调用。函数会更新SDK的连接状态，
 * 设置设备状态为断开状态，并向应用程序发送断开连接事件。
 * 
 * @param user_data 用户数据指针，应该指向LinxSdk实例
 * 
 * @note 该函数在WebSocket线程上下文中被调用
 * @note 如果user_data为NULL，函数会安全返回
 * @note 断开连接后设备状态会自动切换到LINX_DEVICE_STATE_DISCONNECTED
 * @note 应用程序应该监听此事件以处理重连逻辑
 * 
 * @see LINX_EVENT_WEBSOCKET_DISCONNECTED
 * @see LINX_DEVICE_STATE_DISCONNECTED
 * @see _linx_sdk_set_state
 */
static void _linx_sdk_on_websocket_disconnected(void* user_data) {
    LinxSdk* sdk = (LinxSdk*)user_data;
    if (!sdk) return;
    
    sdk->connected = false;
    _linx_sdk_set_state(sdk, LINX_DEVICE_STATE_DISCONNECTED);
    
    // 触发断开连接事件
    LinxEvent event = {
        .type = LINX_EVENT_WEBSOCKET_DISCONNECTED,
        .timestamp = time(NULL)
    };
    
    if (sdk->event_callback) {
        sdk->event_callback(&event, sdk->user_data);
    }
    
    LOG_INFO("WebSocket连接已断开");
}

/**
 * @brief WebSocket错误回调函数
 * 
 * 当WebSocket连接发生错误时，该回调函数会被调用。函数会记录错误信息
 * 并向应用程序发送错误事件通知。
 * 
 * @param error_msg 错误消息字符串，可能为NULL
 * @param user_data 用户数据指针，应该指向LinxSdk实例
 * 
 * @note 该函数在WebSocket线程上下文中被调用
 * @note 如果user_data为NULL，函数会安全返回
 * @note 错误信息会被记录到SDK的错误状态中
 * @note 应用程序应该监听错误事件以处理异常情况
 * 
 * @see LINX_SDK_ERROR_WEBSOCKET
 * @see _linx_sdk_set_error
 * @see LINX_EVENT_ERROR
 */
static void _linx_sdk_on_websocket_error(const char* error_msg, void* user_data) {
    LinxSdk* sdk = (LinxSdk*)user_data;
    if (!sdk) return;
    
    _linx_sdk_set_error(sdk, error_msg, LINX_SDK_ERROR_WEBSOCKET);
}

static void _linx_sdk_on_websocket_message(const cJSON* root, void* user_data) {
    LinxSdk* sdk = (LinxSdk*)user_data;
    if (!sdk || !root) return;
    
    char* json_string = cJSON_Print(root);
    if (!json_string) return;
    
    LOG_DEBUG("收到WebSocket消息: %s", json_string);
    
    // 获取消息类型
    const cJSON* type = cJSON_GetObjectItem(root, "type");
    if (!type || !cJSON_IsString(type)) {
        LOG_ERROR("消息类型缺失或无效");
        free(json_string);
        return;
    }
    
    // 处理hello响应
    if (strcmp(type->valuestring, "hello") == 0) {
        const cJSON* session_id = cJSON_GetObjectItem(root, "session_id");
        if (session_id && cJSON_IsString(session_id)) {
            _linx_sdk_set_session_id(sdk, session_id->valuestring);
            LOG_INFO("会话建立，ID: %s", session_id->valuestring);
            
            // 触发会话建立事件
            LinxEvent event = {
                .type = LINX_EVENT_SESSION_ESTABLISHED,
                .timestamp = time(NULL),
                .data.session_established = {
                    .session_id = session_id->valuestring
                }
            };
            
            if (sdk->event_callback) {
                sdk->event_callback(&event, sdk->user_data);
            }
            
            // 自动开始监听（如果配置了音频通道）
            _linx_sdk_set_listen_state(sdk, "start");
            linx_protocol_send_start_listening((linx_protocol_t*)sdk->ws_protocol, sdk->config.listening_mode);
            LOG_INFO("开始语音监听");
            
            // 触发监听开始事件
            LinxEvent listen_event = {
                .type = LINX_EVENT_LISTENING_STARTED,
                .timestamp = time(NULL)
            };
            
            if (sdk->event_callback) {
                sdk->event_callback(&listen_event, sdk->user_data);
            }
        }
    }
    // 处理TTS状态
    else if (strcmp(type->valuestring, "tts") == 0) {
        const cJSON* state = cJSON_GetObjectItem(root, "state");
        if (state && cJSON_IsString(state)) {
            _linx_sdk_set_tts_state(sdk, state->valuestring);
            LOG_INFO("TTS状态: %s", state->valuestring);
            
            if (strcmp(state->valuestring, "start") == 0) {
                // TTS开始播放，停止监听避免回音
                _linx_sdk_set_listen_state(sdk, "stop");
                if (sdk->ws_protocol) {
                    linx_protocol_send_stop_listening((linx_protocol_t*)sdk->ws_protocol);
                }
                LOG_INFO("停止监听（TTS播放中）");
                
                // 触发TTS开始事件
                LinxEvent event = {
                    .type = LINX_EVENT_TTS_STARTED,
                    .timestamp = time(NULL)
                };
                
                if (sdk->event_callback) {
                    sdk->event_callback(&event, sdk->user_data);
                }
            } else if (strcmp(state->valuestring, "stop") == 0) {
                // TTS播放结束，重新开始监听
                _linx_sdk_set_listen_state(sdk, "start");
                if (sdk->ws_protocol) {
                    linx_protocol_send_start_listening((linx_protocol_t*)sdk->ws_protocol, sdk->config.listening_mode);
                }
                LOG_INFO("恢复语音监听");
                
                // 触发TTS停止事件
                LinxEvent event = {
                    .type = LINX_EVENT_TTS_STOPPED,
                    .timestamp = time(NULL)
                };
                
                if (sdk->event_callback) {
                    sdk->event_callback(&event, sdk->user_data);
                }
            }
        }
    }
    // 处理goodbye消息
    else if (strcmp(type->valuestring, "goodbye") == 0) {
        LOG_INFO("会话结束");
        _linx_sdk_set_session_id(sdk, NULL);
        
        // 触发会话结束事件
        LinxEvent event = {
            .type = LINX_EVENT_SESSION_ENDED,
            .timestamp = time(NULL)
        };
        
        if (sdk->event_callback) {
            sdk->event_callback(&event, sdk->user_data);
        }
    }
    // 处理其他消息类型
    else {
        LOG_WARN("未知消息类型: %s", type->valuestring);
    }
    
    // 如果启用了MCP，将消息传递给MCP服务器处理
    if (sdk->mcp_enabled && sdk->mcp_server) {
        mcp_server_parse_message(sdk->mcp_server, json_string);
    }
    
    free(json_string);
}

/**
 * @brief WebSocket音频数据回调函数
 * 
 * 当接收到WebSocket音频数据包时，该回调函数会被调用。函数会处理音频数据
 * 并触发相应的TTS事件通知应用程序。
 * 
 * @param packet 指向音频数据包的指针，包含音频数据和大小信息
 * @param user_data 用户数据指针，应该指向LinxSdk实例
 * 
 * @note 该函数在WebSocket线程上下文中被调用
 * @note 如果packet或user_data为NULL，函数会安全返回
 * @note 音频数据的处理可以扩展为实际的音频播放功能
 * @note 接收到音频数据会触发LINX_EVENT_AUDIO_DATA事件
 * 
 * @see linx_audio_stream_packet_t
 * @see LINX_EVENT_AUDIO_DATA
 * @see LinxEvent
 */
static void _linx_sdk_on_websocket_audio_data(linx_audio_stream_packet_t* packet, void* user_data) {
    LinxSdk* sdk = (LinxSdk*)user_data;
    if (!sdk || !packet) return;
    
    LOG_DEBUG("收到音频数据: %zu 字节", packet->payload_size);
    
    // 这里可以处理音频数据，例如播放TTS音频
    // 触发TTS相关事件
    LinxEvent event = {
        .type = LINX_EVENT_AUDIO_DATA,
        .timestamp = time(NULL)
    };
    
    if (sdk->event_callback) {
        sdk->event_callback(&event, sdk->user_data);
    }
}

/**
 * @brief SDK事件处理线程函数
 * 
 * 这是SDK的主要事件处理线程，负责轮询WebSocket连接并处理网络事件。
 * 线程会持续运行直到SDK被销毁或明确停止。
 * 
 * @param arg 线程参数，应该指向LinxSdk实例
 * @return void* 线程返回值，总是返回NULL
 * 
 * @note 该函数在独立的线程中运行
 * @note 线程会以10ms的间隔轮询WebSocket事件
 * @note 如果arg为NULL，线程会立即退出
 * @note 线程的运行状态由sdk->event_thread_running控制
 * @note 使用usleep避免过高的CPU占用
 * 
 * @see linx_websocket_poll
 * @see LinxSdk::event_thread_running
 */
static void* _linx_sdk_event_thread(void* arg) {
    LinxSdk* sdk = (LinxSdk*)arg;
    if (!sdk) return NULL;
    
    while (sdk->event_thread_running) {
        // 轮询WebSocket协议
        if (sdk->ws_protocol) {
            linx_websocket_poll(sdk->ws_protocol, 10);
        }
        
        // 短暂休眠避免CPU占用过高
        usleep(10000); // 10ms
    }
    
    return NULL;
}

// 状态管理函数
/**
 * @brief 设置SDK会话ID
 * 
 * 这是一个线程安全的内部函数，用于设置或清除SDK的会话ID。
 * 会话ID用于标识与服务器的特定会话连接。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param session_id 会话ID字符串，可以为NULL表示清除会话ID
 * 
 * @note 该函数是线程安全的，使用互斥锁保护会话ID的设置
 * @note 如果sdk为NULL，函数会安全返回
 * @note 会话ID会被动态分配内存存储
 * @note 设置会话ID的操作会被记录到日志中
 * 
 * @see LinxSdk::session_id
 * @see LinxSdk::state_mutex
 */
static void _linx_sdk_set_session_id(LinxSdk* sdk, const char* session_id) {
    if (!sdk) return;
    
    pthread_mutex_lock(&sdk->state_mutex);
    
    if (sdk->session_id) {
        free(sdk->session_id);
        sdk->session_id = NULL;
    }
    
    if (session_id) {
        sdk->session_id = strdup(session_id);
    }
    
    pthread_mutex_unlock(&sdk->state_mutex);
    
    LOG_INFO("会话ID已设置: %s", session_id ? session_id : "(空)");
}

/**
 * @brief 设置SDK监听状态
 * 
 * 这是一个线程安全的内部函数，用于设置或清除SDK的监听状态字符串。
 * 监听状态反映了当前语音监听的具体状态信息。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param state 监听状态字符串，可以为NULL表示清除状态
 * 
 * @note 该函数是线程安全的，使用互斥锁保护监听状态的设置
 * @note 如果sdk为NULL，函数会安全返回
 * @note 状态字符串会被动态分配内存存储
 * 
 * @see LinxSdk::listen_state
 * @see LinxSdk::state_mutex
 */
static void _linx_sdk_set_listen_state(LinxSdk* sdk, const char* state) {
    if (!sdk) return;
    
    pthread_mutex_lock(&sdk->state_mutex);
    
    if (sdk->listen_state) {
        free(sdk->listen_state);
        sdk->listen_state = NULL;
    }
    
    if (state) {
        sdk->listen_state = strdup(state);
    }
    
    pthread_mutex_unlock(&sdk->state_mutex);
}

/**
 * @brief 设置SDK TTS状态
 * 
 * 这是一个线程安全的内部函数，用于设置或清除SDK的TTS（文本转语音）状态字符串。
 * TTS状态反映了当前语音合成和播放的具体状态信息。
 * 
 * @param sdk 指向LinxSdk实例的指针，不能为NULL
 * @param state TTS状态字符串，可以为NULL表示清除状态
 * 
 * @note 该函数是线程安全的，使用互斥锁保护TTS状态的设置
 * @note 如果sdk为NULL，函数会安全返回
 * @note 状态字符串会被动态分配内存存储
 * 
 * @see LinxSdk::tts_state
 * @see LinxSdk::state_mutex
 */
static void _linx_sdk_set_tts_state(LinxSdk* sdk, const char* state) {
    if (!sdk) return;
    
    pthread_mutex_lock(&sdk->state_mutex);
    
    if (sdk->tts_state) {
        free(sdk->tts_state);
        sdk->tts_state = NULL;
    }
    
    if (state) {
        sdk->tts_state = strdup(state);
    }
    
    pthread_mutex_unlock(&sdk->state_mutex);
}

// 预留的监听控制函数接口，待后续实现

/**
 * @brief MCP消息发送回调函数
 * 
 * 这是一个内部回调函数，用于处理从MCP（Model Context Protocol）
 * 服务器需要发送的消息。由于回调函数签名的限制，无法直接获取SDK实例，
 * 实际实现中可能需要使用全局变量或其他方式来访问SDK上下文。
 * 
 * @param message MCP消息字符串，可能为NULL
 * 
 * @note 该函数在MCP服务器的上下文中被调用
 * @note 如果message为NULL，函数会安全返回
 * @note 接收到的MCP消息会被记录到调试日志中
 * @note 由于回调函数签名限制，暂时无法获取SDK实例进行实际发送
 * @note 实际实现中可能需要重新设计回调机制以支持SDK上下文传递
 * 
 * @see mcp_server_set_send_callback
 * @see LOG_DEBUG
 */
static void _linx_sdk_mcp_send_callback(const char* message) {
    // 这里需要获取sdk实例，但由于回调函数签名限制，暂时留空
    // 实际实现中可能需要使用全局变量或其他方式
    if (message) {
        // 发送MCP消息的逻辑
        LOG_DEBUG("收到MCP消息: %s", message);
    }
}

// ============================================================================
// WebSocket相关函数实现
// ============================================================================

LinxSdkError linx_sdk_abort_speaking(LinxSdk* sdk, linx_abort_reason_t reason) {
    if (!sdk) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->connected || !sdk->ws_protocol) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    linx_protocol_send_abort_speaking((linx_protocol_t*)sdk->ws_protocol, reason);
    
    return LINX_SDK_SUCCESS;
}

LinxSdkError linx_sdk_send_wake_word(LinxSdk* sdk, const char* wake_word) {
    if (!sdk || !wake_word) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->connected || !sdk->ws_protocol) {
        return LINX_SDK_ERROR_NETWORK;
    }
    
    linx_protocol_send_wake_word_detected((linx_protocol_t*)sdk->ws_protocol, wake_word);
    
    return LINX_SDK_SUCCESS;
}

const char* linx_sdk_get_session_id(LinxSdk* sdk) {
    if (!sdk) {
        return NULL;
    }
    
    return sdk->session_id;
}

// ============================================================================
// MCP相关函数实现
// ============================================================================

LinxSdkError linx_sdk_add_mcp_tool(LinxSdk* sdk, const char* name, const char* description,
                                   mcp_property_list_t* properties, mcp_tool_callback_t callback) {
    if (!sdk || !name || !description) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    if (!sdk->mcp_enabled || !sdk->mcp_server) {
        return LINX_SDK_ERROR_NOT_INITIALIZED;
    }
   // 添加MCP工具
    if (!mcp_server_add_simple_tool(sdk->mcp_server, name, description, properties, callback)) {
        return LINX_SDK_ERROR_UNKNOWN;
    }
    
    return LINX_SDK_SUCCESS;
}

// ============================================================================
// 事件处理函数实现
// ============================================================================

LinxSdkError linx_sdk_poll_events(LinxSdk* sdk, int timeout_ms) {
    if (!sdk) {
        return LINX_SDK_ERROR_INVALID_PARAM;
    }
    
    // 在事件线程模式下，这个函数不需要做任何事情
    // 事件处理由专门的线程负责
    (void)timeout_ms; // 避免未使用参数警告
    
    return LINX_SDK_SUCCESS;
}