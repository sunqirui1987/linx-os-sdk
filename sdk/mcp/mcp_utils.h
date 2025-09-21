/**
 * @file mcp_utils.h
 * @brief MCP工具函数库头文件
 * 
 * 提供MCP协议实现中常用的工具函数，包括：
 * - Base64编码功能
 * - 图像内容处理
 * - 字符串工具函数
 * - JSON工具函数
 */
#ifndef MCP_UTILS_H
#define MCP_UTILS_H

#include "mcp_types.h"        // 包含MCP类型定义

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 图像内容结构体
 * 
 * 用于存储图像数据的MIME类型和Base64编码后的数据
 */
typedef struct mcp_image_content {
    char* mime_type;      /**< 图像MIME类型（如"image/png", "image/jpeg"等） */
    char* encoded_data;   /**< Base64编码后的图像数据 */
} mcp_image_content_t;

/* Base64编码函数 */

/**
 * @brief 将二进制数据编码为Base64字符串
 * @param data 要编码的二进制数据
 * @param data_len 数据长度
 * @return 编码后的Base64字符串，失败返回NULL
 * @note 返回的字符串需要调用者释放内存
 */
char* mcp_base64_encode(const char* data, size_t data_len);

/* 图像内容操作函数 */

/**
 * @brief 创建图像内容对象
 * @param mime_type 图像MIME类型
 * @param data 原始图像数据
 * @param data_len 数据长度
 * @return 创建的图像内容对象，失败返回NULL
 * @note 会自动对图像数据进行Base64编码
 */
mcp_image_content_t* mcp_image_content_create(const char* mime_type, const char* data, size_t data_len);

/**
 * @brief 销毁图像内容对象
 * @param image 要销毁的图像内容对象
 */
void mcp_image_content_destroy(mcp_image_content_t* image);

/**
 * @brief 将图像内容转换为JSON字符串
 * @param image 图像内容对象
 * @return JSON字符串，失败返回NULL
 * @note 返回的字符串需要调用者释放内存
 */
char* mcp_image_content_to_json(const mcp_image_content_t* image);

/* 字符串工具函数 */

/**
 * @brief 复制字符串
 * @param str 要复制的字符串
 * @return 复制的字符串，失败返回NULL
 * @note 返回的字符串需要调用者释放内存
 */
char* mcp_strdup(const char* str);

/**
 * @brief 释放字符串内存
 * @param str 要释放的字符串
 */
void mcp_free_string(char* str);

/* 数字转换工具函数 */

/**
 * @brief 将整数转换为字符串
 * @param value 要转换的整数值
 * @return 转换后的字符串，失败返回NULL
 * @note 返回的字符串需要调用者释放内存
 */
char* mcp_itoa(int value);

/* JSON工具函数 */

/**
 * @brief 将cJSON对象转换为字符串
 * @param json cJSON对象
 * @return JSON字符串，失败返回NULL
 * @note 返回的字符串需要调用者释放内存
 */
char* mcp_json_to_string(const cJSON* json);

#ifdef __cplusplus
}
#endif

#endif /* MCP_UTILS_H */