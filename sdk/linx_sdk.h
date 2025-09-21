/**
 * @file linx_sdk.h
 * @brief Linx SDK - 智能语音交互SDK统一接口
 * @version 1.0.0
 * @date 2024
 * 
 * 本SDK提供了完整的智能语音交互解决方案，整合了音频处理、编解码、
 * MCP协议和WebSocket通信等核心功能模块。
 */

#ifndef LINX_SDK_H
#define LINX_SDK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>

// 引入相关模块

#include "protocols/linx_websocket.h"
#include "mcp/mcp_server.h"
#include "cjson/cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDK版本信息
 */
#define LINX_SDK_VERSION "1.0.0"

/**
 * @brief SDK错误码
 */
typedef enum {
    LINX_SDK_SUCCESS = 0,
    LINX_SDK_ERROR_INVALID_PARAM,
    LINX_SDK_ERROR_NOT_INITIALIZED,
    LINX_SDK_ERROR_NETWORK,
    LINX_SDK_ERROR_WEBSOCKET,
    LINX_SDK_ERROR_MEMORY,
    LINX_SDK_ERROR_UNKNOWN
} LinxSdkError;

/**
 * @brief 设备状态
 */
typedef enum {
    LINX_DEVICE_STATE_IDLE = 0,
    LINX_DEVICE_STATE_CONNECTING,
    LINX_DEVICE_STATE_LISTENING,
    LINX_DEVICE_STATE_SPEAKING,
    LINX_DEVICE_STATE_DISCONNECTED,
    LINX_DEVICE_STATE_ERROR
} LinxDeviceState;

/**
 * @brief SDK配置结构体
 */
typedef struct {
    // 基础配置
    char server_url[256];           ///< 服务器URL
    uint32_t sample_rate;           ///< 采样率 (默认16000)
    uint16_t channels;              ///< 声道数 (默认1)
    uint32_t timeout_ms;            ///< 超时时间(毫秒)
    
    // WebSocket连接配置
    char auth_token[256];           ///< 认证令牌
    char device_id[64];             ///< 设备ID
    char client_id[64];             ///< 客户端ID
    uint32_t protocol_version;      ///< 协议版本
    
    linx_listening_mode_t listening_mode; ///< 监听模式
} LinxSdkConfig;

/**
 * @brief SDK事件类型
 */
typedef enum {
    LINX_EVENT_STATE_CHANGED,       ///< 状态改变
    LINX_EVENT_TEXT_MESSAGE,        ///< 文本消息
    LINX_EVENT_AUDIO_DATA,          ///< 音频数据
    LINX_EVENT_ERROR,               ///< 错误事件

    LINX_EVENT_SENTENCE_START,      ///< 句子开始
    LINX_EVENT_SENTENCE_END,        ///< 句子结束
    LINX_EVENT_EMOTION_MESSAGE, ///< 情感事件 [表情]
    LINX_EVENT_SYSTEM_MESSAGE,  ///< 系统事件 [系统]
    LINX_EVENT_CUSTOM_MESSAGE,  ///< 自定义事件 [自定义]
    
    // WebSocket相关事件
    LINX_EVENT_WEBSOCKET_CONNECTED, ///< WebSocket连接成功
    LINX_EVENT_WEBSOCKET_DISCONNECTED, ///< WebSocket连接断开
    LINX_EVENT_SESSION_ESTABLISHED, ///< 会话建立
    LINX_EVENT_SESSION_ENDED,       ///< 会话结束
    LINX_EVENT_LISTENING_STARTED,   ///< 开始监听
    LINX_EVENT_LISTENING_STOPPED,   ///< 停止监听
    LINX_EVENT_TTS_STARTED,         ///< TTS开始播放
    LINX_EVENT_TTS_STOPPED,         ///< TTS停止播放
    
    // MCP相关事件
    LINX_EVENT_MCP_MESSAGE,         ///< MCP消息

} LinxEventType;

/**
 * @brief SDK事件数据
 */
typedef struct {
    LinxEventType type;
    time_t timestamp;
    union {
        struct {
            LinxDeviceState old_state;
            LinxDeviceState new_state;
        } state_changed;
        
        struct {
            char* text;
            char* role;  // "user", "assistant"
        } text_message;

        struct {
            char* value;
        }custom_message;// 自定义事件

        struct {
            char* value;  
        } emotion;
        
        struct {
            uint8_t* data;
            size_t size;
        } audio_data;
        
        struct {
            char* message;
            int code;
        } error;
        
        struct {
            char* session_id;
        } session_established;
        
        struct {
            char* message;
            char* type;
        } mcp_message;
        
        struct {
            char* message;
        } system_message;
    } data;
} LinxEvent;

