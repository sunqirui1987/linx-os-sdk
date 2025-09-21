#include "linx_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../log/linx_log.h"

#define LINX_TIMEOUT_MS 120000  /* 120秒超时 */

/* 获取当前时间戳（毫秒） */
static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* 协议管理函数 */
void linx_protocol_init(linx_protocol_t* protocol, const linx_protocol_vtable_t* vtable) {
    LOG_DEBUG("Initializing protocol with vtable: %p", vtable);
    
    if (!protocol || !vtable) {
        LOG_ERROR("Invalid parameters: protocol=%p, vtable=%p", protocol, vtable);
        return;
    }
    
    memset(protocol, 0, sizeof(linx_protocol_t));
    protocol->vtable = vtable;
    protocol->server_sample_rate = 24000;      // 默认采样率24kHz
    protocol->server_frame_duration = 60;      // 默认帧持续时间60ms
    protocol->error_occurred = false;
    protocol->session_id = NULL;
    protocol->last_incoming_time = get_current_time_ms();
    
    LOG_INFO("Protocol initialized successfully - sample_rate: %d, frame_duration: %d", 
             protocol->server_sample_rate, protocol->server_frame_duration);
}

void linx_protocol_destroy(linx_protocol_t* protocol) {
    if (!protocol) {
        LOG_WARN("Attempting to destroy NULL protocol");
        return;
    }
    
    LOG_DEBUG("Destroying protocol with session_id: %s", 
              protocol->session_id ? protocol->session_id : "NULL");
    
    // 释放会话ID内存
    if (protocol->session_id) {
        protocol->session_id = NULL;
    }
    
    // 调用具体协议的销毁函数
    if (protocol->vtable && protocol->vtable->destroy) {
        protocol->vtable->destroy(protocol);
    }
    
    LOG_INFO("Protocol destroyed successfully");
}

/* 获取器函数 */
int linx_protocol_get_server_sample_rate(const linx_protocol_t* protocol) {
    return protocol ? protocol->server_sample_rate : 0;
}

int linx_protocol_get_server_frame_duration(const linx_protocol_t* protocol) {
    return protocol ? protocol->server_frame_duration : 0;
}

const char* linx_protocol_get_session_id(const linx_protocol_t* protocol) {
    return protocol ? protocol->session_id : NULL;
}

/* 回调函数配置 */
void linx_protocol_set_callbacks(linx_protocol_t* protocol, const linx_protocol_callbacks_t* callbacks) {
    if (protocol && callbacks) {
        protocol->callbacks = *callbacks;
    }
}

/* 协议操作函数 */
bool linx_protocol_start(linx_protocol_t* protocol) {
    LOG_DEBUG("Starting protocol");
    
    if (!protocol || !protocol->vtable || !protocol->vtable->start) {
        LOG_ERROR("Invalid protocol or missing start function: protocol=%p", protocol);
        return false;
    }
    
    bool result = protocol->vtable->start(protocol);
    if (result) {
        LOG_INFO("Protocol started successfully");
    } else {
        LOG_ERROR("Failed to start protocol");
    }
    
    return result;
}

bool linx_protocol_send_audio(linx_protocol_t* protocol, linx_audio_stream_packet_t* packet) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_audio) {
        LOG_ERROR("Invalid protocol or missing send_audio function: protocol=%p", protocol);
        return false;
    }
    
    if (!packet) {
        LOG_ERROR("Audio packet is NULL");
        return false;
    }
    
    LOG_DEBUG("Sending audio packet - size: %zu, sample_rate: %d, timestamp: %u", 
              packet->payload_size, packet->sample_rate, packet->timestamp);
    
    bool result = protocol->vtable->send_audio(protocol, packet);
    if (!result) {
        LOG_WARN("Failed to send audio packet");
    }
    
    return result;
}

/* 高级消息发送函数 */
void linx_protocol_send_wake_word_detected(linx_protocol_t* protocol, const char* wake_word) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_text || !wake_word) {
        return;
    }
    
    char message[512];
    snprintf(message, sizeof(message), 
             "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"detect\",\"text\":\"%s\"}", 
             protocol->session_id ? protocol->session_id : "", wake_word);
    
    protocol->vtable->send_text(protocol, message);
}

