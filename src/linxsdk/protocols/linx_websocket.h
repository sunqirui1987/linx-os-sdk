#ifndef LINX_WEBSOCKET_H
#define LINX_WEBSOCKET_H

#include "linx_protocol.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/* WebSocket 协议实现结构体 - 前向声明（隐藏实现细节） */
typedef struct linx_websocket_protocol linx_websocket_protocol_t;

/* WebSocket 配置结构体 */
typedef struct {
    const char* url;                // WebSocket 服务器URL
    const char* host;               // 服务器主机
    int port;                       // 服务器端口
    const char* path;               // 服务器路径
    const char* auth_token;         // 认证令牌
    const char* device_id;          // 设备ID
    const char* client_id;          // 客户端ID
    

     /* 协议状态 */
    char* client_audio_format;       // 客户端音频格式
    int audio_sample_rate;           // 客户端采样率
    int audio_channels;              // 客户端声道数
    int audio_frame_duration;        // 客户端帧持续时间
    int protocol_version;           // 协议版本

} linx_websocket_config_t;

/* 核心接口函数 */

/**
 * 创建并初始化 WebSocket 协议实例
 * @param config WebSocket 配置参数
 * @return 创建的协议实例，失败返回 NULL
 */
linx_websocket_protocol_t* linx_websocket_protocol_create(const linx_websocket_config_t* config);

/* vtable 函数 */
bool linx_websocket_start(linx_protocol_t* protocol);
bool linx_websocket_send_audio(linx_protocol_t* protocol, linx_audio_stream_packet_t* packet);
bool linx_websocket_send_text(linx_protocol_t* protocol, const char* message);

/**
 * 销毁 WebSocket 协议实例
 * @param protocol 要销毁的协议实例
 */
void linx_websocket_destroy(linx_protocol_t* protocol);

/* 事件循环函数 */

/**
 * 轮询 WebSocket 事件
 * @param protocol WebSocket 协议实例
 * @param timeout_ms 超时时间（毫秒）
 */
void linx_websocket_poll(linx_websocket_protocol_t* protocol, int timeout_ms);

/**
 * 停止 WebSocket 连接
 * @param protocol WebSocket 协议实例
 */
void linx_websocket_stop(linx_websocket_protocol_t* protocol);

/* 工具函数 */

/**
 * 检查 WebSocket 连接状态
 * @param protocol WebSocket 协议实例
 * @return 连接状态，true 表示已连接
 */
bool linx_websocket_is_connected(const linx_websocket_protocol_t* protocol);

/**
 * 获取重连尝试次数
 * @param protocol WebSocket 协议实例
 * @return 重连尝试次数
 */
int linx_websocket_get_reconnect_attempts(const linx_websocket_protocol_t* protocol);

/**
 * 重置重连尝试次数
 * @param protocol WebSocket 协议实例
 */
void linx_websocket_reset_reconnect_attempts(linx_websocket_protocol_t* protocol);

/**
 * 处理 WebSocket 事件
 * @param protocol WebSocket 协议实例
 */
void linx_websocket_process_events(linx_websocket_protocol_t* protocol);

/**
 * 发送 ping 消息
 * @param protocol WebSocket 协议实例
 * @return 发送成功返回 true
 */
bool linx_websocket_send_ping(linx_websocket_protocol_t* protocol);

/**
 * 检查连接是否超时
 * @param protocol WebSocket 协议实例
 * @return 超时返回 true
 */
bool linx_websocket_is_connection_timeout(const linx_websocket_protocol_t* protocol);

/**
 * 创建 WebSocket 协议实例（别名函数）
 * @param config WebSocket 配置参数
 * @return 创建的协议实例，失败返回 NULL
 */
linx_websocket_protocol_t* linx_websocket_create(const linx_websocket_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* LINX_WEBSOCKET_H */