/**
 * @brief SDK事件回调函数类型
 */
typedef void (*LinxEventCallback)(const LinxEvent* event, void* user_data);

/**
 * @brief LinxSdk 内部结构体
 */
struct LinxSdk {
    LinxSdkConfig config;                   ///< SDK配置
    LinxDeviceState state;                  ///< 当前状态
    LinxEventCallback event_callback;       ///< 事件回调函数
    void* user_data;                        ///< 用户数据
    
    // 状态管理
    bool initialized;                       ///< 是否已初始化
    bool connected;                         ///< 是否已连接
    char last_error[256];                   ///< 最后错误信息
    
    // 简化的连接状态
    time_t connect_time;                    ///< 连接时间
    uint32_t message_count;                 ///< 消息计数
    
    // WebSocket协议相关
    linx_websocket_protocol_t* ws_protocol; ///< WebSocket协议实例
    pthread_t event_thread;                 ///< 事件处理线程
    bool event_thread_running;              ///< 事件线程运行状态
    char* session_id;                       ///< 会话ID
    char* listen_state;                     ///< 监听状态
    char* tts_state;                        ///< TTS状态
    pthread_mutex_t state_mutex;            ///< 状态互斥锁
    
    // MCP相关
    bool mcp_enabled;                       ///< MCP是否启用
    mcp_server_t* mcp_server;               ///< MCP服务器实例

};

/**
 * @brief LinxSdk类型定义
 */
typedef struct LinxSdk LinxSdk;



// ============================================================================
// 核心API函数
// ============================================================================

/**
 * @brief 创建SDK实例
 * 
 * 创建并初始化一个新的Linx SDK实例，包括日志系统、WebSocket协议和MCP服务器的初始化。
 * 
 * @param config SDK配置参数，包含服务器URL、音频参数、超时设置等
 *               - server_url: 服务器WebSocket地址，格式如 "ws://localhost:8080"
 *               - sample_rate: 音频采样率，推荐16000Hz
 *               - channels: 音频声道数，通常为1（单声道）
 *               - timeout_ms: 网络超时时间，单位毫秒
 *               - listening_mode: 监听模式配置
 * 
 * @return 成功返回SDK实例指针，失败返回NULL
 * 
 * @note 
 * - 调用此函数后，SDK实例处于未连接状态，需要调用linx_sdk_connect()建立连接
 * - 使用完毕后必须调用linx_sdk_destroy()释放资源
 * - 配置参数会被复制到SDK实例中，调用后可以安全释放config参数
 * 
 * @warning 
 * - config参数不能为NULL
 * - server_url必须是有效的WebSocket URL格式
 * 
 * @see linx_sdk_destroy(), linx_sdk_connect()
 * 
 * @example
 * ```c
 * LinxSdkConfig config = {
 *     .server_url = "ws://localhost:8080",
 *     .sample_rate = 16000,
 *     .channels = 1,
 *     .timeout_ms = 5000,
 *     .listening_mode = LINX_LISTENING_CONTINUOUS
 * };
 * 
 * LinxSdk* sdk = linx_sdk_create(&config);
 * if (sdk == NULL) {
 *     // 处理创建失败
 * }
 * ```
 */
LinxSdk* linx_sdk_create(const LinxSdkConfig* config);

/**
 * @brief 销毁SDK实例
 * 
 * 安全地销毁SDK实例，释放所有相关资源，包括网络连接、线程、内存等。
 * 
 * @param sdk 要销毁的SDK实例指针，可以为NULL（此时函数安全返回）
 * 
 * @note 
 * - 此函数会自动断开所有活跃连接
 * - 会停止所有后台线程并等待其安全退出
 * - 释放所有动态分配的内存
 * - 清理日志系统资源
 * - 调用后sdk指针将变为无效，不应再使用
 * 
 * @warning 
 * - 确保在调用此函数前，没有其他线程正在使用该SDK实例
 * - 不要重复调用此函数
 * 
 * @see linx_sdk_create(), linx_sdk_disconnect()
 * 
 * @example
 * ```c
 * LinxSdk* sdk = linx_sdk_create(&config);
 * // ... 使用SDK ...
 * linx_sdk_destroy(sdk);
 * sdk = NULL; // 防止误用
 * ```
 */
