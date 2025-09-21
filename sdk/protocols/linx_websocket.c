#include "linx_websocket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <mongoose.h>
#include "../cjson/cJSON.h"
#include "../log/linx_log.h"

/* WebSocket 协议实现结构体 - 隐藏实现细节 */
struct linx_websocket_protocol {
    linx_protocol_t base;           // 基础协议结构体
    struct mg_mgr mgr;              // Mongoose 管理器
    struct mg_connection* conn;     // WebSocket 连接句柄
    bool connected;                 // 连接状态标志
    bool audio_channel_opened;      // 音频通道开启状态
    int version;                    // 协议版本
    bool server_hello_received;     // 服务器hello消息接收状态
    bool running;                   // 运行状态
    bool should_stop;               // 停止标志
    char* server_url;               // 服务器URL
    char* server_host;              // 服务器主机
    char* server_path;              // 服务器路径
    int server_port;                // 服务器端口
    char* session_id;               // 会话ID
    char* auth_token;               // 认证令牌
    char* device_id;                // 设备ID
    char* client_id;                // 客户端ID
    int server_sample_rate;         // 服务器采样率
    int server_frame_duration;      // 服务器帧持续时间
};

/* Internal helper function declarations */
static void linx_websocket_protocol_destroy(linx_websocket_protocol_t* ws_protocol);
static bool linx_websocket_parse_server_hello(linx_websocket_protocol_t* ws_protocol, const char* json_str);
static char* linx_websocket_get_hello_message(linx_websocket_protocol_t* ws_protocol);
static void linx_websocket_event_handler(struct mg_connection* conn, int ev, void* ev_data);
static char* extract_json_string_value(const cJSON* json, const char* key);
static int extract_json_int_value(const cJSON* json, const char* key);
static bool linx_websocket_protocol_set_server_url(linx_websocket_protocol_t* ws_protocol, const char* url);
static bool linx_websocket_protocol_set_server(linx_websocket_protocol_t* ws_protocol, const char* host, int port, const char* path);
static bool linx_websocket_protocol_set_auth_token(linx_websocket_protocol_t* ws_protocol, const char* token);
static bool linx_websocket_protocol_set_device_id(linx_websocket_protocol_t* ws_protocol, const char* device_id);
static bool linx_websocket_protocol_set_client_id(linx_websocket_protocol_t* ws_protocol, const char* client_id);

/* Protocol vtable for WebSocket implementation */
static const linx_protocol_vtable_t linx_websocket_vtable = {
    .start = linx_websocket_start,
    .send_audio = linx_websocket_send_audio,
    .send_text = linx_websocket_send_text,
    .destroy = linx_websocket_destroy
};

