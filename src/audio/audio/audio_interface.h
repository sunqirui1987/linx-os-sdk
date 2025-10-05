#ifndef AUDIO_INTERFACE_H
#define AUDIO_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 音频接口错误码定义
 */
#define AUDIO_SUCCESS           0
#define AUDIO_ERROR_INVALID     -1
#define AUDIO_ERROR_NOT_INIT    -2
#define AUDIO_ERROR_NO_MEMORY   -3
#define AUDIO_ERROR_DEVICE      -4
#define AUDIO_ERROR_TIMEOUT     -5
#define AUDIO_ERROR_OVERFLOW    -6
#define AUDIO_ERROR_UNDERRUN    -7

/**
 * 音频接口常量定义
 */
#define AUDIO_VOLUME_MIN        0
#define AUDIO_VOLUME_MAX        100
#define AUDIO_VOLUME_DEFAULT    70

/**
 * 音频接口结构体前向声明（C99兼容性）
 */
typedef struct AudioInterface AudioInterface;

/**
 * 音频接口虚函数表
 * 所有实现必须提供这些函数指针，完全对齐AudioCodec的功能
 */
typedef struct {
    // 核心生命周期函数
    int (*init)(AudioInterface* self);                                          ///< 初始化音频接口（合并了init和init_play）
    int (*start)(AudioInterface* self);                                         ///< 启动音频接口
    int (*destroy)(AudioInterface* self);                                       ///< 销毁和清理接口
    
    // 音量控制函数
    int (*set_output_volume)(AudioInterface* self, int volume);                 ///< 设置输出音量 (0-100)
    
    // 输入输出管理函数
    int (*enable_input)(AudioInterface* self, bool enable);                     ///< 启用/禁用输入
    int (*enable_output)(AudioInterface* self, bool enable);                    ///< 启用/禁用输出
    
    // 高级数据处理函数（对齐AudioCodec::OutputData和InputData）
    int (*output_data)(AudioInterface* self, const int16_t* data, size_t samples);  ///< 输出音频数据
    int (*input_data)(AudioInterface* self, int16_t* data, size_t samples);         ///< 输入音频数据，返回实际读取的样本数
    
    // 底层读写函数（对齐AudioCodec::Read和Write）
    int (*read)(AudioInterface* self, int16_t* dest, size_t samples);           ///< 底层读取音频数据
    int (*write)(AudioInterface* self, const int16_t* data, size_t samples);    ///< 底层写入音频数据
    
    // 配置函数
    void (*set_config)(AudioInterface* self, unsigned int sample_rate,          ///< 设置音频配置
                      int frame_size, int channels, int periods, 
                      int buffer_size, int period_size);
} AudioInterfaceVTable;

/**
 * 音频接口基础结构体
 * 提供所有音频实现的通用接口，完全对齐AudioCodec的功能
 */
struct AudioInterface {
    const AudioInterfaceVTable* vtable;     ///< 虚函数表指针
    void* impl_data;                        ///< 实现特定的私有数据
    
    // 音频配置参数
    unsigned int sample_rate;               ///< 采样率（Hz，如44100, 48000）
    int frame_size;                         ///< 帧大小（每通道样本数）
    int channels;                           ///< 音频通道数（1=单声道，2=立体声）
    int periods;                            ///< 缓冲区周期数
    int buffer_size;                        ///< 总缓冲区大小（样本数）
    int period_size;                        ///< 周期大小（样本数）
    
    // 运行时状态标志
    bool is_initialized;                    ///< 是否已初始化
    bool is_started;                        ///< 是否已启动
    
    // 音频控制状态（对齐AudioCodec的成员变量）
    int output_volume_;                     ///< 输出音量级别 (0-100)
    bool input_enabled_;                    ///< 输入是否启用
    bool output_enabled_;                   ///< 输出是否启用
    bool duplex_;                           ///< 是否支持全双工操作
    bool input_reference_;                  ///< 是否有输入参考
    
    // 分离的输入/输出配置（对齐AudioCodec）
    int input_sample_rate_;                 ///< 输入特定采样率（0 = 使用通用采样率）
    int output_sample_rate_;                ///< 输出特定采样率（0 = 使用通用采样率）
    int input_channels_;                    ///< 输入特定通道数（0 = 使用通用通道数）
    int output_channels_;                   ///< 输出特定通道数（0 = 使用通用通道数）
};

// ============================================================================
// 公共接口函数（对齐AudioCodec的公共方法）
// ============================================================================

/**
 * 初始化音频接口
 */
int audio_interface_init(AudioInterface* self);

/**
 * 启动音频接口（对齐AudioCodec::Start）
 */
int audio_interface_start(AudioInterface* self);

/**
 * 销毁音频接口
 */
int audio_interface_destroy(AudioInterface* self);

/**
 * 设置音频配置
 */
void audio_interface_set_config(AudioInterface* self, unsigned int sample_rate, 
                               int frame_size, int channels, int periods, 
                               int buffer_size, int period_size);

/**
 * 设置输出音量（对齐AudioCodec::SetOutputVolume）
 */
int audio_interface_set_output_volume(AudioInterface* self, int volume);

/**
 * 启用/禁用输入（对齐AudioCodec::EnableInput）
 */
int audio_interface_enable_input(AudioInterface* self, bool enable);

/**
 * 启用/禁用输出（对齐AudioCodec::EnableOutput）
 */
int audio_interface_enable_output(AudioInterface* self, bool enable);

/**
 * 输出音频数据（对齐AudioCodec::OutputData）
 */
int audio_interface_output_data(AudioInterface* self, const int16_t* data, size_t samples);

/**
 * 输入音频数据（对齐AudioCodec::InputData）
 */
int audio_interface_input_data(AudioInterface* self, int16_t* data, size_t samples);

/**
 * 底层读取音频数据（对齐AudioCodec::Read）
 */
int audio_interface_read(AudioInterface* self, int16_t* dest, size_t samples);

/**
 * 底层写入音频数据（对齐AudioCodec::Write）
 */
int audio_interface_write(AudioInterface* self, const int16_t* data, size_t samples);

// ============================================================================
// Getter函数（对齐AudioCodec的inline getter方法）
// ============================================================================

/**
 * 获取是否支持全双工
 */
bool audio_interface_duplex(AudioInterface* self);

/**
 * 获取是否有输入参考
 */
bool audio_interface_input_reference(AudioInterface* self);

/**
 * 获取输入采样率
 */
int audio_interface_input_sample_rate(AudioInterface* self);

/**
 * 获取输出采样率
 */
int audio_interface_output_sample_rate(AudioInterface* self);

/**
 * 获取输入通道数
 */
int audio_interface_input_channels(AudioInterface* self);

/**
 * 获取输出通道数
 */
int audio_interface_output_channels(AudioInterface* self);

/**
 * 获取输出音量
 */
int audio_interface_output_volume(AudioInterface* self);

/**
 * 获取输入是否启用
 */
bool audio_interface_input_enabled(AudioInterface* self);

/**
 * 获取输出是否启用
 */
bool audio_interface_output_enabled(AudioInterface* self);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_INTERFACE_H