void linx_sdk_destroy(LinxSdk* sdk);

/**
 * @brief 设置事件回调函数
 * 
 * 设置SDK事件回调函数，用于接收各种SDK事件通知，如连接状态变化、消息接收等。
 * 
 * @param sdk SDK实例指针
 * @param callback 事件回调函数指针，当有事件发生时会被调用
 *                 函数签名: void callback(const LinxEvent* event, void* user_data)
 *                 可以为NULL，表示取消事件回调
 * @param user_data 用户自定义数据指针，会在回调时传递给callback函数
 *                  可以为NULL
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 设置成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk参数为NULL
 * 
 * @note 
 * - 回调函数在SDK内部线程中执行，应避免长时间阻塞操作
 * - 可以多次调用此函数来更换回调函数
 * - 回调函数中不应调用SDK的销毁函数
 * 
 * @warning 
 * - 回调函数必须是线程安全的
 * - 不要在回调函数中调用linx_sdk_destroy()
 * 
 * @see LinxEventCallback, LinxEvent, LinxEventType
 * 
 * @example
 * ```c
 * void my_event_handler(const LinxEvent* event, void* user_data) {
 *     switch (event->type) {
 *         case LINX_EVENT_WEBSOCKET_CONNECTED:
 *             printf("WebSocket连接成功\n");
 *             break;
 *         case LINX_EVENT_TEXT_MESSAGE:
 *             printf("收到文本: %s\n", event->data.text_message.text);
 *             break;
 *         // ... 处理其他事件
 *     }
 * }
 * 
 * LinxSdk* sdk = linx_sdk_create(&config);
 * linx_sdk_set_event_callback(sdk, my_event_handler, NULL);
 * ```
 */
LinxSdkError linx_sdk_set_event_callback(LinxSdk* sdk, LinxEventCallback callback, void* user_data);

/**
 * @brief 连接到服务器
 * 
 * 建立与服务器的WebSocket连接，启动事件处理线程，初始化会话。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 连接成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk参数为NULL
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未正确初始化
 * - LINX_SDK_ERROR_NETWORK: 网络连接失败
 * 
 * @note 
 * - 连接成功后会触发LINX_EVENT_WEBSOCKET_CONNECTED事件
 * - 连接过程是异步的，函数返回成功不代表连接已完全建立
 * - 如果已经连接，重复调用会返回成功
 * 
 * @warning 
 * - 确保在调用前已设置事件回调函数
 * - 网络连接可能需要一些时间，请耐心等待事件通知
 * 
 * @see linx_sdk_disconnect(), linx_sdk_is_connected()
 * 
 * @example
 * ```c
 * LinxSdk* sdk = linx_sdk_create(&config);
 * linx_sdk_set_event_callback(sdk, event_handler, NULL);
 * 
 * LinxSdkError result = linx_sdk_connect(sdk);
 * if (result != LINX_SDK_SUCCESS) {
 *     printf("连接失败: %d\n", result);
 * }
 * ```
 */
LinxSdkError linx_sdk_connect(LinxSdk* sdk);

/**
 * @brief 断开连接
 * 
 * 断开与服务器的连接，停止事件处理线程，清理会话状态。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 断开成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk参数为NULL
 * 
 * @note 
 * - 断开连接后会触发LINX_EVENT_WEBSOCKET_DISCONNECTED事件
 * - 如果当前未连接，调用此函数是安全的
 * - 断开连接会自动清理会话ID和相关状态
 * 
 * @warning 
 * - 断开连接后，所有未完成的操作都会被取消
 * 
 * @see linx_sdk_connect(), linx_sdk_is_connected()
 * 
 * @example
 * ```c
 * LinxSdkError result = linx_sdk_disconnect(sdk);
 * if (result != LINX_SDK_SUCCESS) {
 *     printf("断开连接失败: %d\n", result);
 * }
 * ```
 */
LinxSdkError linx_sdk_disconnect(LinxSdk* sdk);