/* WebSocket protocol creation and destruction */
linx_websocket_protocol_t* linx_websocket_protocol_create(const linx_websocket_config_t* config) {
    LOG_DEBUG("Creating WebSocket protocol with config: %p", config);
    
    if (!config) {
        LOG_ERROR("WebSocket protocol creation failed: config is NULL");
        return NULL;
    }
    
    linx_websocket_protocol_t* ws_protocol = malloc(sizeof(linx_websocket_protocol_t));
    if (!ws_protocol) {
        LOG_ERROR("WebSocket protocol creation failed: memory allocation failed");
        return NULL;
    }
    
    memset(ws_protocol, 0, sizeof(linx_websocket_protocol_t));
    
    /* Initialize base protocol */
    linx_protocol_init(&ws_protocol->base, &linx_websocket_vtable);
    
    /* Initialize mongoose manager */
    mg_mgr_init(&ws_protocol->mgr);
    
    /* Set default values */
    ws_protocol->connected = false;
    ws_protocol->audio_channel_opened = false;
    ws_protocol->version = 1;
    ws_protocol->server_hello_received = false;
    ws_protocol->running = false;
    ws_protocol->should_stop = false;
    ws_protocol->conn = NULL;
    ws_protocol->auth_token = NULL;
    ws_protocol->device_id = NULL;
    ws_protocol->client_id = NULL;
    
    LOG_DEBUG("WebSocket protocol basic initialization completed");
    
    /* Configure server connection */
    if (config->url) {
        LOG_DEBUG("Configuring WebSocket with URL: %s", config->url);
        if (!linx_websocket_protocol_set_server_url(ws_protocol, config->url)) {
            LOG_ERROR("Failed to set WebSocket server URL");
            linx_websocket_protocol_destroy(ws_protocol);
            free(ws_protocol);
            return NULL;
        }
    } else if (config->host && config->path) {
        LOG_DEBUG("Configuring WebSocket with host: %s, port: %d, path: %s", 
                  config->host, config->port, config->path);
        if (!linx_websocket_protocol_set_server(ws_protocol, config->host, config->port, config->path)) {
            LOG_ERROR("Failed to set WebSocket server configuration");
            linx_websocket_protocol_destroy(ws_protocol);
            free(ws_protocol);
            return NULL;
        }
    } else {
        LOG_ERROR("WebSocket protocol creation failed: neither URL nor host+path provided");
        linx_websocket_protocol_destroy(ws_protocol);
        free(ws_protocol);
        return NULL; /* Either url or host+path must be provided */
    }
    
    /* Configure authentication and identification */
    if (config->auth_token) {
        LOG_DEBUG("Setting WebSocket auth token");
        if (!linx_websocket_protocol_set_auth_token(ws_protocol, config->auth_token)) {
            LOG_ERROR("Failed to set WebSocket auth token");
            linx_websocket_protocol_destroy(ws_protocol);
            free(ws_protocol);
            return NULL;
        }
    }
    
    if (config->device_id) {
        LOG_DEBUG("Setting WebSocket device ID: %s", config->device_id);
        if (!linx_websocket_protocol_set_device_id(ws_protocol, config->device_id)) {
            LOG_ERROR("Failed to set WebSocket device ID");
            linx_websocket_protocol_destroy(ws_protocol);
            free(ws_protocol);
            return NULL;
        }
    }
    
    if (config->client_id) {
        LOG_DEBUG("Setting WebSocket client ID: %s", config->client_id);
        if (!linx_websocket_protocol_set_client_id(ws_protocol, config->client_id)) {
            LOG_ERROR("Failed to set WebSocket client ID");
            linx_websocket_protocol_destroy(ws_protocol);
            free(ws_protocol);
            return NULL;
        }
    }
    
    /* Set protocol version */
    if (config->protocol_version > 0) {
        LOG_DEBUG("Setting WebSocket protocol version: %d", config->protocol_version);
        ws_protocol->version = config->protocol_version;
    }
    
    LOG_INFO("WebSocket protocol created successfully - version: %d, URL: %s", 
             ws_protocol->version, ws_protocol->server_url ? ws_protocol->server_url : "N/A");
    
    return ws_protocol;
}

static void linx_websocket_protocol_destroy(linx_websocket_protocol_t* ws_protocol) {
    if (!ws_protocol) {
        LOG_WARN("Attempting to destroy NULL WebSocket protocol");
        return;
    }
    
    LOG_DEBUG("Destroying WebSocket protocol");
    
    /* Stop the protocol if running */
    linx_websocket_stop(ws_protocol);
    
    /* Clean up connection */
    if (ws_protocol->conn) {
        LOG_DEBUG("Closing WebSocket connection");
        ws_protocol->conn->is_closing = 1;
        ws_protocol->conn = NULL;
    }
    
    /* Clean up mongoose manager */
    mg_mgr_free(&ws_protocol->mgr);
    
    /* Free allocated strings */
    if (ws_protocol->server_url) {
        free(ws_protocol->server_url);
    }
    if (ws_protocol->server_host) {
        free(ws_protocol->server_host);
    }
    if (ws_protocol->server_path) {
        free(ws_protocol->server_path);
    }
    if (ws_protocol->auth_token) {
        free(ws_protocol->auth_token);
    }
    if (ws_protocol->device_id) {
        free(ws_protocol->device_id);
    }
    if (ws_protocol->client_id) {
        free(ws_protocol->client_id);
    }
    
    /* Clean up base protocol resources directly (avoid recursive call) */
    if (ws_protocol->base.session_id) {
        free(ws_protocol->base.session_id);
        ws_protocol->base.session_id = NULL;
    }
    
    LOG_INFO("WebSocket protocol destroyed successfully");
    
    free(ws_protocol);
}

