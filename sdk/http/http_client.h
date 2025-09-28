#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>
#include <stdbool.h>
#include "../log/linx_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// HTTP客户端结构体
typedef struct http_client http_client_t;

/**
 * @brief 创建HTTP客户端实例
 * @return 成功返回客户端实例指针，失败返回NULL
 */
http_client_t* http_client_create(void);

/**
 * @brief 销毁HTTP客户端实例
 * @param client 客户端实例指针
 */
void http_client_destroy(http_client_t *client);

/**
 * @brief 设置HTTP请求头
 * @param client 客户端实例
 * @param name 头部名称
 * @param value 头部值
 * @return 成功返回0，失败返回负数错误码
 */
int http_client_set_header(http_client_t *client, const char *name, const char *value);

/**
 * @brief 打开HTTP连接
 * @param client 客户端实例
 * @param method HTTP方法字符串
 * @param url 请求URL
 * @return 成功返回0，失败返回负数错误码
 */
int http_client_open(http_client_t *client, const char *method, const char *url);

/**
 * @brief 写入数据到HTTP请求体
 * @param client 客户端实例
 * @param data 要写入的数据
 * @param length 数据长度，传0表示结束请求
 * @return 成功返回0，失败返回负数错误码
 */
int http_client_write(http_client_t *client, const char *data, size_t length);

/**
 * @brief 获取HTTP响应状态码
 * @param client 客户端实例
 * @return 返回HTTP状态码
 */
int http_client_get_status_code(http_client_t *client);

/**
 * @brief 读取所有响应数据
 * @param client 客户端实例
 * @return 返回响应数据字符串，需要调用者释放内存
 */
char* http_client_read_all(http_client_t *client);

/**
 * @brief 关闭HTTP连接
 * @param client 客户端实例
 */
void http_client_close(http_client_t *client);

/**
 * @brief 设置全局超时时间
 * @param client 客户端实例
 * @param timeout_ms 超时时间(毫秒)
 */
void http_client_set_timeout(http_client_t *client, int timeout_ms);



// HTTP客户端错误码
typedef enum {
    HTTP_CLIENT_OK = 0,
    HTTP_CLIENT_ERROR_INVALID_ARG = -1,
    HTTP_CLIENT_ERROR_MEMORY = -2,
    HTTP_CLIENT_ERROR_NETWORK = -3,
    HTTP_CLIENT_ERROR_TIMEOUT = -4
} http_client_error_t;

// 为了兼容性，定义简化的错误常量
#define HTTP_OK                 HTTP_CLIENT_OK
#define HTTP_ERROR_INVALID_ARG  HTTP_CLIENT_ERROR_INVALID_ARG
#define HTTP_ERROR_MEMORY       HTTP_CLIENT_ERROR_MEMORY
#define HTTP_ERROR_NETWORK      HTTP_CLIENT_ERROR_NETWORK
#define HTTP_ERROR_TIMEOUT      HTTP_CLIENT_ERROR_TIMEOUT

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H