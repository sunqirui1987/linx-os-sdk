/*
 * MCP属性管理头文件
 * 定义属性结构体和属性管理相关的函数声明
 */

#ifndef MCP_PROPERTY_H
#define MCP_PROPERTY_H

#include "mcp_types.h"  // 包含MCP类型定义

#ifdef __cplusplus
extern "C" {
#endif

/* 属性结构体 */
typedef struct mcp_property {
    char name[MCP_MAX_NAME_LENGTH];     // 属性名称
    mcp_property_type_t type;           // 属性类型（布尔、整数、字符串）
    mcp_property_value_t value;         // 属性值联合体
    bool has_default_value;             // 是否有默认值
    bool has_range;                     // 是否有取值范围（仅对整数类型有效）
    int min_value;                      // 最小值（仅对整数类型有效）
    int max_value;                      // 最大值（仅对整数类型有效）
} mcp_property_t;

/* 属性列表结构体 */
typedef struct mcp_property_list {
    mcp_property_t properties[MCP_MAX_PROPERTIES];  // 属性数组
    size_t count;                                   // 属性数量
} mcp_property_list_t;

/* 属性操作函数 */

/**
 * 创建布尔类型属性
 * @param name 属性名称
 * @param default_value 默认值
 * @param has_default 是否有默认值
 * @return 创建的属性指针，失败返回NULL
 */
mcp_property_t* mcp_property_create_boolean(const char* name, bool default_value, bool has_default);

/**
 * 创建整数类型属性
 * @param name 属性名称
 * @param default_value 默认值
 * @param has_default 是否有默认值
 * @param has_range 是否有取值范围
 * @param min_value 最小值
 * @param max_value 最大值
 * @return 创建的属性指针，失败返回NULL
 */
mcp_property_t* mcp_property_create_integer(const char* name, int default_value, bool has_default, 
                                           bool has_range, int min_value, int max_value);

/**
 * 创建字符串类型属性
 * @param name 属性名称
 * @param default_value 默认值
 * @param has_default 是否有默认值
 * @return 创建的属性指针，失败返回NULL
 */
mcp_property_t* mcp_property_create_string(const char* name, const char* default_value, bool has_default);

/**
 * 设置布尔属性值
 * @param prop 属性指针
 * @param value 要设置的值
 * @return 成功返回0，失败返回非0
 */
int mcp_property_set_bool_value(mcp_property_t* prop, bool value);

/**
 * 设置整数属性值
 * @param prop 属性指针
 * @param value 要设置的值
 * @return 成功返回0，失败返回非0
 */
int mcp_property_set_int_value(mcp_property_t* prop, int value);

/**
 * 设置字符串属性值
 * @param prop 属性指针
 * @param value 要设置的值
 * @return 成功返回0，失败返回非0
 */
int mcp_property_set_string_value(mcp_property_t* prop, const char* value);

/**
 * 获取布尔属性值
 * @param prop 属性指针
 * @return 属性值
 */
bool mcp_property_get_bool_value(const mcp_property_t* prop);

/**
 * 获取整数属性值
 * @param prop 属性指针
 * @return 属性值
 */
int mcp_property_get_int_value(const mcp_property_t* prop);

/**
 * 获取字符串属性值
 * @param prop 属性指针
 * @return 属性值
 */
const char* mcp_property_get_string_value(const mcp_property_t* prop);

/**
 * 将属性转换为JSON字符串
 * @param prop 属性指针
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_property_to_json(const mcp_property_t* prop);

/**
 * 销毁属性并释放内存
 * @param prop 属性指针
 */
void mcp_property_destroy(mcp_property_t* prop);

/* 属性列表操作函数 */

/**
 * 创建属性列表
 * @return 创建的属性列表指针，失败返回NULL
 */
mcp_property_list_t* mcp_property_list_create(void);

/**
 * 销毁属性列表并释放内存
 * @param list 属性列表指针
 */
void mcp_property_list_destroy(mcp_property_list_t* list);

/**
 * 克隆属性列表（深拷贝）
 * @param list 要克隆的属性列表指针
 * @return 克隆的属性列表指针，失败返回NULL
 */
mcp_property_list_t* mcp_property_list_clone(const mcp_property_list_t* list);

/**
 * 向属性列表添加属性
 * @param list 属性列表指针
 * @param prop 要添加的属性
 * @return 成功返回true，失败返回false
 */
bool mcp_property_list_add(mcp_property_list_t* list, const mcp_property_t* prop);

/**
 * 在属性列表中查找属性（只读）
 * @param list 属性列表指针
 * @param name 属性名称
 * @return 找到的属性指针，未找到返回NULL
 */
const mcp_property_t* mcp_property_list_find(const mcp_property_list_t* list, const char* name);

/**
 * 在属性列表中查找属性（可修改）
 * @param list 属性列表指针
 * @param name 属性名称
 * @return 找到的属性指针，未找到返回NULL
 */
mcp_property_t* mcp_property_list_find_mutable(mcp_property_list_t* list, const char* name);

/**
 * 将属性列表转换为JSON字符串
 * @param list 属性列表指针
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_property_list_to_json(const mcp_property_list_t* list);

/**
 * 获取属性列表中必需属性的JSON字符串
 * @param list 属性列表指针
 * @return JSON字符串，需要调用者释放内存
 */
char* mcp_property_list_get_required_json(const mcp_property_list_t* list);

#ifdef __cplusplus
}
#endif

#endif /* MCP_PROPERTY_H */