/* Internal configuration functions */
static bool linx_websocket_protocol_set_server_url(linx_websocket_protocol_t* ws_protocol, const char* url) {
    if (!ws_protocol || !url) {
        return false;
    }
    
    if (ws_protocol->server_url) {
        free(ws_protocol->server_url);
    }
    
    ws_protocol->server_url = strdup(url);
    return ws_protocol->server_url != NULL;
}

static bool linx_websocket_protocol_set_server(linx_websocket_protocol_t* ws_protocol, 
                                        const char* host, 
                                        int port, 
                                        const char* path) {
    if (!ws_protocol || !host || !path) {
        return false;
    }
    
    /* Free existing values */
    if (ws_protocol->server_host) {
        free(ws_protocol->server_host);
    }
    if (ws_protocol->server_path) {
        free(ws_protocol->server_path);
    }
    if (ws_protocol->server_url) {
        free(ws_protocol->server_url);
    }
    
    /* Set new values */
    ws_protocol->server_host = strdup(host);
    ws_protocol->server_port = port;
    ws_protocol->server_path = strdup(path);
    
    /* Construct URL */
    char url[512];
    snprintf(url, sizeof(url), "ws://%s:%d%s", host, port, path);
    ws_protocol->server_url = strdup(url);
    
    return ws_protocol->server_host && ws_protocol->server_path && ws_protocol->server_url;
}

static bool linx_websocket_protocol_set_auth_token(linx_websocket_protocol_t* ws_protocol, const char* token) {
    if (!ws_protocol || !token) {
        return false;
    }
    
    if (ws_protocol->auth_token) {
        free(ws_protocol->auth_token);
    }
    
    ws_protocol->auth_token = strdup(token);
    return ws_protocol->auth_token != NULL;
}

static bool linx_websocket_protocol_set_device_id(linx_websocket_protocol_t* ws_protocol, const char* device_id) {
    if (!ws_protocol || !device_id) {
        return false;
    }
    
    if (ws_protocol->device_id) {
        free(ws_protocol->device_id);
    }
    
    ws_protocol->device_id = strdup(device_id);
    return ws_protocol->device_id != NULL;
}

static bool linx_websocket_protocol_set_client_id(linx_websocket_protocol_t* ws_protocol, const char* client_id) {
    if (!ws_protocol || !client_id) {
        return false;
    }
    
    if (ws_protocol->client_id) {
        free(ws_protocol->client_id);
    }
    
    ws_protocol->client_id = strdup(client_id);
    return ws_protocol->client_id != NULL;
}

/* Public configuration function */