/**
 * @brief 发送文本消息
 * 
 * 向服务器发送文本消息，用于文本对话交互。
 * 
 * @param sdk SDK实例指针
 * @param text 要发送的文本内容，必须是UTF-8编码的字符串
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 发送成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk或text参数为NULL
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未连接到服务器
 * - LINX_SDK_ERROR_NETWORK: 网络发送失败
 * 
 * @note 
 * - 文本会被封装为JSON格式发送到服务器
 * - 发送成功不代表服务器已处理，需要通过事件回调获取响应
 * - 支持发送空字符串，但不建议
 * 
 * @warning 
 * - 确保在连接建立后调用此函数
 * - 文本长度不应过长，建议控制在合理范围内
 * 
 * @see linx_sdk_send_audio(), LINX_EVENT_TEXT_MESSAGE
 * 
 * @example
 * ```c
 * LinxSdkError result = linx_sdk_send_text(sdk, "你好，我想了解天气情况");
 * if (result != LINX_SDK_SUCCESS) {
 *     printf("发送文本失败: %d\n", result);
 * }
 * ```
 */
LinxSdkError linx_sdk_send_text(LinxSdk* sdk, const char* text);

/**
 * @brief 发送音频数据
 * 
 * 向服务器发送音频数据，用于语音识别和处理。
 * 
 * @param sdk SDK实例指针
 * @param data 音频数据缓冲区指针
 * @param size 音频数据大小（字节数）
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 发送成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk或data参数为NULL，或size为0
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未连接到服务器
 * - LINX_SDK_ERROR_NETWORK: 网络发送失败
 * 
 * @note 
 * - 音频数据应符合SDK配置中指定的采样率和声道数
 * - 推荐使用PCM格式的音频数据
 * - 数据会被实时发送到服务器进行处理
 * 
 * @warning 
 * - 确保音频数据格式与SDK配置一致
 * - 不要发送过大的音频块，建议分块发送
 * - 确保data指针在函数调用期间有效
 * 
 * @see linx_sdk_send_text(), LINX_EVENT_AUDIO_DATA
 * 
 * @example
 * ```c
 * uint8_t audio_buffer[1024];
 * size_t audio_size = capture_audio(audio_buffer, sizeof(audio_buffer));
 * 
 * LinxSdkError result = linx_sdk_send_audio(sdk, audio_buffer, audio_size);
 * if (result != LINX_SDK_SUCCESS) {
 *     printf("发送音频失败: %d\n", result);
 * }
 * ```
 */
LinxSdkError linx_sdk_send_audio(LinxSdk* sdk, const uint8_t* data, size_t size);

/**
 * @brief 获取当前状态
 * 
 * 获取SDK当前的设备状态。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 当前设备状态，如果sdk为NULL则返回LINX_DEVICE_STATE_ERROR
 * - LINX_DEVICE_STATE_IDLE: 空闲状态
 * - LINX_DEVICE_STATE_CONNECTING: 正在连接
 * - LINX_DEVICE_STATE_LISTENING: 正在监听
 * - LINX_DEVICE_STATE_SPEAKING: 正在播放语音
 * - LINX_DEVICE_STATE_ERROR: 错误状态
 * 
 * @note 
 * - 此函数是线程安全的
 * - 状态变化会通过LINX_EVENT_STATE_CHANGED事件通知
 * 
 * @see LinxDeviceState, LINX_EVENT_STATE_CHANGED
 * 
 * @example
 * ```c
 * LinxDeviceState state = linx_sdk_get_state(sdk);
 * switch (state) {
 *     case LINX_DEVICE_STATE_IDLE:
 *         printf("设备空闲\n");
 *         break;
 *     case LINX_DEVICE_STATE_LISTENING:
 *         printf("正在监听\n");
 *         break;
 *     // ... 处理其他状态
 * }
 * ```
 */
LinxDeviceState linx_sdk_get_state(LinxSdk* sdk);

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查是否已连接
 * 
 * 检查SDK是否已成功连接到服务器。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 
 * - true: 已连接到服务器
 * - false: 未连接或sdk为NULL
 * 
 * @note 
 * - 此函数是线程安全的
 * - 连接状态变化会通过相应事件通知
 * 
 * @see linx_sdk_connect(), linx_sdk_disconnect()
 * 
 * @example
 * ```c
 * if (linx_sdk_is_connected(sdk)) {
 *     printf("已连接，可以发送消息\n");
 *     linx_sdk_send_text(sdk, "Hello");
 * } else {
 *     printf("未连接，需要先连接\n");
 *     linx_sdk_connect(sdk);
 * }
 * ```
 */
