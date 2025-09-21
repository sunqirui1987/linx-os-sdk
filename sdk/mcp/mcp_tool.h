/*
 * MCP工具管理头文件
 * 定义工具结构体和工具管理相关的函数声明
 */

#ifndef MCP_TOOL_H
#define MCP_TOOL_H

#include "mcp_types.h"      // 包含MCP类型定义
#include "mcp_property.h"   // 包含属性管理
#include "mcp_utils.h"      // 包含工具函数

#ifdef __cplusplus
extern "C" {
#endif

/* 工具结构体 */
typedef struct mcp_tool {
    char name[MCP_MAX_NAME_LENGTH];                     // 工具名称
    char description[MCP_MAX_DESCRIPTION_LENGTH];       // 工具描述
    mcp_property_list_t* properties;                    // 工具参数列表
    mcp_tool_callback_t callback;                       // 工具回调函数
    bool user_only;                                     // 是否仅限用户使用
} mcp_tool_t;

/* 工具操作函数 */

/**
 * 创建工具
 * @param name 工具名称
 * @param description 工具描述
 * @param properties 工具参数列表
 * @param callback 工具回调函数
 * @return 创建的工具指针，失败返回NULL
 */
mcp_tool_t* mcp_tool_create(const char* name, const char* description, 
                           mcp_property_list_t* properties, mcp_tool_callback_t callback);

/**
 * 销毁工具并释放内存
 * @param tool 工具指针
 */
void mcp_tool_destroy(mcp_tool_t* tool);

/**
 * 设置工具是否仅限用户使用
 * @param tool 工具指针
 * @param user_only 是否仅限用户使用
 */
void mcp_tool_set_user_only(mcp_tool_t* tool, bool user_only);

/**
 * 检查工具是否仅限用户使用
 * @param tool 工具指针
 * @return 如果仅限用户使用返回true，否则返回false
 */
bool mcp_tool_is_user_only(const mcp_tool_t* tool);

/**
 * 将工具转换为JSON字符串
 * @param tool 工具指针
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_tool_to_json(const mcp_tool_t* tool);

/**
 * 调用工具并获取结果
 * @param tool 工具指针
 * @param properties 调用参数
 * @return 调用结果的JSON字符串，需要调用者释放内存
 */
char* mcp_tool_call(const mcp_tool_t* tool, const mcp_property_list_t* properties);

/* 返回值辅助函数 */

/**
 * 创建布尔类型返回值
 * @param value 布尔值
 * @return 返回值结构体
 */
mcp_return_value_t mcp_return_bool(bool value);

/**
 * 创建整数类型返回值
 * @param value 整数值
 * @return 返回值结构体
 */
mcp_return_value_t mcp_return_int(int value);

/**
 * 创建字符串类型返回值
 * @param value 字符串值
 * @return 返回值结构体
 */
mcp_return_value_t mcp_return_string(const char* value);

/**
 * 创建JSON类型返回值
 * @param value JSON对象
 * @return 返回值结构体
 */
mcp_return_value_t mcp_return_json(cJSON* value);

/**
 * 创建图像类型返回值
 * @param value 图像内容
 * @return 返回值结构体
 */
mcp_return_value_t mcp_return_image(mcp_image_content_t* value);

/* 返回值清理函数 */

/**
 * 清理返回值并释放内存
 * @param ret_val 返回值指针
 * @param type 返回值类型
 */
void mcp_return_value_cleanup(mcp_return_value_t* ret_val, mcp_return_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TOOL_H */