/* Event handler for mongoose WebSocket events */
static void linx_websocket_event_handler(struct mg_connection* conn, int ev, void* ev_data) {
    linx_websocket_protocol_t* ws_protocol = (linx_websocket_protocol_t*)conn->fn_data;
    
    if (!ws_protocol) {
        LOG_ERROR("WebSocket event handler called with NULL protocol");
        return;
    }
    
    switch (ev) {
        case MG_EV_CONNECT: {
            /* Connection established, WebSocket upgrade will happen automatically */
            LOG_DEBUG("WebSocket TCP connection established");
            break;
        }
        
        case MG_EV_WS_OPEN: {
            /* WebSocket connection opened */
            LOG_INFO("WebSocket connection opened successfully");
            ws_protocol->connected = true;
            if (ws_protocol->base.callbacks.on_connected) {
                ws_protocol->base.callbacks.on_connected(ws_protocol->base.callbacks.user_data);
            }
            
            /* Send hello message */
            char* hello_msg = linx_websocket_get_hello_message(ws_protocol);
            if (hello_msg) {
                LOG_DEBUG("Sending WebSocket hello message");
                mg_ws_send(conn, hello_msg, strlen(hello_msg), WEBSOCKET_OP_TEXT);
                free(hello_msg);
            } else {
                LOG_ERROR("Failed to generate WebSocket hello message");
            }
            break;
        }
        
        case MG_EV_WS_MSG: {
            /* WebSocket message received */
            struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
            
            if (wm->flags & WEBSOCKET_OP_TEXT) {
                /* Text message - parse as JSON */
                LOG_DEBUG("WebSocket received text message (length: %zu)", wm->data.len);
                
                cJSON* json = cJSON_ParseWithLength((const char*)wm->data.buf, wm->data.len);
                if (!json) {
                    LOG_ERROR("WebSocket failed to parse JSON message");
                    return;
                }

               

                
                cJSON* type = cJSON_GetObjectItem(json, "type");
                if (!cJSON_IsString(type) || !type->valuestring) {
                    LOG_ERROR("WebSocket invalid or missing message type");
                    cJSON_Delete(json);
                    return;
                }
                
                LOG_DEBUG("WebSocket message type: %s", type->valuestring);
                
                /* Handle different message types */
                if (strcmp(type->valuestring, "hello") == 0) {
                    /* Server hello message - handle internally */
                    LOG_INFO("WebSocket processing server hello message");
                    char* json_string = cJSON_Print(json);
                    if (json_string) {
                        linx_websocket_parse_server_hello(ws_protocol, json_string);
                        LOG_INFO("WebSocket server hello processed successfully");
                        free(json_string);
                    } else {
                        LOG_ERROR("WebSocket failed to serialize hello message");
                    }
                } 

                /* Other message types - call user callback */
                if (ws_protocol->base.callbacks.on_incoming_json) {
                    ws_protocol->base.callbacks.on_incoming_json(json, ws_protocol->base.callbacks.user_data);
                    LOG_DEBUG("WebSocket user callback executed for type: %s", type->valuestring);
                } else {
                    LOG_DEBUG("WebSocket no user callback registered");
                }
              

                cJSON_Delete(json);
            } else if (wm->flags & WEBSOCKET_OP_BINARY) {
               
                /* Binary message - parse as audio data based on protocol version */
                if (ws_protocol->base.callbacks.on_incoming_audio) {
                    if (ws_protocol->version == 2) {
                        /* Use binary protocol v2 */
                        if (wm->data.len >= sizeof(linx_binary_protocol2_t)) {
                            linx_binary_protocol2_t* bp2 = (linx_binary_protocol2_t*)wm->data.buf;
                            uint16_t version = ntohs(bp2->version);
                            uint16_t type = ntohs(bp2->type);
                            uint32_t timestamp = ntohl(bp2->timestamp);
                            uint32_t payload_size = ntohl(bp2->payload_size);
                            
                            if (type == 0 && payload_size > 0) { /* Audio data */
                                linx_audio_stream_packet_t* packet = linx_audio_stream_packet_create(payload_size);
                                if (packet) {
                                    packet->sample_rate = ws_protocol->base.server_sample_rate;
                                    packet->frame_duration = ws_protocol->base.server_frame_duration;
                                    packet->timestamp = timestamp;
                                    memcpy(packet->payload, bp2->payload, payload_size);
                                    
                                    ws_protocol->base.callbacks.on_incoming_audio(packet, ws_protocol->base.callbacks.user_data);
                                    linx_audio_stream_packet_destroy(packet);
                                }
                            }
                        }
                    } else if (ws_protocol->version == 3) {
                        /* Use binary protocol v3 */
                        if (wm->data.len >= sizeof(linx_binary_protocol3_t)) {
                            linx_binary_protocol3_t* bp3 = (linx_binary_protocol3_t*)wm->data.buf;
                            uint8_t type = bp3->type;
                            uint16_t payload_size = ntohs(bp3->payload_size);
                            
                            if (type == 0 && payload_size > 0) { /* Audio data */
                                linx_audio_stream_packet_t* packet = linx_audio_stream_packet_create(payload_size);
                                if (packet) {
                                    packet->sample_rate = ws_protocol->base.server_sample_rate;
                                    packet->frame_duration = ws_protocol->base.server_frame_duration;
                                    packet->timestamp = 0; /* v3 protocol doesn't include timestamp */
                                    memcpy(packet->payload, bp3->payload, payload_size);
                                    
                                    ws_protocol->base.callbacks.on_incoming_audio(packet, ws_protocol->base.callbacks.user_data);
                                    linx_audio_stream_packet_destroy(packet);
                                }
                            }
                        }
                    } else {
                        /* Fallback for unsupported protocol versions - treat as raw audio data */
                        linx_audio_stream_packet_t* packet = linx_audio_stream_packet_create(wm->data.len);
                        if (packet) {
                            packet->sample_rate = ws_protocol->base.server_sample_rate;
                            packet->frame_duration = ws_protocol->base.server_frame_duration;
                            packet->timestamp = 0;
                            memcpy(packet->payload, wm->data.buf, wm->data.len);
                            
                            ws_protocol->base.callbacks.on_incoming_audio(packet, ws_protocol->base.callbacks.user_data);
                            linx_audio_stream_packet_destroy(packet);
                        }
                    }
                }
            }
            break;
        }
        
        case MG_EV_CLOSE: {
            /* Connection closed */
            LOG_INFO("WebSocket connection closed");
            ws_protocol->connected = false;
            ws_protocol->audio_channel_opened = false;
            ws_protocol->conn = NULL;
            
            if (ws_protocol->base.callbacks.on_disconnected) {
                ws_protocol->base.callbacks.on_disconnected(ws_protocol->base.callbacks.user_data);
            }
            break;
        }
        
        case MG_EV_ERROR: {
            /* Connection error */
            char* error_msg = (char*)ev_data;
            LOG_ERROR("WebSocket connection error: %s", error_msg ? error_msg : "Unknown error");
            linx_protocol_set_error(&ws_protocol->base, error_msg ? error_msg : "WebSocket connection error");
            break;
        }
        
        default:
            break;
    }
}

