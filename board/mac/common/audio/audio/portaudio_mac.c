#include "portaudio_mac.h"
#include "common/log/linx_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORTAUDIO_AVAILABLE 1

#if PORTAUDIO_AVAILABLE

/**
 * PortAudio实现数据结构
 */
typedef struct {
    PaStream* input_stream;                 ///< 输入音频流
    PaStream* output_stream;                ///< 输出音频流
    PaStreamParameters input_params;        ///< 输入流参数
    PaStreamParameters output_params;       ///< 输出流参数
    
    // 音频数据环形缓冲区
    int16_t* record_buffer;                 ///< 录音缓冲区
    int16_t* play_buffer;                   ///< 播放缓冲区
    size_t record_buffer_size;              ///< 录音缓冲区大小
    size_t play_buffer_size;                ///< 播放缓冲区大小
    size_t record_read_pos;                 ///< 录音读取位置
    size_t record_write_pos;                ///< 录音写入位置
    size_t play_read_pos;                   ///< 播放读取位置
    size_t play_write_pos;                  ///< 播放写入位置
    
    // 线程同步
    pthread_mutex_t record_mutex;           ///< 录音互斥锁
    pthread_mutex_t play_mutex;             ///< 播放互斥锁
    pthread_cond_t record_cond;             ///< 录音条件变量
    pthread_cond_t play_cond;               ///< 播放条件变量
    
    // 状态标志
    bool input_stream_active;               ///< 输入流是否活跃
    bool output_stream_active;              ///< 输出流是否活跃
} PortAudioMacData;

// 虚函数表函数前向声明
static int portaudio_mac_init(AudioInterface* self);
static int portaudio_mac_start(AudioInterface* self);
static int portaudio_mac_destroy(AudioInterface* self);
static int portaudio_mac_set_output_volume(AudioInterface* self, int volume);
static int portaudio_mac_enable_input(AudioInterface* self, bool enable);
static int portaudio_mac_enable_output(AudioInterface* self, bool enable);
static int portaudio_mac_output_data(AudioInterface* self, const int16_t* data, size_t samples);
static int portaudio_mac_input_data(AudioInterface* self, int16_t* data, size_t samples);
static int portaudio_mac_read(AudioInterface* self, int16_t* dest, size_t samples);
static int portaudio_mac_write(AudioInterface* self, const int16_t* data, size_t samples);
static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                     int channels, int periods, int buffer_size, int period_size);

// PortAudio回调函数前向声明
// static int _portaudio_record_callback(const void* input_buffer, void* output_buffer,
//                                      unsigned long frame_count,
//                                      const PaStreamCallbackTimeInfo* time_info,
//                                      PaStreamCallbackFlags status_flags,
//                                      void* user_data);

// static int _portaudio_play_callback(const void* input_buffer, void* output_buffer,
//                                    unsigned long frame_count,
//                                    const PaStreamCallbackTimeInfo* time_info,
//                                    PaStreamCallbackFlags status_flags,
//                                    void* user_data);

// 内部辅助函数前向声明
static int _start_input_stream(AudioInterface* self);
static int _start_output_stream(AudioInterface* self);

// PortAudio Mac虚函数表，对齐AudioCodec功能
static const AudioInterfaceVTable portaudio_mac_vtable = {
    // 核心生命周期函数
    .init = portaudio_mac_init,
    .start = portaudio_mac_start,
    .destroy = portaudio_mac_destroy,
    
    // 音量控制函数
    .set_output_volume = portaudio_mac_set_output_volume,
    
    // 输入输出管理函数
    .enable_input = portaudio_mac_enable_input,
    .enable_output = portaudio_mac_enable_output,
    
    // 高级数据处理函数
    .output_data = portaudio_mac_output_data,
    .input_data = portaudio_mac_input_data,
    
    // 底层读写函数
    .read = portaudio_mac_read,
    .write = portaudio_mac_write,
    
    // 配置函数
    .set_config = portaudio_mac_set_config
};

