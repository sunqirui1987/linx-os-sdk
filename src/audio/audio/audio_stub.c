#include "audio_stub.h"
#include "../../common/log/linx_log.h"
#include <stdlib.h>
#include <string.h>

// 虚函数表函数前向声明
static int audio_stub_init(AudioInterface* self);
static int audio_stub_start(AudioInterface* self);
static int audio_stub_destroy(AudioInterface* self);
static int audio_stub_set_output_volume(AudioInterface* self, int volume);
static int audio_stub_enable_input(AudioInterface* self, bool enable);
static int audio_stub_enable_output(AudioInterface* self, bool enable);
static int audio_stub_output_data(AudioInterface* self, const int16_t* data, size_t samples);
static int audio_stub_input_data(AudioInterface* self, int16_t* data, size_t samples);
static int audio_stub_read(AudioInterface* self, int16_t* dest, size_t samples);
static int audio_stub_write(AudioInterface* self, const int16_t* data, size_t samples);
static void audio_stub_set_config(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                 int channels, int periods, int buffer_size, int period_size);

// 完整的桩虚函数表，对齐AudioCodec功能
static const AudioInterfaceVTable audio_stub_vtable = {
    // 核心生命周期函数
    .init = audio_stub_init,
    .start = audio_stub_start,
    .destroy = audio_stub_destroy,
    
    // 音量控制函数
    .set_output_volume = audio_stub_set_output_volume,
    
    // 输入输出管理函数
    .enable_input = audio_stub_enable_input,
    .enable_output = audio_stub_enable_output,
    
    // 高级数据处理函数
    .output_data = audio_stub_output_data,
    .input_data = audio_stub_input_data,
    
    // 底层读写函数
    .read = audio_stub_read,
    .write = audio_stub_write,
    
    // 配置函数
    .set_config = audio_stub_set_config
};

AudioInterface* audio_stub_create(void) {
    AudioInterface* interface = (AudioInterface*)malloc(sizeof(AudioInterface));
    if (!interface) {
        LOG_ERROR("分配AudioInterface内存失败");
        return NULL;
    }
    
    AudioStubData* data = (AudioStubData*)malloc(sizeof(AudioStubData));
    if (!data) {
        LOG_ERROR("分配AudioStubData内存失败");
        free(interface);
        return NULL;
    }
    
    // 初始化接口结构体
    memset(interface, 0, sizeof(AudioInterface));
    interface->vtable = &audio_stub_vtable;
    interface->impl_data = data;
    
    // 初始化默认值（对齐AudioCodec的默认值）
    interface->output_volume_ = AUDIO_VOLUME_DEFAULT;
    interface->input_enabled_ = false;
    interface->output_enabled_ = false;
    interface->is_started = false;
    interface->duplex_ = true;                // 桩支持全双工
    interface->input_reference_ = false;
    interface->input_sample_rate_ = 0;        // 使用通用采样率
    interface->output_sample_rate_ = 0;       // 使用通用采样率
    interface->input_channels_ = 1;           // 默认单声道输入
    interface->output_channels_ = 1;          // 默认单声道输出
    
    // 初始化桩数据
    memset(data, 0, sizeof(AudioStubData));
    
    LOG_INFO("音频桩接口创建成功");
    return interface;
}

// ============================================================================
// 核心生命周期函数实现
// ============================================================================

static int audio_stub_init(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("无效的桩数据");
        return AUDIO_ERROR_INVALID;
    }
    
    data->initialized = true;
    self->is_initialized = true;
    
    LOG_INFO("音频桩初始化成功");
    return AUDIO_SUCCESS;
}

static int audio_stub_start(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("无效的桩数据");
        return AUDIO_ERROR_INVALID;
    }
    
    // 设置默认值（对齐AudioCodec::Start的逻辑）
    if (self->output_volume_ <= 0) {
        self->output_volume_ = AUDIO_VOLUME_DEFAULT;
        LOG_INFO("桩：设置默认输出音量为 %d", self->output_volume_);
    }
    
    data->started = true;
    self->is_started = true;
    
    // 启用输入和输出（对齐AudioCodec::Start的行为）
    audio_stub_enable_input(self, true);
    audio_stub_enable_output(self, true);
    
    LOG_INFO("音频桩启动成功");
    return AUDIO_SUCCESS;
}