/* Protocol implementation functions */
bool linx_websocket_start(linx_protocol_t* protocol) {
    linx_websocket_protocol_t* ws_protocol = (linx_websocket_protocol_t*)protocol;
    
    LOG_DEBUG("Starting WebSocket protocol");
    
    if (!ws_protocol || !ws_protocol->server_url) {
        LOG_ERROR("Cannot start WebSocket: invalid protocol or missing server URL");
        return false;
    }
    
    LOG_INFO("Starting WebSocket connection to: %s", ws_protocol->server_url);

    /* Create WebSocket connection with headers */
    char headers[1024] = "";
    
    /* Build headers string */
    if (ws_protocol->auth_token) {
        char auth_header[512];
        /* Add "Bearer " prefix if token doesn't have a space */
        if (strchr(ws_protocol->auth_token, ' ') == NULL) {
            snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s\r\n", ws_protocol->auth_token);
        } else {
            snprintf(auth_header, sizeof(auth_header), "Authorization: %s\r\n", ws_protocol->auth_token);
        }
        strncat(headers, auth_header, sizeof(headers) - strlen(headers) - 1);
    }
    
    if (ws_protocol->version > 0) {
        char version_header[64];
        snprintf(version_header, sizeof(version_header), "Protocol-Version: %d\r\n", ws_protocol->version);
        strncat(headers, version_header, sizeof(headers) - strlen(headers) - 1);
    }
    
    if (ws_protocol->device_id) {
        char device_header[256];
        snprintf(device_header, sizeof(device_header), "Device-Id: %s\r\n", ws_protocol->device_id);
        strncat(headers, device_header, sizeof(headers) - strlen(headers) - 1);
    }
    
    if (ws_protocol->client_id) {
        char client_header[256];
        snprintf(client_header, sizeof(client_header), "Client-Id: %s\r\n", ws_protocol->client_id);
        strncat(headers, client_header, sizeof(headers) - strlen(headers) - 1);
    }
    
    ws_protocol->conn = mg_ws_connect(&ws_protocol->mgr, ws_protocol->server_url, 
                                     linx_websocket_event_handler, ws_protocol, 
                                     strlen(headers) > 0 ? "%s" : NULL, headers);
    
    if (!ws_protocol->conn) {
        return false;
    }
    
    ws_protocol->running = true;
    ws_protocol->should_stop = false;
    
    return true;
}