void linx_protocol_send_start_listening(linx_protocol_t* protocol, linx_listening_mode_t mode) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_text) {
        return;
    }
    
    const char* mode_str;
    switch (mode) {
        case LINX_LISTENING_MODE_AUTO_STOP:
            mode_str = "auto";
            break;
        case LINX_LISTENING_MODE_MANUAL_STOP:
            mode_str = "manual";
            break;
        case LINX_LISTENING_MODE_REALTIME:
            mode_str = "realtime";
            break;
        default:
            mode_str = "auto";
            break;
    }
    
    char message[256];
    snprintf(message, sizeof(message), 
             "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"%s\"}", 
             protocol->session_id ? protocol->session_id : "", mode_str);
    
    protocol->vtable->send_text(protocol, message);
}

void linx_protocol_send_stop_listening(linx_protocol_t* protocol) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_text) {
        return;
    }
    
    char message[256];
    snprintf(message, sizeof(message), 
             "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}", 
             protocol->session_id ? protocol->session_id : "");
    
    protocol->vtable->send_text(protocol, message);
}

void linx_protocol_send_abort_speaking(linx_protocol_t* protocol, linx_abort_reason_t reason) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_text) {
        return;
    }
    
    char message[256];
    if (reason == LINX_ABORT_REASON_WAKE_WORD_DETECTED) {
        snprintf(message, sizeof(message), 
                 "{\"session_id\":\"%s\",\"type\":\"abort\",\"reason\":\"wake_word_detected\"}", 
                 protocol->session_id ? protocol->session_id : "");
    } else {
        snprintf(message, sizeof(message), 
                 "{\"session_id\":\"%s\",\"type\":\"abort\"}", 
                 protocol->session_id ? protocol->session_id : "");
    }
    
    protocol->vtable->send_text(protocol, message);
}

void linx_protocol_send_mcp_message(linx_protocol_t* protocol, const char* message) {
    if (!protocol || !protocol->vtable || !protocol->vtable->send_text || !message) {
        return;
    }
    
    char mcp_message[1024];
    snprintf(mcp_message, sizeof(mcp_message), 
             "{\"session_id\":\"%s\",\"type\":\"mcp\",\"payload\":\"%s\"}", 
             protocol->session_id ? protocol->session_id : "", message);
    
    protocol->vtable->send_text(protocol, mcp_message);
}

/* 工具函数 */
void linx_protocol_set_error(linx_protocol_t* protocol, const char* message) {
    if (!protocol) {
        LOG_ERROR("Cannot set error on NULL protocol");
        return;
    }
    
    LOG_ERROR("Protocol error occurred: %s", message ? message : "Unknown error");
    
    protocol->error_occurred = true;
    if (protocol->callbacks.on_network_error && message) {
        protocol->callbacks.on_network_error(message, protocol->callbacks.user_data);
    }
}

bool linx_protocol_is_timeout(const linx_protocol_t* protocol) {
    if (!protocol) {
        LOG_WARN("Checking timeout on NULL protocol");
        return false;
    }
    
    uint64_t current_time = get_current_time_ms();
    bool is_timeout = (current_time - protocol->last_incoming_time) > LINX_TIMEOUT_MS;
    
    if (is_timeout) {
        LOG_WARN("Protocol timeout detected - last_incoming: %llu, current: %llu, diff: %llu ms", 
                 protocol->last_incoming_time, current_time, 
                 current_time - protocol->last_incoming_time);
    }
    
    return is_timeout;
}

/* 音频数据包管理 */
linx_audio_stream_packet_t* linx_audio_stream_packet_create(size_t payload_size) {
    linx_audio_stream_packet_t* packet = malloc(sizeof(linx_audio_stream_packet_t));
    if (!packet) {
        return NULL;
    }
    
    memset(packet, 0, sizeof(linx_audio_stream_packet_t));
    
    // 如果需要载荷，分配内存
    if (payload_size > 0) {
        packet->payload = malloc(payload_size);
        if (!packet->payload) {
            free(packet);
            return NULL;
        }
        packet->payload_size = payload_size;
    }
    
    return packet;
}

void linx_audio_stream_packet_destroy(linx_audio_stream_packet_t* packet) {
    if (!packet) {
        return;
    }
    
    // 释放载荷内存
    if (packet->payload) {
        free(packet->payload);
    }
    free(packet);
}