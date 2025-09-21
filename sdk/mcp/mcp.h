#ifndef MCP_H
#define MCP_H

/*
 * MCP (模型上下文协议) C语言实现
 * 参考规范: https://modelcontextprotocol.io/specification/2024-11-05
 * 
 * 这是MCP服务器功能的C99实现，
 * 从原始的C++实现移植而来。
 */

#include "mcp_types.h"      // MCP类型定义
#include "mcp_utils.h"      // MCP工具函数
#include "mcp_property.h"   // MCP属性管理
#include "mcp_tool.h"       // MCP工具管理
#include "mcp_server.h"     // MCP服务器实现
#include "../log/linx_log.h" // 日志模块

#ifdef __cplusplus
extern "C" {
#endif

/* 版本信息 */
#define MCP_VERSION_MAJOR 1         // 主版本号
#define MCP_VERSION_MINOR 0         // 次版本号
#define MCP_VERSION_PATCH 0         // 补丁版本号
#define MCP_VERSION_STRING "1.0.0"  // 版本字符串

/* 协议版本 */
#define MCP_PROTOCOL_VERSION "2024-11-05"  // MCP协议版本

/* 创建属性的便利宏定义 */

/* 创建布尔类型属性（带默认值） */
#define MCP_PROPERTY_BOOL(name, default_val) \
    mcp_property_create_boolean(name, default_val, true)

/* 创建必需的布尔类型属性（无默认值） */
#define MCP_PROPERTY_BOOL_REQUIRED(name) \
    mcp_property_create_boolean(name, false, false)

/* 创建整数类型属性（带默认值） */
#define MCP_PROPERTY_INT(name, default_val) \
    mcp_property_create_integer(name, default_val, true, false, 0, 0)

/* 创建必需的整数类型属性（无默认值） */
#define MCP_PROPERTY_INT_REQUIRED(name) \
    mcp_property_create_integer(name, 0, false, false, 0, 0)

/* 创建带范围限制的整数类型属性（带默认值） */
#define MCP_PROPERTY_INT_RANGE(name, default_val, min_val, max_val) \
    mcp_property_create_integer(name, default_val, true, true, min_val, max_val)

/* 创建必需的带范围限制的整数类型属性（无默认值） */
#define MCP_PROPERTY_INT_RANGE_REQUIRED(name, min_val, max_val) \
    mcp_property_create_integer(name, 0, false, true, min_val, max_val)

/* 创建字符串类型属性（带默认值） */
#define MCP_PROPERTY_STRING(name, default_val) \
    mcp_property_create_string(name, default_val, true)

/* 创建必需的字符串类型属性（无默认值） */
#define MCP_PROPERTY_STRING_REQUIRED(name) \
    mcp_property_create_string(name, NULL, false)

/* 返回值的便利宏定义 */
#define MCP_RETURN_TRUE() mcp_return_bool(true)        // 返回真值
#define MCP_RETURN_FALSE() mcp_return_bool(false)      // 返回假值
#define MCP_RETURN_SUCCESS() mcp_return_bool(true)     // 返回成功
#define MCP_RETURN_FAILURE() mcp_return_bool(false)    // 返回失败

#ifdef __cplusplus
}
#endif

#endif /* MCP_H */