bool linx_websocket_send_audio(linx_protocol_t* protocol, linx_audio_stream_packet_t* packet) {
    linx_websocket_protocol_t* ws_protocol = (linx_websocket_protocol_t*)protocol;
    
    if (!ws_protocol || !ws_protocol->conn || !ws_protocol->connected || !packet) {
        LOG_ERROR("Invalid websocket protocol or connection state");
        return false;
    }
    LOG_DEBUG("Sending audio packet - Sample Rate: %d, Frame Duration: %d, Timestamp: %u, Payload Size: %zu, Version: %d", 
              packet->sample_rate, packet->frame_duration, packet->timestamp, packet->payload_size, ws_protocol->version);
    
    if (ws_protocol->version == 2) {

        /* Use binary protocol v2 */
        size_t total_size = sizeof(linx_binary_protocol2_t) + packet->payload_size;
        uint8_t* buffer = malloc(total_size);
        if (!buffer) {
            LOG_ERROR("WebSocket send failed: memory allocation failed (protocol v2)");
            return false;
        }
        
        linx_binary_protocol2_t* bp2 = (linx_binary_protocol2_t*)buffer;
        bp2->version = htons(ws_protocol->version);
        bp2->type = htons(0); /* Audio type */
        bp2->reserved = 0;
        bp2->timestamp = htonl(packet->timestamp);
        bp2->payload_size = htonl(packet->payload_size);
        memcpy(bp2->payload, packet->payload, packet->payload_size);
        
        int send_result = mg_ws_send(ws_protocol->conn, buffer, total_size, WEBSOCKET_OP_BINARY);
        free(buffer);
        
        if (send_result > 0) {
            LOG_DEBUG("WebSocket send successful: %zu bytes (protocol v2, total size: %zu)", packet->payload_size, total_size);
        } else {
            LOG_ERROR("WebSocket send failed: mg_ws_send returned %d (protocol v2)", send_result);
        }
        
        return send_result > 0;
    } else if (ws_protocol->version == 3) {
        /* Use binary protocol v3 */
        size_t total_size = sizeof(linx_binary_protocol3_t) + packet->payload_size;
        uint8_t* buffer = malloc(total_size);
        if (!buffer) {
            LOG_ERROR("WebSocket send failed: memory allocation failed (protocol v3)");
            return false;
        }
        
        linx_binary_protocol3_t* bp3 = (linx_binary_protocol3_t*)buffer;
        bp3->type = 0; /* Audio type */
        bp3->reserved = 0;
        bp3->payload_size = htons(packet->payload_size);
        memcpy(bp3->payload, packet->payload, packet->payload_size);
        
        int send_result = mg_ws_send(ws_protocol->conn, buffer, total_size, WEBSOCKET_OP_BINARY);
        free(buffer);
        
        if (send_result > 0) {
            LOG_DEBUG("WebSocket send successful: %zu bytes (protocol v3, total size: %zu)", packet->payload_size, total_size);
        } else {
            LOG_ERROR("WebSocket send failed: mg_ws_send returned %d (protocol v3)", send_result);
        }
        
        return send_result > 0;
    } else {
        /* Fallback for unsupported protocol versions - send raw payload */
        int send_result = mg_ws_send(ws_protocol->conn, packet->payload, packet->payload_size, WEBSOCKET_OP_BINARY);
        
        if (send_result > 0) {
            LOG_DEBUG("WebSocket send successful: %zu bytes (raw data, protocol v%d)", packet->payload_size, ws_protocol->version);
        } else {
            LOG_ERROR("WebSocket send failed: mg_ws_send returned %d (raw data, protocol v%d)", send_result, ws_protocol->version);
        }
        
        return send_result > 0;
    }
}

bool linx_websocket_send_text(linx_protocol_t* protocol, const char* text) {
    linx_websocket_protocol_t* ws_protocol = (linx_websocket_protocol_t*)protocol;
    
    if (!ws_protocol || !ws_protocol->conn || !ws_protocol->connected || !text) {
        LOG_ERROR("WebSocket send text failed: invalid protocol or connection or not connected or text is empty");
        return false;
    }
    LOG_DEBUG("WebSocket sending text: %s", text);
    mg_ws_send(ws_protocol->conn, text, strlen(text), WEBSOCKET_OP_TEXT);
    return true;
}

void linx_websocket_destroy(linx_protocol_t* protocol) {
    linx_websocket_protocol_t* ws_protocol = (linx_websocket_protocol_t*)protocol;
    linx_websocket_protocol_destroy(ws_protocol);
}

/* Event loop functions */
void linx_websocket_poll(linx_websocket_protocol_t* ws_protocol, int timeout_ms) {
    if (!ws_protocol) {
        return;
    }
    
    mg_mgr_poll(&ws_protocol->mgr, timeout_ms);
}

void linx_websocket_stop(linx_websocket_protocol_t* ws_protocol) {
    if (!ws_protocol) {
        return;
    }
    
    ws_protocol->should_stop = true;
    ws_protocol->running = false;
    
    if (ws_protocol->conn) {
        ws_protocol->conn->is_closing = 1;
        ws_protocol->conn = NULL;
    }
}

