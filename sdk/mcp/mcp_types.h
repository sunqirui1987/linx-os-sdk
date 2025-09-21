#ifndef MCP_TYPES_H
#define MCP_TYPES_H

/*
 * MCP类型定义头文件
 * 包含MCP协议中使用的所有数据类型、枚举和结构体定义
 */

#include <stdbool.h>    // 布尔类型支持
#include <stdint.h>     // 标准整数类型
#include <stddef.h>     // 标准定义
#include "../cjson/cJSON.h"      // JSON处理库

#ifdef __cplusplus
extern "C" {
#endif

/* 属性类型枚举 */
typedef enum {
    MCP_PROPERTY_TYPE_BOOLEAN,  // 布尔类型属性
    MCP_PROPERTY_TYPE_INTEGER,  // 整数类型属性
    MCP_PROPERTY_TYPE_STRING    // 字符串类型属性
} mcp_property_type_t;

/* 返回值类型枚举 */
typedef enum {
    MCP_RETURN_TYPE_BOOL,       // 布尔返回值
    MCP_RETURN_TYPE_INT,        // 整数返回值
    MCP_RETURN_TYPE_STRING,     // 字符串返回值
    MCP_RETURN_TYPE_JSON,       // JSON对象返回值
    MCP_RETURN_TYPE_IMAGE       // 图像内容返回值
} mcp_return_type_t;

/* 属性值联合体 */
typedef union {
    bool bool_val;      // 布尔值
    int int_val;        // 整数值
    char* string_val;   // 字符串值
} mcp_property_value_t;

/* 带类型信息的返回值结构体 */
typedef struct {
    mcp_return_type_t type;     // 返回值类型
    union {
        bool bool_val;                      // 布尔值
        int int_val;                        // 整数值
        char* string_val;                   // 字符串值
        cJSON* json_val;                    // JSON对象值
        struct mcp_image_content* image_val; // 图像内容值
    } value;
} mcp_return_value_t;

/* 前向声明 */
struct mcp_property;        // MCP属性结构体
struct mcp_property_list;   // MCP属性列表结构体
struct mcp_tool;            // MCP工具结构体
struct mcp_server;          // MCP服务器结构体
struct mcp_image_content;   // MCP图像内容结构体

/* 工具回调函数类型 */
typedef mcp_return_value_t (*mcp_tool_callback_t)(const struct mcp_property_list* properties);

/* 能力配置回调函数类型 */
/* 摄像头解释URL设置回调函数类型 */
typedef void (*mcp_camera_set_explain_url_callback_t)(const char* url, const char* token);

/* 能力配置结构体 */
typedef struct {
    mcp_camera_set_explain_url_callback_t camera_set_explain_url;  // 摄像头解释URL设置回调
    /* 可以在这里添加其他硬件能力的回调函数指针 */
} mcp_capability_callbacks_t;

/* 常量定义 */
#define MCP_MAX_NAME_LENGTH 256         // 最大名称长度
#define MCP_MAX_DESCRIPTION_LENGTH 1024 // 最大描述长度
#define MCP_MAX_TOOLS 64                // 最大工具数量
#define MCP_MAX_PROPERTIES 32           // 最大属性数量
#define MCP_MAX_URL_LENGTH 512          // 最大URL长度

#ifdef __cplusplus
}
#endif

#endif /* MCP_TYPES_H */