bool linx_sdk_is_connected(LinxSdk* sdk);

/**
 * @brief 获取消息计数
 * 
 * 获取SDK自连接以来发送和接收的消息总数。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 消息计数，如果sdk为NULL则返回0
 * 
 * @note 
 * - 计数包括发送和接收的所有消息
 * - 断开连接后重新连接，计数会重置
 * - 此函数是线程安全的
 * 
 * @see linx_sdk_get_connect_time()
 * 
 * @example
 * ```c
 * uint32_t count = linx_sdk_get_message_count(sdk);
 * printf("已处理 %u 条消息\n", count);
 * ```
 */
uint32_t linx_sdk_get_message_count(LinxSdk* sdk);

/**
 * @brief 获取连接时间
 * 
 * 获取SDK最后一次成功连接的时间戳。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 连接时间戳（Unix时间），如果sdk为NULL或未连接则返回0
 * 
 * @note 
 * - 返回的是Unix时间戳（自1970年1月1日以来的秒数）
 * - 每次重新连接都会更新此时间
 * - 此函数是线程安全的
 * 
 * @see linx_sdk_get_message_count(), linx_sdk_is_connected()
 * 
 * @example
 * ```c
 * time_t connect_time = linx_sdk_get_connect_time(sdk);
 * if (connect_time > 0) {
 *     printf("连接时间: %s", ctime(&connect_time));
 * }
 * ```
 */
time_t linx_sdk_get_connect_time(LinxSdk* sdk);

// ============================================================================
// WebSocket相关函数
// ============================================================================


/**
 * @brief 中止语音播放
 * 
 * 立即停止当前正在播放的TTS语音，并恢复语音监听。
 * 
 * @param sdk SDK实例指针
 * @param reason 中止原因，用于日志记录和事件通知
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 中止成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk参数为NULL
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未连接到服务器
 * 
 * @note 
 * - 只有在TTS播放状态下调用才有效果
 * - 中止后会自动恢复语音监听
 * - 会触发相应的状态变化事件
 * 
 * @warning 
 * - 确保在连接状态下调用此函数
 * 
 * @see linx_sdk_get_state(), LINX_EVENT_TTS_STOPPED
 * 
 * @example
 * ```c
 * if (linx_sdk_get_state(sdk) == LINX_DEVICE_STATE_SPEAKING) {
 *     LinxSdkError result = linx_sdk_abort_speaking(sdk, LINX_ABORT_USER_INTERRUPT);
 *     if (result == LINX_SDK_SUCCESS) {
 *         printf("语音播放已中止\n");
 *     }
 * }
 * ```
 */
LinxSdkError linx_sdk_abort_speaking(LinxSdk* sdk, linx_abort_reason_t reason);

/**
 * @brief 发送唤醒词
 * 
 * 向服务器发送唤醒词，用于激活语音交互会话。
 * 
 * @param sdk SDK实例指针
 * @param wake_word 唤醒词字符串，必须是UTF-8编码
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 发送成功
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk或wake_word参数为NULL
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未连接到服务器
 * - LINX_SDK_ERROR_NETWORK: 网络发送失败
 * 
 * @note 
 * - 唤醒词用于启动新的对话会话
 * - 发送成功后可能会收到会话建立事件
 * - 支持自定义唤醒词
 * 
 * @warning 
 * - 确保在连接状态下调用此函数
 * - 唤醒词不应为空字符串
 * 
 * @see LINX_EVENT_SESSION_ESTABLISHED, linx_sdk_send_text()
 * 
 * @example
 * ```c
 * LinxSdkError result = linx_sdk_send_wake_word(sdk, "小助手");
 * if (result == LINX_SDK_SUCCESS) {
 *     printf("唤醒词已发送\n");
 * }
 * ```
 */
LinxSdkError linx_sdk_send_wake_word(LinxSdk* sdk, const char* wake_word);

