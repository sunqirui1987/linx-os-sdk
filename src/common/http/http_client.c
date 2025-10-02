#include "http_client.h"
#include "../third/mongoose/mongoose.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

// 最大头部数量
#define MAX_HEADERS 32
// 默认超时时间(毫秒)
#define DEFAULT_TIMEOUT_MS 30000

// HTTP头部结构
typedef struct {
    char name[256];
    char value[1024];
} http_header_t;

// HTTP客户端结构体
struct http_client {
    struct mg_mgr mgr;              // mongoose管理器
    struct mg_connection *conn;     // 连接指针
    int timeout_ms;                 // 超时时间
    
    // HTTP请求相关
    char *method;                   // HTTP方法
    char *url;                      // 请求URL
    http_header_t headers[MAX_HEADERS]; // 请求头数组
    int header_count;               // 请求头数量
    
    // 响应相关
    int status_code;                // 响应状态码
    char *response_body;            // 响应体
    size_t response_len;            // 响应长度
    size_t response_capacity;       // 响应缓冲区容量
    
    // 状态控制
    bool is_connected;              // 是否已连接
    bool request_sent;              // 是否已发送请求
    bool response_complete;         // 响应是否完成
    bool chunked_mode;              // 是否为分块传输模式
    
    // 错误处理
    int last_error;                 // 最后的错误码
};

// 前向声明
static void http_client_event_handler(struct mg_connection *c, int ev, void *ev_data);

// mongoose事件处理回调函数
static void http_client_event_handler(struct mg_connection *c, int ev, void *ev_data) {
    http_client_t *client = (http_client_t *)c->fn_data;
    
    if (!client) return;
    
    switch (ev) {
        case MG_EV_CONNECT: {
            // 连接建立
            client->is_connected = true;
            client->last_error = HTTP_CLIENT_OK;
            LOG_INFO("HTTP client connected");
            break;
        }
        
        case MG_EV_HTTP_MSG: {
            // 收到HTTP响应
            struct mg_http_message *hm = (struct mg_http_message *)ev_data;
            client->status_code = mg_http_status(hm);
            
            // 分配或扩展响应缓冲区
            size_t needed_size = client->response_len + hm->body.len + 1;
            if (needed_size > client->response_capacity) {
                size_t new_capacity = needed_size * 2; // 预留一些空间
                char *new_buffer = realloc(client->response_body, new_capacity);
                if (new_buffer) {
                    client->response_body = new_buffer;
                    client->response_capacity = new_capacity;
                } else {
                    client->last_error = HTTP_CLIENT_ERROR_MEMORY;
                    return;
                }
            }
            
            // 复制响应数据
            if (hm->body.len > 0) {
                memcpy(client->response_body + client->response_len, hm->body.buf, hm->body.len);
                client->response_len += hm->body.len;
                client->response_body[client->response_len] = '\0';
            }
            
            client->response_complete = true;
            LOG_INFO("HTTP response received, status: %d", client->status_code);
            break;
        }
        
        case MG_EV_ERROR: {
            // 连接错误
            client->last_error = HTTP_CLIENT_ERROR_NETWORK;
            client->is_connected = false;
            LOG_ERROR("HTTP client connection error: %s", (char *)ev_data);
            break;
        }
        
        case MG_EV_CLOSE: {
            // 连接关闭
            client->is_connected = false;
            client->conn = NULL;
            LOG_INFO("HTTP client connection closed");
            break;
        }
        
        default:
            break;
    }
}

// 创建HTTP客户端实例
http_client_t* http_client_create(void) {
    http_client_t *client = calloc(1, sizeof(http_client_t));
    if (!client) {
        LOG_ERROR("Failed to allocate memory for HTTP client");
        return NULL;
    }
    
    // 初始化mongoose管理器
    mg_mgr_init(&client->mgr);
    
    // 设置默认值
    client->timeout_ms = DEFAULT_TIMEOUT_MS;
    client->header_count = 0;
    client->status_code = 0;
    client->response_body = NULL;
    client->response_len = 0;
    client->response_capacity = 0;
    client->is_connected = false;
    client->request_sent = false;
    client->response_complete = false;
    client->chunked_mode = false;
    client->last_error = HTTP_CLIENT_OK;
    client->method = NULL;
    client->url = NULL;
    client->conn = NULL;
    
    LOG_INFO("HTTP client created successfully");
    return client;
}

// 销毁HTTP客户端实例
void http_client_destroy(http_client_t *client) {
    if (!client) return;
    
    // 关闭连接
    if (client->conn) {
        mg_close_conn(client->conn);
        client->conn = NULL;
    }
    
    // 清理mongoose管理器
    mg_mgr_free(&client->mgr);
    
    // 释放内存
    if (client->method) {
        free(client->method);
    }
    if (client->url) {
        free(client->url);
    }
    if (client->response_body) {
        free(client->response_body);
    }
    
    free(client);
    LOG_INFO("HTTP client destroyed");
}