AudioInterface* portaudio_mac_create(void) {
    AudioInterface* interface = (AudioInterface*)malloc(sizeof(AudioInterface));
    if (!interface) {
        LOG_ERROR("分配AudioInterface内存失败");
        return NULL;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)malloc(sizeof(PortAudioMacData));
    if (!data) {
        LOG_ERROR("分配PortAudioMacData内存失败");
        free(interface);
        return NULL;
    }
    
    // 初始化结构体
    memset(interface, 0, sizeof(AudioInterface));
    memset(data, 0, sizeof(PortAudioMacData));
    
    interface->vtable = &portaudio_mac_vtable;
    interface->impl_data = data;
    
    // 初始化默认值（对齐AudioCodec的默认值）
    interface->output_volume_ = AUDIO_VOLUME_DEFAULT;
    interface->input_enabled_ = false;
    interface->output_enabled_ = false;
    interface->is_started = false;
    interface->duplex_ = true;               // 支持全双工
    interface->input_reference_ = false;     // 无输入参考
    interface->input_sample_rate_ = 0;       // 使用通用采样率
    interface->output_sample_rate_ = 0;      // 使用通用采样率
    interface->input_channels_ = 1;          // 默认单声道输入
    interface->output_channels_ = 1;         // 默认单声道输出
    
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&data->record_mutex, NULL);
    pthread_mutex_init(&data->play_mutex, NULL);
    pthread_cond_init(&data->record_cond, NULL);
    pthread_cond_init(&data->play_cond, NULL);
    
    LOG_INFO("PortAudio Mac接口创建成功");
    return interface;
}

// ============================================================================
// 核心生命周期函数实现
// ============================================================================

static int portaudio_mac_init(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        LOG_ERROR("初始化PortAudio失败: %s", Pa_GetErrorText(err));
        return AUDIO_ERROR_DEVICE;
    }
    
    self->is_initialized = true;
    LOG_INFO("PortAudio初始化成功");
    return AUDIO_SUCCESS;
}

static int portaudio_mac_start(AudioInterface* self) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    // 设置默认值（对齐AudioCodec::Start的逻辑）
    if (self->output_volume_ <= 0) {
        self->output_volume_ = AUDIO_VOLUME_DEFAULT;
        LOG_INFO("设置默认输出音量为 %d", self->output_volume_);
    }
    
    self->is_started = true;
    
    // 启用输入和输出（对齐AudioCodec::Start的行为）
    portaudio_mac_enable_input(self, true);
    portaudio_mac_enable_output(self, true);
    
    // 启动音频流
    if (self->input_enabled_) {
        int result = _start_input_stream(self);
        if (result != AUDIO_SUCCESS) {
            LOG_ERROR("启动输入流失败");
            return result;
        }
    }
    
    if (self->output_enabled_) {
        int result = _start_output_stream(self);
        if (result != AUDIO_SUCCESS) {
            LOG_ERROR("启动输出流失败");
            return result;
        }
    }
    
    LOG_INFO("PortAudio Mac接口启动成功");
    return AUDIO_SUCCESS;
}

static int portaudio_mac_destroy(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // 停止并关闭音频流
    if (data->input_stream) {
        Pa_StopStream(data->input_stream);
        Pa_CloseStream(data->input_stream);
        data->input_stream = NULL;
    }
    
    if (data->output_stream) {
        Pa_StopStream(data->output_stream);
        Pa_CloseStream(data->output_stream);
        data->output_stream = NULL;
    }
    
    // 清理缓冲区
    if (data->record_buffer) {
        free(data->record_buffer);
        data->record_buffer = NULL;
    }
    
    if (data->play_buffer) {
        free(data->play_buffer);
        data->play_buffer = NULL;
    }
    
    // 清理同步对象
    pthread_mutex_destroy(&data->record_mutex);
    pthread_mutex_destroy(&data->play_mutex);
    pthread_cond_destroy(&data->record_cond);
    pthread_cond_destroy(&data->play_cond);
    
    // 释放数据结构
    free(data);
    self->impl_data = NULL;
    
    // 终止PortAudio
    if (self->is_initialized) {
        Pa_Terminate();
    }
    
    LOG_INFO("PortAudio Mac实现销毁成功");
    return AUDIO_SUCCESS;
}

// ============================================================================
// 配置函数实现
// ============================================================================

