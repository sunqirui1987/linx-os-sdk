#ifndef AUDIO_STUB_H
#define AUDIO_STUB_H

#include "audio_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 音频桩实现数据结构
 * 这是一个占位符实现，用于没有音频支持的平台或测试目的
 */
typedef struct {
    bool initialized;                       ///< 是否已初始化
    bool started;                           ///< 是否已启动
    
    // 桩特定的状态用于模拟
    size_t samples_written;                 ///< 已写入的样本总数（用于模拟）
    size_t samples_read;                    ///< 已读取的样本总数（用于模拟）
    unsigned int last_config_sample_rate;  ///< 最后配置的采样率
    int last_config_channels;              ///< 最后配置的通道数
} AudioStubData;

/**
 * 创建音频桩实现
 * 
 * 此函数创建一个音频桩接口，可用于测试或在没有真实音频硬件的平台上使用。
 * 桩实现将：
 * - 接受所有配置调用而不出错
 * - 为所有读取操作返回静音
 * - 丢弃所有写入操作
 * - 模拟适当的状态转换
 * 
 * @return AudioInterface实例，失败时返回NULL
 */
AudioInterface* audio_stub_create(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_STUB_H