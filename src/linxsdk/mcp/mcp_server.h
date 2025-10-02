#ifndef MCP_SERVER_H
#define MCP_SERVER_H

/*
 * MCP服务器头文件
 * 定义MCP服务器的核心功能，包括工具管理、消息处理和能力配置
 */

#include "mcp_types.h"  // MCP类型定义
#include "mcp_tool.h"   // MCP工具定义

#ifdef __cplusplus
extern "C" {
#endif

/* MCP服务器结构体 */
typedef struct mcp_server {
    mcp_tool_t* tools[MCP_MAX_TOOLS];           // 工具数组
    size_t tool_count;                          // 工具数量
    char server_name[MCP_MAX_NAME_LENGTH];      // 服务器名称
    char server_version[64];                    // 服务器版本
    mcp_capability_callbacks_t capability_callbacks; // 能力回调函数集合
} mcp_server_t;

/* 消息发送回调函数类型 */
typedef void (*mcp_send_message_callback_t)(const char* message);

/* 服务器基础函数 */
/**
 * 创建MCP服务器实例
 * @param server_name 服务器名称
 * @param server_version 服务器版本
 * @return 服务器实例指针，失败返回NULL
 */
mcp_server_t* mcp_server_create(const char* server_name, const char* server_version);

/**
 * 销毁MCP服务器实例
 * @param server 服务器实例指针
 */
void mcp_server_destroy(mcp_server_t* server);

/* 工具管理函数 */
/**
 * 向服务器添加工具
 * @param server 服务器实例
 * @param tool 工具实例
 * @return 成功返回true，失败返回false
 */
bool mcp_server_add_tool(mcp_server_t* server, mcp_tool_t* tool);

/**
 * 向服务器添加简单工具
 * @param server 服务器实例
 * @param name 工具名称
 * @param description 工具描述
 * @param properties 工具属性列表
 * @param callback 工具回调函数
 * @return 成功返回true，失败返回false
 */
bool mcp_server_add_simple_tool(mcp_server_t* server, const char* name, const char* description,
                                mcp_property_list_t* properties, mcp_tool_callback_t callback);

/**
 * 向服务器添加仅用户可见的工具
 * @param server 服务器实例
 * @param name 工具名称
 * @param description 工具描述
 * @param properties 工具属性列表
 * @param callback 工具回调函数
 * @return 成功返回true，失败返回false
 */
bool mcp_server_add_user_only_tool(mcp_server_t* server, const char* name, const char* description,
                                   mcp_property_list_t* properties, mcp_tool_callback_t callback);

/**
 * 根据名称查找工具
 * @param server 服务器实例
 * @param name 工具名称
 * @return 工具实例指针，未找到返回NULL
 */
const mcp_tool_t* mcp_server_find_tool(const mcp_server_t* server, const char* name);

/* 消息处理函数 */
/**
 * 设置消息发送回调函数
 * @param callback 回调函数指针
 */
void mcp_server_set_send_callback(mcp_send_message_callback_t callback);

/**
 * 解析字符串消息
 * @param server 服务器实例
 * @param message 消息字符串
 */
void mcp_server_parse_message(mcp_server_t* server, const char* message);

/**
 * 解析JSON消息
 * @param server 服务器实例
 * @param json JSON对象
 */
void mcp_server_parse_json_message(mcp_server_t* server, const cJSON* json);

/* 响应函数 */
/**
 * 回复成功结果
 * @param id 请求ID
 * @param result 结果字符串
 */
void mcp_server_reply_result(int id, const char* result);

/**
 * 回复错误信息
 * @param id 请求ID
 * @param message 错误消息
 */
void mcp_server_reply_error(int id, const char* message);

/* 处理器函数 */
/**
 * 处理初始化请求
 * @param server 服务器实例
 * @param id 请求ID
 * @param params 参数JSON对象
 */
void mcp_server_handle_initialize(mcp_server_t* server, int id, const cJSON* params);

/**
 * 处理工具列表请求
 * @param server 服务器实例
 * @param id 请求ID
 * @param params 参数JSON对象
 */
void mcp_server_handle_tools_list(mcp_server_t* server, int id, const cJSON* params);

/**
 * 处理工具调用请求
 * @param server 服务器实例
 * @param id 请求ID
 * @param params 参数JSON对象
 */
void mcp_server_handle_tools_call(mcp_server_t* server, int id, const cJSON* params);

/* 能力解析函数 */
/**
 * 解析能力配置
 * @param server 服务器实例
 * @param capabilities 能力配置JSON对象
 */
void mcp_server_parse_capabilities(mcp_server_t* server, const cJSON* capabilities);

/* 能力配置函数 */
/**
 * 设置能力回调函数
 * @param server 服务器实例
 * @param callbacks 回调函数结构体
 */
void mcp_server_set_capability_callbacks(mcp_server_t* server, const mcp_capability_callbacks_t* callbacks);

/* 工具函数 */
/**
 * 获取工具列表的JSON字符串
 * @param server 服务器实例
 * @param cursor 游标（用于分页）
 * @param list_user_only_tools 是否只列出用户专用工具
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_server_get_tools_list_json(const mcp_server_t* server, const char* cursor, bool list_user_only_tools);

#ifdef __cplusplus
}
#endif

#endif /* MCP_SERVER_H */