static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate, 
                                     int frame_size, int channels, int periods, 
                                     int buffer_size, int period_size) {
    if (!self || !self->impl_data) {
        LOG_ERROR("无效的音频接口");
        return;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // 存储配置到AudioInterface结构体中
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    // 设置输入参数
    data->input_params.device = Pa_GetDefaultInputDevice();
    if (data->input_params.device == paNoDevice) {
        LOG_ERROR("未找到默认输入设备");
        return;
    }
    
    const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(data->input_params.device);
    if (!inputDeviceInfo) {
        LOG_ERROR("获取输入设备信息失败");
        return;
    }
    
    // 使用请求的通道数和设备最大通道数中的较小值
    int inputChannels = (channels <= inputDeviceInfo->maxInputChannels) ? channels : inputDeviceInfo->maxInputChannels;
    
    data->input_params.channelCount = inputChannels;
    data->input_params.sampleFormat = paInt16;
    data->input_params.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
    data->input_params.hostApiSpecificStreamInfo = NULL;
    
    LOG_INFO("输入设备: %s, 通道数: %d (请求: %d, 最大: %d)", 
             inputDeviceInfo->name, inputChannels, channels, inputDeviceInfo->maxInputChannels);
    
    // 设置输出参数
    data->output_params.device = Pa_GetDefaultOutputDevice();
    if (data->output_params.device == paNoDevice) {
        LOG_ERROR("未找到默认输出设备");
        return;
    }
    
    const PaDeviceInfo* outputDeviceInfo = Pa_GetDeviceInfo(data->output_params.device);
    if (!outputDeviceInfo) {
        LOG_ERROR("获取输出设备信息失败");
        return;
    }
    
    // 使用请求的通道数和设备最大通道数中的较小值
    int outputChannels = (channels <= outputDeviceInfo->maxOutputChannels) ? channels : outputDeviceInfo->maxOutputChannels;
    
    data->output_params.channelCount = outputChannels;
    data->output_params.sampleFormat = paInt16;
    data->output_params.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
    data->output_params.hostApiSpecificStreamInfo = NULL;
    
    LOG_INFO("输出设备: %s, 通道数: %d (请求: %d, 最大: %d)", 
             outputDeviceInfo->name, outputChannels, channels, outputDeviceInfo->maxOutputChannels);
    
    // 分配环形缓冲区
    data->record_buffer_size = buffer_size * channels;
    data->play_buffer_size = buffer_size * channels;
    
    data->record_buffer = (int16_t*)malloc(data->record_buffer_size * sizeof(int16_t));
    data->play_buffer = (int16_t*)malloc(data->play_buffer_size * sizeof(int16_t));
    
    if (!data->record_buffer || !data->play_buffer) {
        LOG_ERROR("分配音频缓冲区失败");
        return;
    }
    
    memset(data->record_buffer, 0, data->record_buffer_size * sizeof(int16_t));
    memset(data->play_buffer, 0, data->play_buffer_size * sizeof(int16_t));
    
    LOG_INFO("音频配置设置: %u Hz, %d 通道, %d 帧大小", 
             sample_rate, channels, frame_size);
}

// ============================================================================
// 音量控制函数实现
// ============================================================================

static int portaudio_mac_set_output_volume(AudioInterface* self, int volume) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    // 限制音量范围
    if (volume < AUDIO_VOLUME_MIN) volume = AUDIO_VOLUME_MIN;
    if (volume > AUDIO_VOLUME_MAX) volume = AUDIO_VOLUME_MAX;
    
    self->output_volume_ = volume;
    LOG_INFO("设置输出音量为 %d", volume);
    
    // 注意：PortAudio本身不直接支持音量控制，这里只是存储值
    // 实际的音量控制需要在音频数据处理时应用
    
    return AUDIO_SUCCESS;
}

// ============================================================================
// 输入输出管理函数实现
// ============================================================================

static int portaudio_mac_enable_input(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    if (enable == self->input_enabled_) {
        return AUDIO_SUCCESS; // 状态没有变化
    }
    
    self->input_enabled_ = enable;
    LOG_INFO("设置输入启用状态为 %s", enable ? "true" : "false");
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    if (!data) {
        return AUDIO_SUCCESS;
    }
    
    // 如果正在录制且要禁用输入，停止录制
    if (!enable && data->input_stream_active && data->input_stream) {
        Pa_StopStream(data->input_stream);
        Pa_CloseStream(data->input_stream);
        data->input_stream = NULL;
        data->input_stream_active = false;
        LOG_INFO("由于输入禁用而停止录制");
    }
    
    return AUDIO_SUCCESS;
}

