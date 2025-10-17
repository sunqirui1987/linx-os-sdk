#ifndef LINX_AUDIO_BUILTIN_GAIN_H
#define LINX_AUDIO_BUILTIN_GAIN_H

#include "../plugin_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建增益插件实例
 * @param config 插件配置
 * @return 插件实例指针，失败返回NULL
 */
linx_plugin_base_t* create_gain_plugin(const linx_plugin_config_t* config);

/**
 * @brief 销毁增益插件实例
 * @param plugin 插件实例
 */
void destroy_gain_plugin(linx_plugin_base_t* plugin);

/**
 * @brief 获取增益插件元数据
 * @param metadata 元数据指针
 * @return 操作结果
 */
linx_audio_result_t get_gain_plugin_metadata(linx_plugin_metadata_t* metadata);

/**
 * @brief 获取gain插件描述符
 * @return 插件描述符指针
 */
const linx_plugin_descriptor_t* linx_gain_plugin_get_descriptor(void);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_BUILTIN_GAIN_H