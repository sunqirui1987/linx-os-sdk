#ifndef WAKE_WORD_STUB_H
#define WAKE_WORD_STUB_H

#include "wake_word_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 唤醒词桩实现数据结构
 */
typedef struct {
    // 桩特定数据
    bool detection_enabled;         // 检测是否启用
    int detection_threshold;        // 检测阈值
    size_t samples_processed;       // 已处理的采样点数
    char detected_word[64];         // 检测到的唤醒词
    
    // 音频处理缓冲区
    int16_t* audio_buffer;          // 音频缓冲区
    size_t buffer_size;             // 缓冲区大小
    size_t buffer_pos;              // 缓冲区位置
    
    // Opus编码模拟
    uint8_t* opus_data;             // Opus数据
    size_t opus_size;               // Opus数据大小
} WakeWordStubData;

/**
 * 创建新的唤醒词桩实现
 * @return 创建的WakeWordInterface指针，失败时返回NULL
 */
WakeWordInterface* wake_word_stub_create(void);

/**
 * 销毁唤醒词桩实现
 * @param interface 要销毁的WakeWordInterface指针
 */
void wake_word_stub_destroy(WakeWordInterface* interface);

#ifdef __cplusplus
}
#endif

#endif // WAKE_WORD_STUB_H