static int audio_stub_destroy(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (data) {
        LOG_INFO("销毁音频桩（已写入: %zu 样本，已读取: %zu 样本）", 
                 data->samples_written, data->samples_read);
        free(data);
        self->impl_data = NULL;
    }
    
    LOG_INFO("音频桩销毁成功");
    return AUDIO_SUCCESS;
}

// ============================================================================
// 音量控制函数实现
// ============================================================================

static int audio_stub_set_output_volume(AudioInterface* self, int volume) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    // 限制音量范围
    if (volume < AUDIO_VOLUME_MIN) volume = AUDIO_VOLUME_MIN;
    if (volume > AUDIO_VOLUME_MAX) volume = AUDIO_VOLUME_MAX;
    
    self->output_volume_ = volume;
    LOG_INFO("桩：设置输出音量为 %d", volume);
    return AUDIO_SUCCESS;
}

// ============================================================================
// 输入输出管理函数实现
// ============================================================================

static int audio_stub_enable_input(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    self->input_enabled_ = enable;
    LOG_INFO("桩：输入 %s", enable ? "启用" : "禁用");
    return AUDIO_SUCCESS;
}

static int audio_stub_enable_output(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    self->output_enabled_ = enable;
    LOG_INFO("桩：输出 %s", enable ? "启用" : "禁用");
    return AUDIO_SUCCESS;
}

// ============================================================================
// 高级数据处理函数实现（对齐AudioCodec::OutputData和InputData）
// ============================================================================

static int audio_stub_output_data(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("输出数据参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    if (!self->output_enabled_) {
        LOG_WARN("输出已禁用，忽略输出数据调用");
        return AUDIO_ERROR_DEVICE;
    }
    
    AudioStubData* stub_data = (AudioStubData*)self->impl_data;
    if (stub_data) {
        stub_data->samples_written += samples;
    }
    
    // 桩实现：直接丢弃数据
    LOG_DEBUG("桩：输出 %zu 样本（总计: %zu）", samples, 
              stub_data ? stub_data->samples_written : 0);
    return (int)samples; // 返回实际处理的样本数
}

static int audio_stub_input_data(AudioInterface* self, int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("输入数据参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    if (!self->input_enabled_) {
        LOG_WARN("输入已禁用，忽略输入数据调用");
        return AUDIO_ERROR_DEVICE;
    }
    
    AudioStubData* stub_data = (AudioStubData*)self->impl_data;
    if (stub_data) {
        stub_data->samples_read += samples;
    }
    
    // 桩实现：返回静音
    memset(data, 0, samples * sizeof(int16_t));
    
    LOG_DEBUG("桩：输入 %zu 样本（静音，总计: %zu）", samples,
              stub_data ? stub_data->samples_read : 0);
    return (int)samples; // 返回实际读取的样本数
}

// ============================================================================
// 底层读写函数实现（对齐AudioCodec::Read和Write）
// ============================================================================

static int audio_stub_read(AudioInterface* self, int16_t* dest, size_t samples) {
    if (!self || !dest || samples == 0) {
        LOG_ERROR("读取参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data || !data->initialized) {
        LOG_ERROR("桩未初始化");
        return AUDIO_ERROR_NOT_INIT;
    }
    
    // 桩实现：返回静音
    memset(dest, 0, samples * sizeof(int16_t));
    return (int)samples; // 返回读取的样本数
}

static int audio_stub_write(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("写入参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    AudioStubData* stub_data = (AudioStubData*)self->impl_data;
    if (!stub_data || !stub_data->initialized) {
        LOG_ERROR("桩未初始化");
        return AUDIO_ERROR_NOT_INIT;
    }
    
    // 桩实现：直接丢弃数据
    return (int)samples; // 返回写入的样本数
}

// ============================================================================
// 配置函数实现
// ============================================================================

static void audio_stub_set_config(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                 int channels, int periods, int buffer_size, int period_size) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return;
    }
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) {
        LOG_ERROR("无效的桩数据");
        return;
    }
    
    // 存储配置到接口结构体中
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    // 存储到桩数据中用于模拟
    data->last_config_sample_rate = sample_rate;
    data->last_config_channels = channels;
    
    LOG_INFO("音频桩配置设置: %u Hz, %d 通道, %d 帧大小", 
             sample_rate, channels, frame_size);
}