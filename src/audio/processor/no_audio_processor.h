#ifndef NO_AUDIO_PROCESSOR_H
#define NO_AUDIO_PROCESSOR_H

#include "audio_processor.h"
#include "../audio/audio_interface.h"
#include "../common/std/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file no_audio_processor.h
 * @brief 无音频处理实现
 * @details 这是一个不进行任何音频处理的处理器，仅进行通道转换和数据透传
 */

/**
 * @brief 创建无音频处理器实例
 * @return 音频处理器指针，失败时返回NULL
 */
AudioProcessor* no_audio_processor_create(void);

/**
 * @brief 销毁无音频处理器实例
 * @param processor 音频处理器指针
 */
void no_audio_processor_destroy(AudioProcessor* processor);

#ifdef __cplusplus
}
#endif

#endif // NO_AUDIO_PROCESSOR_H