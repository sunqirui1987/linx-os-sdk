#ifndef LINX_PROTOCOL_H
#define LINX_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../cjson/cJSON.h"
#include "../log/linx_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 音频流数据包结构 */
typedef struct {
    int sample_rate;        // 采样率
    int frame_duration;     // 帧持续时间
    uint32_t timestamp;     // 时间戳
    uint8_t* payload;       // 音频数据载荷
    size_t payload_size;    // 载荷大小
} linx_audio_stream_packet_t;

/* 二进制协议 v2 结构 */
typedef struct __attribute__((packed)) {
    uint16_t version;       // 协议版本
    uint16_t type;          // 消息类型 (0: OPUS, 1: JSON)
    uint32_t reserved;      // 保留字段，供将来使用
    uint32_t timestamp;     // 时间戳（毫秒），用于服务端回声消除
    uint32_t payload_size;  // 载荷大小（字节）
    uint8_t payload[];      // 载荷数据
} linx_binary_protocol2_t;

/* 二进制协议 v3 结构 */
typedef struct __attribute__((packed)) {
    uint8_t type;           // 消息类型
    uint8_t reserved;       // 保留字段
    uint16_t payload_size;  // 载荷大小
    uint8_t payload[];      // 载荷数据
} linx_binary_protocol3_t;

/* 中止原因枚举 */
typedef enum {
    LINX_ABORT_REASON_NONE,                 // 无特定原因
    LINX_ABORT_REASON_WAKE_WORD_DETECTED    // 检测到唤醒词
} linx_abort_reason_t;

/* 监听模式枚举 */
typedef enum {
    LINX_LISTENING_MODE_AUTO_STOP,      // 自动停止模式
    LINX_LISTENING_MODE_MANUAL_STOP,    // 手动停止模式
    LINX_LISTENING_MODE_REALTIME        // 实时模式（需要回声消除支持）
} linx_listening_mode_t;

/* 前向声明 */
typedef struct linx_protocol linx_protocol_t;

/* 回调函数类型定义 */
typedef void (*linx_on_incoming_audio_cb_t)(linx_audio_stream_packet_t* packet, void* user_data);
typedef void (*linx_on_incoming_json_cb_t)(const cJSON* root, void* user_data);
typedef void (*linx_on_network_error_cb_t)(const char* message, void* user_data);
typedef void (*linx_on_connected_cb_t)(void* user_data);
typedef void (*linx_on_disconnected_cb_t)(void* user_data);

/* 回调函数配置结构体 */
typedef struct {
    linx_on_incoming_audio_cb_t on_incoming_audio;      // 接收音频回调
    linx_on_incoming_json_cb_t on_incoming_json;        // 接收JSON回调
    linx_on_network_error_cb_t on_network_error;        // 网络错误回调
    linx_on_connected_cb_t on_connected;                // 连接成功回调
    linx_on_disconnected_cb_t on_disconnected;          // 连接断开回调
    void* user_data;                                    // 用户数据
} linx_protocol_callbacks_t;

/* 协议接口结构（虚函数表） */
typedef struct {
    bool (*start)(linx_protocol_t* protocol);
    bool (*send_audio)(linx_protocol_t* protocol, linx_audio_stream_packet_t* packet);
    bool (*send_text)(linx_protocol_t* protocol, const char* text);
    void (*destroy)(linx_protocol_t* protocol);
} linx_protocol_vtable_t;

/* 协议基础结构 */
struct linx_protocol {
    const linx_protocol_vtable_t* vtable;   // 虚函数表指针
    linx_protocol_callbacks_t callbacks;    // 回调函数配置
    
    /* 协议状态 */
    int server_sample_rate;         // 服务器采样率
    int server_frame_duration;      // 服务器帧持续时间
    bool error_occurred;            // 是否发生错误
    char* session_id;               // 会话ID
    uint64_t last_incoming_time;    // 最后接收数据的时间戳（毫秒）
};

/* 协议管理函数 */
void linx_protocol_init(linx_protocol_t* protocol, const linx_protocol_vtable_t* vtable);
void linx_protocol_destroy(linx_protocol_t* protocol);

/* 回调函数配置 */
void linx_protocol_set_callbacks(linx_protocol_t* protocol, const linx_protocol_callbacks_t* callbacks);

/* 获取器函数 */
int linx_protocol_get_server_sample_rate(const linx_protocol_t* protocol);
int linx_protocol_get_server_frame_duration(const linx_protocol_t* protocol);
const char* linx_protocol_get_session_id(const linx_protocol_t* protocol);

/* 协议操作函数 */
bool linx_protocol_start(linx_protocol_t* protocol);
bool linx_protocol_send_audio(linx_protocol_t* protocol, linx_audio_stream_packet_t* packet);

/* 高级消息发送函数 */
void linx_protocol_send_wake_word_detected(linx_protocol_t* protocol, const char* wake_word);
void linx_protocol_send_start_listening(linx_protocol_t* protocol, linx_listening_mode_t mode);
void linx_protocol_send_stop_listening(linx_protocol_t* protocol);
void linx_protocol_send_abort_speaking(linx_protocol_t* protocol, linx_abort_reason_t reason);
void linx_protocol_send_mcp_message(linx_protocol_t* protocol, const char* message);

/* 工具函数 */
void linx_protocol_set_error(linx_protocol_t* protocol, const char* message);
bool linx_protocol_is_timeout(const linx_protocol_t* protocol);

/* 音频数据包管理 */
linx_audio_stream_packet_t* linx_audio_stream_packet_create(size_t payload_size);
void linx_audio_stream_packet_destroy(linx_audio_stream_packet_t* packet);

#ifdef __cplusplus
}
#endif

#endif /* LINX_PROTOCOL_H */