// 设置HTTP请求头
int http_client_set_header(http_client_t *client, const char *name, const char *value) {
    if (!client || !name || !value) {
        LOG_ERROR("Invalid arguments for set header");
        return HTTP_CLIENT_ERROR_INVALID_ARG;
    }
    
    if (client->header_count >= MAX_HEADERS) {
        LOG_ERROR("Too many headers, max: %d", MAX_HEADERS);
        return HTTP_CLIENT_ERROR_MEMORY;
    }
    
    // 检查是否已存在相同名称的头部，如果存在则更新
    for (int i = 0; i < client->header_count; i++) {
        if (strcmp(client->headers[i].name, name) == 0) {
            strncpy(client->headers[i].value, value, sizeof(client->headers[i].value) - 1);
            client->headers[i].value[sizeof(client->headers[i].value) - 1] = '\0';
            LOG_DEBUG("Updated header: %s = %s", name, value);
            return HTTP_CLIENT_OK;
        }
    }
    
    // 添加新的头部
    strncpy(client->headers[client->header_count].name, name, sizeof(client->headers[client->header_count].name) - 1);
    client->headers[client->header_count].name[sizeof(client->headers[client->header_count].name) - 1] = '\0';
    strncpy(client->headers[client->header_count].value, value, sizeof(client->headers[client->header_count].value) - 1);
    client->headers[client->header_count].value[sizeof(client->headers[client->header_count].value) - 1] = '\0';
    client->header_count++;
    
    LOG_DEBUG("Added header: %s = %s", name, value);
    return HTTP_CLIENT_OK;
}

// 打开HTTP连接
int http_client_open(http_client_t *client, const char *method, const char *url) {
    if (!client || !method || !url) {
        LOG_ERROR("Invalid arguments for open connection");
        return HTTP_CLIENT_ERROR_INVALID_ARG;
    }
    
    // 如果已有连接，先关闭
    if (client->conn) {
        mg_close_conn(client->conn);
        client->conn = NULL;
    }
    
    // 保存方法和URL
    if (client->method) {
        free(client->method);
    }
    client->method = strdup(method);
    if (!client->method) {
        LOG_ERROR("Failed to allocate memory for method");
        return HTTP_CLIENT_ERROR_MEMORY;
    }
    
    if (client->url) {
        free(client->url);
    }
    client->url = strdup(url);
    if (!client->url) {
        LOG_ERROR("Failed to allocate memory for URL");
        free(client->method);
        client->method = NULL;
        return HTTP_CLIENT_ERROR_MEMORY;
    }
    
    // 重置状态
    client->is_connected = false;
    client->request_sent = false;
    client->response_complete = false;
    client->status_code = 0;
    client->response_len = 0;
    client->last_error = HTTP_CLIENT_OK;
    
    // 建立连接
    client->conn = mg_http_connect(&client->mgr, url, http_client_event_handler, client);
    if (!client->conn) {
        LOG_ERROR("Failed to create HTTP connection to %s", url);
        return HTTP_CLIENT_ERROR_NETWORK;
    }
    
    LOG_INFO("HTTP connection opened to %s", url);
    return HTTP_CLIENT_OK;
}