static int portaudio_mac_enable_output(AudioInterface* self, bool enable) {
    if (!self) {
        LOG_ERROR("无效的音频接口");
        return AUDIO_ERROR_INVALID;
    }
    
    if (enable == self->output_enabled_) {
        return AUDIO_SUCCESS; // 状态没有变化
    }
    
    self->output_enabled_ = enable;
    LOG_INFO("设置输出启用状态为 %s", enable ? "true" : "false");
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    if (!data) {
        return AUDIO_SUCCESS;
    }
    
    // 如果正在播放且要禁用输出，停止播放
    if (!enable && data->output_stream_active && data->output_stream) {
        Pa_StopStream(data->output_stream);
        Pa_CloseStream(data->output_stream);
        data->output_stream = NULL;
        data->output_stream_active = false;
        LOG_INFO("由于输出禁用而停止播放");
    }
    
    return AUDIO_SUCCESS;
}

// ============================================================================
// 高级数据处理函数实现（对齐AudioCodec::OutputData和InputData）
// ============================================================================

static int portaudio_mac_output_data(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("输出数据参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    // 检查输出是否启用
    if (!self->output_enabled_) {
        LOG_WARN("输出已禁用，忽略输出数据调用");
        return AUDIO_ERROR_DEVICE;
    }
    
    // 应用音量控制
    if (self->output_volume_ != 100) {
        // 创建临时缓冲区应用音量
        int16_t* temp_buffer = (int16_t*)malloc(samples * sizeof(int16_t));
        if (!temp_buffer) {
            LOG_ERROR("分配音量控制临时缓冲区失败");
            return portaudio_mac_write(self, data, samples);
        }
        
        float volume_factor = self->output_volume_ / 100.0f;
        for (size_t i = 0; i < samples; i++) {
            temp_buffer[i] = (int16_t)(data[i] * volume_factor);
        }
        
        int result = portaudio_mac_write(self, temp_buffer, samples);
        free(temp_buffer);
        return result;
    } else {
        // 直接写入，无需音量调整
        return portaudio_mac_write(self, data, samples);
    }
}

static int portaudio_mac_input_data(AudioInterface* self, int16_t* data, size_t samples) {
    if (!self || !data || samples == 0) {
        LOG_ERROR("输入数据参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    // 检查输入是否启用
    if (!self->input_enabled_) {
        LOG_WARN("输入已禁用，忽略输入数据调用");
        return AUDIO_ERROR_DEVICE;
    }
    
    return portaudio_mac_read(self, data, samples);
}

// ============================================================================
// 底层读写函数实现（对齐AudioCodec::Read和Write）
// ============================================================================

static int portaudio_mac_read(AudioInterface* self, int16_t* dest, size_t samples) {
    if (!self || !self->impl_data || !dest) {
        LOG_ERROR("读取参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    // 检查输入是否启用
    if (!self->input_enabled_) {
        LOG_WARN("音频输入未启用");
        return AUDIO_ERROR_DEVICE;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // 直接使用PortAudio的读取函数
    PaError err = Pa_ReadStream(data->input_stream, dest, samples);
    if (err != paNoError) {
        if (err == paTimedOut) {
            LOG_WARN("音频输入流读取超时");
            return AUDIO_ERROR_TIMEOUT;
        } else {
            LOG_ERROR("音频输入流读取错误: %s", Pa_GetErrorText(err));
            return AUDIO_ERROR_DEVICE;
        }
    }
    
    return (int)samples;
}

static int portaudio_mac_write(AudioInterface* self, const int16_t* data, size_t samples) {
    if (!self || !self->impl_data || !data) {
        LOG_ERROR("写入参数无效");
        return AUDIO_ERROR_INVALID;
    }
    
    // 检查输出是否启用
    if (!self->output_enabled_) {
        LOG_WARN("音频输出未启用");
        return AUDIO_ERROR_DEVICE;
    }
    
    PortAudioMacData* pa_data = (PortAudioMacData*)self->impl_data;
    
    // 直接使用PortAudio的写入函数
    PaError err = Pa_WriteStream(pa_data->output_stream, data, samples);
    if (err != paNoError) {
        LOG_ERROR("音频输出流写入错误: %s", Pa_GetErrorText(err));
        return AUDIO_ERROR_DEVICE;      
    }
    
    return AUDIO_SUCCESS;
}

// ============================================================================
// 内部辅助函数
// ============================================================================

static int _start_input_stream(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return AUDIO_ERROR_INVALID;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (data->input_stream_active) {
        return AUDIO_SUCCESS; // 已经激活
    }
    
    // 验证配置是否已设置
    if (self->sample_rate == 0 || self->frame_size == 0 || self->channels == 0) {
        LOG_ERROR("音频配置未设置。请先调用set_config。");
        return AUDIO_ERROR_NOT_INIT;
    }
    
    // 验证输入设备
    if (data->input_params.device == paNoDevice) {
        LOG_ERROR("未配置输入设备");
        return AUDIO_ERROR_DEVICE;
    }
    
    // 检查设备是否仍然有效
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(data->input_params.device);
    if (!deviceInfo) {
        LOG_ERROR("输入设备不再可用");
        return AUDIO_ERROR_DEVICE;
    }
    
    LOG_INFO("打开输入流: 设备=%s, 采样率=%u, 通道=%d, 帧大小=%d", 
             deviceInfo->name, self->sample_rate, self->channels, self->frame_size);
    
    // 使用阻塞模式（不使用回调函数）
    PaError err = Pa_OpenStream(&data->input_stream,
                               &data->input_params,
                               NULL, // 无输出
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               NULL, // 不使用回调函数
                               NULL); // 不使用用户数据
    
    if (err != paNoError) {
        LOG_ERROR("打开输入流失败: %s", Pa_GetErrorText(err));
        return AUDIO_ERROR_DEVICE;
    }
    
    err = Pa_StartStream(data->input_stream);
    if (err != paNoError) {
        LOG_ERROR("启动输入流失败: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->input_stream);
        data->input_stream = NULL;
        return AUDIO_ERROR_DEVICE;
    }
    
    data->input_stream_active = true;
    LOG_INFO("录制启动成功");
    return AUDIO_SUCCESS;
}

static int _start_output_stream(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return AUDIO_ERROR_INVALID;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (data->output_stream_active) {
        return AUDIO_SUCCESS; // 已经激活
    }
    
    // 验证配置是否已设置
    if (self->sample_rate == 0 || self->frame_size == 0 || self->channels == 0) {
        LOG_ERROR("音频配置未设置。请先调用set_config。");
        return AUDIO_ERROR_NOT_INIT;
    }
    
    // 验证输出设备
    if (data->output_params.device == paNoDevice) {
        LOG_ERROR("未配置输出设备");
        return AUDIO_ERROR_DEVICE;
    }
    
    // 检查设备是否仍然有效
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(data->output_params.device);
    if (!deviceInfo) {
        LOG_ERROR("输出设备不再可用");
        return AUDIO_ERROR_DEVICE;
    }
    
    LOG_INFO("打开输出流: 设备=%s, 采样率=%u, 通道=%d, 帧大小=%d", 
             deviceInfo->name, self->sample_rate, self->channels, self->frame_size);
    
    // 使用阻塞模式（不使用回调函数）
    PaError err = Pa_OpenStream(&data->output_stream,
                               NULL, // 无输入
                               &data->output_params,
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               NULL, // 不使用回调函数
                               NULL); // 不使用用户数据
    
    if (err != paNoError) {
        LOG_ERROR("打开输出流失败: %s", Pa_GetErrorText(err));
        return AUDIO_ERROR_DEVICE;
    }
    
    err = Pa_StartStream(data->output_stream);
    if (err != paNoError) {
        LOG_ERROR("启动输出流失败: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->output_stream);
        data->output_stream = NULL;
        return AUDIO_ERROR_DEVICE;
    }
    
    data->output_stream_active = true;
    LOG_INFO("播放启动成功");
    return AUDIO_SUCCESS;
}

// ============================================================================
// PortAudio回调函数实现
// ============================================================================

#else // !PORTAUDIO_AVAILABLE

// PortAudio不可用时的桩实现
AudioInterface* portaudio_mac_create(void) {
    LOG_ERROR("PortAudio不可用，无法创建PortAudio Mac实现");
    return NULL;
}

#endif // PORTAUDIO_AVAILABLE