/**
 * @brief 获取会话ID
 * 
 * 获取当前活跃会话的唯一标识符。
 * 
 * @param sdk SDK实例指针
 * 
 * @return 会话ID字符串，如果sdk为NULL或无活跃会话则返回NULL
 * 
 * @note 
 * - 会话ID在会话建立时生成
 * - 会话结束时会被清空
 * - 返回的字符串由SDK内部管理，不要修改或释放
 * - 此函数是线程安全的
 * 
 * @warning 
 * - 返回的指针可能在会话结束后变为无效
 * - 如需长期保存，请复制字符串内容
 * 
 * @see LINX_EVENT_SESSION_ESTABLISHED, LINX_EVENT_SESSION_ENDED
 * 
 * @example
 * ```c
 * const char* session_id = linx_sdk_get_session_id(sdk);
 * if (session_id) {
 *     printf("当前会话ID: %s\n", session_id);
 * } else {
 *     printf("无活跃会话\n");
 * }
 * ```
 */
const char* linx_sdk_get_session_id(LinxSdk* sdk);



// ============================================================================
// MCP相关函数
// ============================================================================

/**
 * @brief 添加MCP工具
 * 
 * 向SDK注册一个MCP（Model Context Protocol）工具，使AI模型能够调用自定义功能。
 * 
 * @param sdk SDK实例指针
 * @param name 工具名称，必须唯一且符合MCP规范
 * @param description 工具描述，用于AI模型理解工具功能
 * @param properties 工具参数属性列表，定义工具接受的参数
 * @param callback 工具回调函数，当AI模型调用工具时会被执行
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 添加成功
 * - LINX_SDK_ERROR_INVALID_PARAM: 参数无效（sdk、name、description或callback为NULL）
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未正确初始化
 * - LINX_SDK_ERROR_MEMORY: 内存分配失败
 * 
 * @note 
 * - 工具名称必须唯一，重复添加会覆盖原有工具
 * - 工具描述应清晰说明功能，帮助AI模型正确使用
 * - properties可以为NULL，表示工具不需要参数
 * - 回调函数在工具被调用时执行，应避免长时间阻塞
 * 
 * @warning 
 * - 回调函数必须是线程安全的
 * - 工具名称和描述字符串会被复制，调用后可以安全释放
 * 
 * @see mcp_tool_callback_t
 * 
 * @example
 * ```c
 * // 定义工具回调函数
 * char* weather_tool_callback(const char* params, void* user_data) {
 *     // 解析参数并获取天气信息
 *     return strdup("{\"temperature\": 25, \"weather\": \"晴天\"}");
 * }
 * 
 * // 添加天气查询工具
 * LinxSdkError result = linx_sdk_add_mcp_tool(
 *     sdk,
 *     "get_weather",
 *     "获取指定城市的天气信息",
 *     weather_properties,
 *     weather_tool_callback
 * );
 * ```
 */
LinxSdkError linx_sdk_add_mcp_tool(LinxSdk* sdk, const char* name, const char* description,
                                   mcp_property_list_t* properties, mcp_tool_callback_t callback);


// ============================================================================
// 事件处理函数
// ============================================================================

/**
 * @brief 轮询事件
 * 
 * 主动轮询SDK事件，用于在没有设置事件回调或需要同步处理事件的场景。
 * 
 * @param sdk SDK实例指针
 * @param timeout_ms 超时时间（毫秒），-1表示无限等待，0表示立即返回
 * 
 * @return 
 * - LINX_SDK_SUCCESS: 轮询成功（可能有事件或超时）
 * - LINX_SDK_ERROR_INVALID_PARAM: sdk参数为NULL
 * - LINX_SDK_ERROR_NOT_INITIALIZED: SDK未正确初始化
 * 
 * @note 
 * - 此函数会阻塞直到有事件发生或超时
 * - 如果设置了事件回调函数，通常不需要调用此函数
 * - 适用于需要同步事件处理的场景
 * - 超时返回也被视为成功
 * 
 * @warning 
 * - 不要在事件回调函数中调用此函数，可能导致死锁
 * - 长时间轮询会阻塞当前线程
 * 
 * @see linx_sdk_set_event_callback(), LinxEvent
 * 
 * @example
 * ```c
 * // 轮询事件，最多等待1秒
 * LinxSdkError result = linx_sdk_poll_events(sdk, 1000);
 * if (result == LINX_SDK_SUCCESS) {
 *     printf("轮询完成\n");
 * }
 * 
 * // 立即检查是否有事件
 * linx_sdk_poll_events(sdk, 0);
 * ```
 */
LinxSdkError linx_sdk_poll_events(LinxSdk* sdk, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // LINX_SDK_H