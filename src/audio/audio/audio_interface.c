#include "audio_interface.h"
#include "../../common/log/linx_log.h"

// ============================================================================
// 核心生命周期函数实现
// ============================================================================

int audio_interface_init(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->init) {
        LOG_ERROR("无效的音频接口或虚函数表");
        return AUDIO_ERROR_INVALID;
    }
    return self->vtable->init(self);
}

int audio_interface_start(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    if (self->vtable && self->vtable->start) {
        return self->vtable->start(self);
    }
    
    // 默认实现：设置默认值并启用输入输出
    if (self->output_volume_ <= 0) {
        self->output_volume_ = AUDIO_VOLUME_DEFAULT;
        LOG_INFO("设置默认输出音量为 %d", self->output_volume_);
    }
    
    self->is_started = true;
    
    // 启用输入和输出
    audio_interface_enable_input(self, true);
    audio_interface_enable_output(self, true);
    
    LOG_INFO("音频接口启动成功");
    return AUDIO_SUCCESS;
}

int audio_interface_destroy(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->destroy) {
        LOG_ERROR("无效的音频接口或虚函数表");
        return AUDIO_ERROR_INVALID;
    }
    
    return self->vtable->destroy(self);
}

// ============================================================================
// 配置函数实现
// ============================================================================

void audio_interface_set_config(AudioInterface* self, unsigned int sample_rate, 
                               int frame_size, int channels, int periods, 
                               int buffer_size, int period_size) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return;
    }
    
    // 存储配置到接口结构体中
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    LOG_INFO("音频配置设置: %u Hz, %d 通道, %d 帧大小", 
             sample_rate, channels, frame_size);
    
    if (self->vtable && self->vtable->set_config) {
        self->vtable->set_config(self, sample_rate, frame_size, channels, 
                                periods, buffer_size, period_size);
    }
}

// ============================================================================
// 音量控制函数实现
// ============================================================================

int audio_interface_set_output_volume(AudioInterface* self, int volume) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    // 限制音量范围
    if (volume < AUDIO_VOLUME_MIN) volume = AUDIO_VOLUME_MIN;
    if (volume > AUDIO_VOLUME_MAX) volume = AUDIO_VOLUME_MAX;
    
    self->output_volume_ = volume;
    LOG_INFO("设置输出音量为 %d", volume);
    
    if (self->vtable && self->vtable->set_output_volume) {
        return self->vtable->set_output_volume(self, volume);
    }
    
    return AUDIO_SUCCESS;
}

// ============================================================================
// 输入输出管理函数实现
// ============================================================================

int audio_interface_enable_input(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    if (enable == self->input_enabled_) {
        return AUDIO_SUCCESS; // 状态没有变化
    }
    
    self->input_enabled_ = enable;
    LOG_INFO("设置输入启用状态为 %s", enable ? "true" : "false");
    
    if (self->vtable && self->vtable->enable_input) {
        return self->vtable->enable_input(self, enable);
    }
    
    return AUDIO_SUCCESS;
}

int audio_interface_enable_output(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    if (enable == self->output_enabled_) {
        return AUDIO_SUCCESS; // 状态没有变化
    }
    
    self->output_enabled_ = enable;
    LOG_INFO("设置输出启用状态为 %s", enable ? "true" : "false");
    
    if (self->vtable && self->vtable->enable_output) {
        return self->vtable->enable_output(self, enable);
    }
    
    return AUDIO_SUCCESS;
}

// ============================================================================
// 高级数据处理函数实现（对齐AudioCodec::OutputData和InputData）
// ============================================================================

int audio_interface_output_data(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("无效的音频接口或数据参数");
        return AUDIO_ERROR_INVALID;
    }
    
    if (self->vtable && self->vtable->output_data) {
        return self->vtable->output_data(self, data, samples);
    }
    
    // 默认实现：调用底层write方法
    if (self->vtable && self->vtable->write) {
        return self->vtable->write(self, data, samples);
    }
    
    LOG_ERROR("未实现输出数据方法");
    return AUDIO_ERROR_INVALID;
}

int audio_interface_input_data(AudioInterface* self, int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("无效的音频接口或数据参数");
        return AUDIO_ERROR_INVALID;
    }
    
    if (self->vtable && self->vtable->input_data) {
        return self->vtable->input_data(self, data, samples);
    }
    
    // 默认实现：调用底层read方法
    if (self->vtable && self->vtable->read) {
        return self->vtable->read(self, data, samples);
    }
    
    LOG_ERROR("未实现输入数据方法");
    return AUDIO_ERROR_INVALID;
}

// ============================================================================
// 底层读写函数实现（对齐AudioCodec::Read和Write）
// ============================================================================

int audio_interface_read(AudioInterface* self, int16_t* dest, size_t samples) {
    if (!self || !self->vtable || !self->vtable->read) {
        LOG_ERROR("无效的音频接口或虚函数表");
        return AUDIO_ERROR_INVALID;
    }
    return self->vtable->read(self, dest, samples);
}

int audio_interface_write(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !self->vtable || !self->vtable->write) {
        LOG_ERROR("无效的音频接口或虚函数表");
        return AUDIO_ERROR_INVALID;
    }
    return self->vtable->write(self, data, samples);
}

// ============================================================================
// Getter函数实现（对齐AudioCodec的inline getter方法）
// ============================================================================

bool audio_interface_duplex(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return false;
    }
    return self->duplex_;
}

bool audio_interface_input_reference(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return false;
    }
    return self->input_reference_;
}

int audio_interface_input_sample_rate(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    return self->input_sample_rate_ > 0 ? self->input_sample_rate_ : (int)self->sample_rate;
}

int audio_interface_output_sample_rate(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    return self->output_sample_rate_ > 0 ? self->output_sample_rate_ : (int)self->sample_rate;
}

int audio_interface_input_channels(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    return self->input_channels_ > 0 ? self->input_channels_ : self->channels;
}

int audio_interface_output_channels(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    return self->output_channels_ > 0 ? self->output_channels_ : self->channels;
}

int audio_interface_output_volume(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    return self->output_volume_;
}

bool audio_interface_input_enabled(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return false;
    }
    return self->input_enabled_;
}

bool audio_interface_output_enabled(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return false;
    }
    return self->output_enabled_;
}