// 写入数据到HTTP请求体（流式）
int http_client_write(http_client_t *client, const char *data, size_t length) {
    if (!client) {
        LOG_ERROR("Invalid client for write");
        return HTTP_CLIENT_ERROR_INVALID_ARG;
    }
    
    if (!client->conn) {
        LOG_ERROR("No connection established");
        return HTTP_CLIENT_ERROR_NETWORK;
    }
    
    // 如果length为0，表示结束请求
    if (length == 0) {
        if (!client->request_sent) {
            // 构建HTTP请求头
            struct mg_str headers_str = {NULL, 0};
            char *headers_buf = NULL;
            size_t headers_len = 0;
            
            // 计算所需的头部缓冲区大小
            for (int i = 0; i < client->header_count; i++) {
                headers_len += strlen(client->headers[i].name) + strlen(client->headers[i].value) + 4; // ": " + "\r\n"
            }
            
            if (headers_len > 0) {
                headers_buf = malloc(headers_len + 1);
                if (headers_buf) {
                    headers_buf[0] = '\0';
                    for (int i = 0; i < client->header_count; i++) {
                        strcat(headers_buf, client->headers[i].name);
                        strcat(headers_buf, ": ");
                        strcat(headers_buf, client->headers[i].value);
                        strcat(headers_buf, "\r\n");
                    }
                    headers_str.buf = headers_buf;
                    headers_str.len = strlen(headers_buf);
                }
            }
            
            // 发送HTTP请求
            mg_printf(client->conn, "%s %s HTTP/1.1\r\n", client->method, client->url);
            if (headers_str.buf) {
                mg_send(client->conn, headers_str.buf, headers_str.len);
                free(headers_buf);
            }
            mg_send(client->conn, "\r\n", 2); // 结束头部
            
            client->request_sent = true;
            LOG_INFO("HTTP request sent");
        }
        return HTTP_CLIENT_OK;
    }
    
    if (!data) {
        LOG_ERROR("Invalid data for write");
        return HTTP_CLIENT_ERROR_INVALID_ARG;
    }
    
    // 如果还没发送请求头，先发送
    if (!client->request_sent) {
        // 添加Content-Length头部（如果没有设置）
        bool has_content_length = false;
        for (int i = 0; i < client->header_count; i++) {
            if (strcasecmp(client->headers[i].name, "Content-Length") == 0) {
                has_content_length = true;
                break;
            }
        }
        
        if (!has_content_length) {
            // 对于流式传输，使用chunked编码
            http_client_set_header(client, "Transfer-Encoding", "chunked");
            client->chunked_mode = true;
        }
        
        // 构建并发送HTTP请求头
        struct mg_str headers_str = {NULL, 0};
        char *headers_buf = NULL;
        size_t headers_len = 0;
        
        // 计算所需的头部缓冲区大小
        for (int i = 0; i < client->header_count; i++) {
            headers_len += strlen(client->headers[i].name) + strlen(client->headers[i].value) + 4; // ": " + "\r\n"
        }
        
        if (headers_len > 0) {
            headers_buf = malloc(headers_len + 1);
            if (headers_buf) {
                headers_buf[0] = '\0';
                for (int i = 0; i < client->header_count; i++) {
                    strcat(headers_buf, client->headers[i].name);
                    strcat(headers_buf, ": ");
                    strcat(headers_buf, client->headers[i].value);
                    strcat(headers_buf, "\r\n");
                }
                headers_str.buf = headers_buf;
                headers_str.len = strlen(headers_buf);
            }
        }
        
        // 发送HTTP请求行和头部
        mg_printf(client->conn, "%s %s HTTP/1.1\r\n", client->method, client->url);
        if (headers_str.buf) {
            mg_send(client->conn, headers_str.buf, headers_str.len);
            free(headers_buf);
        }
        mg_send(client->conn, "\r\n", 2); // 结束头部
        
        client->request_sent = true;
        LOG_DEBUG("HTTP request headers sent");
    }
    
    // 发送数据
    if (client->chunked_mode) {
        // 发送chunked数据
        mg_printf(client->conn, "%lx\r\n", (unsigned long)length);
        mg_send(client->conn, data, length);
        mg_send(client->conn, "\r\n", 2);
    } else {
        // 直接发送数据
        mg_send(client->conn, data, length);
    }
    
    LOG_DEBUG("Sent %zu bytes of data", length);
    return HTTP_CLIENT_OK;
}

// 获取HTTP响应状态码
int http_client_get_status_code(http_client_t *client) {
    if (!client) {
        LOG_ERROR("Invalid client for get status code");
        return -1;
    }
    
    if (!client->response_complete) {
        // 等待响应完成
        uint64_t start_time = mg_millis();
        while (!client->response_complete && (mg_millis() - start_time) < (uint64_t)client->timeout_ms) {
            mg_mgr_poll(&client->mgr, 10); // 10ms轮询间隔
            if (client->last_error != HTTP_CLIENT_OK) {
                break;
            }
        }
        
        if (!client->response_complete) {
            LOG_ERROR("Response timeout while getting status code");
            return -1;
        }
    }
    
    return client->status_code;
}

// 读取所有响应数据
char* http_client_read_all(http_client_t *client) {
    if (!client) {
        LOG_ERROR("Invalid client for read all");
        return NULL;
    }
    
    if (!client->response_complete) {
        // 等待响应完成
        uint64_t start_time = mg_millis();
        while (!client->response_complete && (mg_millis() - start_time) < (uint64_t)client->timeout_ms) {
            mg_mgr_poll(&client->mgr, 10); // 10ms轮询间隔
            if (client->last_error != HTTP_CLIENT_OK) {
                break;
            }
        }
        
        if (!client->response_complete) {
            LOG_ERROR("Response timeout or error");
            return NULL;
        }
    }
    
    if (!client->response_body) {
        return NULL;
    }
    
    // 返回响应数据的副本
    char *result = malloc(client->response_len + 1);
    if (result) {
        memcpy(result, client->response_body, client->response_len);
        result[client->response_len] = '\0';
    }
    
    return result;
}

// 关闭HTTP连接
void http_client_close(http_client_t *client) {
    if (!client) return;
    
    if (client->conn) {
        // 如果是chunked模式且还没结束，发送结束标记
        if (client->chunked_mode && client->request_sent) {
            mg_send(client->conn, "0\r\n\r\n", 5); // 结束chunked传输
        }
        
        mg_close_conn(client->conn);
        client->conn = NULL;
    }
    
    client->is_connected = false;
    client->request_sent = false;
    client->response_complete = false;
    client->chunked_mode = false;
    
    LOG_INFO("HTTP connection closed");
}

// 设置全局超时时间
void http_client_set_timeout(http_client_t *client, int timeout_ms) {
    if (!client) {
        LOG_ERROR("Invalid client for set timeout");
        return;
    }
    
    client->timeout_ms = timeout_ms;
    LOG_DEBUG("Timeout set to %d ms", timeout_ms);
}