/* cJSON-based JSON value extraction helpers */
static char* extract_json_string_value(const cJSON* json, const char* key) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsString(item) || (item->valuestring == NULL)) {
        return NULL;
    }
    
    size_t len = strlen(item->valuestring);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strcpy(result, item->valuestring);
    return result;
}

static int extract_json_int_value(const cJSON* json, const char* key) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(item)) {
        return -1;
    }
    
    return item->valueint;
}

/* Helper functions */
static bool linx_websocket_parse_server_hello(linx_websocket_protocol_t* ws_protocol, const char* json_str) {
    if (!ws_protocol || !json_str) {
        return false;
    }
    
    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        return false;
    }
    
    /* Check transport type */
    char* transport = extract_json_string_value(root, "transport");
    if (transport) {
        if (strcmp(transport, "websocket") != 0) {
            free(transport);
            cJSON_Delete(root);
            return false;
        }
        free(transport);
    }
    
    /* Parse session ID */
    char* session_id = extract_json_string_value(root, "session_id");
    if (session_id) {
        if (ws_protocol->session_id) {
            free(ws_protocol->session_id);
        }
        ws_protocol->session_id = session_id;
    }
    
    /* Parse audio_params section */
    const cJSON* audio_params = cJSON_GetObjectItemCaseSensitive(root, "audio_params");
    if (cJSON_IsObject(audio_params)) {
        int sample_rate = extract_json_int_value(audio_params, "sample_rate");
        if (sample_rate > 0) {
            ws_protocol->server_sample_rate = sample_rate;
        }
        
        int frame_duration = extract_json_int_value(audio_params, "frame_duration");
        if (frame_duration > 0) {
            ws_protocol->server_frame_duration = frame_duration;
        }
    }
    
    ws_protocol->server_hello_received = true;
    cJSON_Delete(root);
    return true;
}

static char* linx_websocket_get_hello_message(linx_websocket_protocol_t* ws_protocol) {
    if (!ws_protocol) {
        return NULL;
    }
    
    /* Build JSON using cJSON for better structure */
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", ws_protocol->version);
    
    /* Add features object */
    cJSON* features = cJSON_CreateObject();
    cJSON_AddBoolToObject(features, "mcp", true);
    /* Note: AEC feature would be added here if supported */
    cJSON_AddItemToObject(root, "features", features);
    
    cJSON_AddStringToObject(root, "transport", "websocket");
    
    /* Add audio_params object */
    cJSON* audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", LINX_WEBSOCKET_AUDIO_FORMAT);
    cJSON_AddNumberToObject(audio_params, "sample_rate", LINX_WEBSOCKET_AUDIO_SAMPLE_RATE);
    cJSON_AddNumberToObject(audio_params, "channels", LINX_WEBSOCKET_AUDIO_CHANNELS);
    cJSON_AddNumberToObject(audio_params, "frame_duration", LINX_WEBSOCKET_AUDIO_FRAME_DURATION);
    cJSON_AddItemToObject(root, "audio_params", audio_params);
    
    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return json_string;
}

/* Utility functions */
bool linx_websocket_is_connected(const linx_websocket_protocol_t* ws_protocol) {
    return ws_protocol ? ws_protocol->connected : false;
}

/* Additional WebSocket functions */
int linx_websocket_get_reconnect_attempts(const linx_websocket_protocol_t* protocol) {
    /* TODO: Add reconnect attempts tracking */
    return 0;
}

void linx_websocket_reset_reconnect_attempts(linx_websocket_protocol_t* protocol) {
    /* TODO: Reset reconnect attempts counter */
}

void linx_websocket_process_events(linx_websocket_protocol_t* protocol) {
    if (!protocol) return;
    linx_websocket_poll(protocol, 10);
}

bool linx_websocket_send_ping(linx_websocket_protocol_t* protocol) {
    if (!protocol || !protocol->conn) {
        return false;
    }
    
    mg_ws_send(protocol->conn, "", 0, WEBSOCKET_OP_PING);
    return true;
}

bool linx_websocket_is_connection_timeout(const linx_websocket_protocol_t* protocol) {
    /* TODO: Implement connection timeout check */
    return false;
}

/* WebSocket create function with config */
linx_websocket_protocol_t* linx_websocket_create(const linx_websocket_config_t* config) {
    return linx_websocket_protocol_create(config);
}