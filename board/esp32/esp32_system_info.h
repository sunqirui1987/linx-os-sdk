#ifndef ESP32_SYSTEM_INFO_H
#define ESP32_SYSTEM_INFO_H

#include "common/system_info.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ESP32特定的系统信息实现
 */
typedef struct {
    SystemInfo base;  // 继承基类
    // ESP32特定的数据成员可以在这里添加
} ESP32SystemInfo;

/**
 * @brief 创建ESP32系统信息实例
 * @return ESP32SystemInfo* 创建的实例，失败返回NULL
 */
ESP32SystemInfo* esp32_system_info_create(void);

/**
 * @brief 销毁ESP32系统信息实例
 * @param self ESP32SystemInfo实例
 */
void esp32_system_info_destroy(ESP32SystemInfo* self);

#ifdef __cplusplus
}
#endif

#endif // ESP32_SYSTEM_INFO_H