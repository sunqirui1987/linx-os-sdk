#include "audio_processor.h"
#include "../common/log/linx_log.h"
#include <string.h>

/**
 * @file audio_processor.c
 * @brief 音频处理器公共接口实现
 * @details 提供音频处理器的统一接口实现，通过虚函数表调用具体实现
 */

#define AUDIO_PROCESSOR_TAG "AudioProcessor"

// 公共API函数实现

audio_processor_error_t audio_processor_initialize(AudioProcessor* processor,
                                                   const audio_processor_config_t* config,
                                                   audio_codec_t* codec) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器虚函数表为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable->initialize) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "初始化函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!config) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "配置参数为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "初始化音频处理器，采样率: %d, 声道数: %d", 
              config->sample_rate, config->channels);
    
    return processor->vtable->initialize(processor, config, codec);
}

audio_processor_error_t audio_processor_start(AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->start) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "启动函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "启动音频处理器");
    return processor->vtable->start(processor);
}

audio_processor_error_t audio_processor_stop(AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->stop) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "停止函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "停止音频处理器");
    return processor->vtable->stop(processor);
}

audio_processor_error_t audio_processor_feed(AudioProcessor* processor, 
                                            const int16_t* data, 
                                            size_t size) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->feed) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "数据输入函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!data || size == 0) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "输入数据无效");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    return processor->vtable->feed(processor, data, size);
}

size_t audio_processor_get_feed_size(const AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return 0;
    }
    
    if (!processor->vtable || !processor->vtable->get_feed_size) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "获取输入大小函数未实现");
        return 0;
    }
    
    return processor->vtable->get_feed_size(processor);
}

audio_processor_error_t audio_processor_enable_device_aec(AudioProcessor* processor, bool enable) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->enable_device_aec) {
        LINX_LOGW(AUDIO_PROCESSOR_TAG, "设备AEC控制函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "%s设备AEC", enable ? "启用" : "禁用");
    return processor->vtable->enable_device_aec(processor, enable);
}

audio_processor_error_t audio_processor_set_output_callback(AudioProcessor* processor,
                                                           audio_processor_output_callback_t callback,
                                                           void* user_data) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->set_output_callback) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "设置输出回调函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "设置音频输出回调函数");
    return processor->vtable->set_output_callback(processor, callback, user_data);
}

audio_processor_error_t audio_processor_set_vad_callback(AudioProcessor* processor,
                                                        audio_processor_vad_callback_t callback,
                                                        void* user_data) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->set_vad_callback) {
        LINX_LOGW(AUDIO_PROCESSOR_TAG, "设置VAD回调函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "设置VAD状态回调函数");
    return processor->vtable->set_vad_callback(processor, callback, user_data);
}

bool audio_processor_get_vad_status(const AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return false;
    }
    
    if (!processor->vtable || !processor->vtable->get_vad_status) {
        LINX_LOGW(AUDIO_PROCESSOR_TAG, "获取VAD状态函数未实现");
        return false;
    }
    
    return processor->vtable->get_vad_status(processor);
}

audio_processor_error_t audio_processor_reset(AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    if (!processor->vtable || !processor->vtable->reset) {
        LINX_LOGW(AUDIO_PROCESSOR_TAG, "重置函数未实现");
        return AUDIO_PROCESSOR_ERROR_INVALID_PARAM;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "重置音频处理器状态");
    return processor->vtable->reset(processor);
}

int audio_processor_get_delay_ms(const AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return 0;
    }
    
    if (!processor->vtable || !processor->vtable->get_delay_ms) {
        LINX_LOGW(AUDIO_PROCESSOR_TAG, "获取延迟函数未实现");
        return 0;
    }
    
    return processor->vtable->get_delay_ms(processor);
}

void audio_processor_destroy(AudioProcessor* processor) {
    if (!processor) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "音频处理器指针为空");
        return;
    }
    
    if (!processor->vtable || !processor->vtable->destroy) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "销毁函数未实现");
        return;
    }
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "销毁音频处理器");
    processor->vtable->destroy(processor);
}

const char* audio_processor_error_to_string(audio_processor_error_t error) {
    switch (error) {
        case AUDIO_PROCESSOR_SUCCESS:
            return "成功";
        case AUDIO_PROCESSOR_ERROR_INVALID_PARAM:
            return "无效参数";
        case AUDIO_PROCESSOR_ERROR_NOT_INITIALIZED:
            return "未初始化";
        case AUDIO_PROCESSOR_ERROR_ALREADY_STARTED:
            return "已经启动";
        case AUDIO_PROCESSOR_ERROR_NOT_STARTED:
            return "未启动";
        case AUDIO_PROCESSOR_ERROR_MEMORY_ALLOC:
            return "内存分配失败";
        case AUDIO_PROCESSOR_ERROR_CODEC_INIT:
            return "编解码器初始化失败";
        case AUDIO_PROCESSOR_ERROR_PROCESSING:
            return "处理失败";
        case AUDIO_PROCESSOR_ERROR_UNKNOWN:
        default:
            return "未知错误";
    }
}

void audio_processor_config_init_default(audio_processor_config_t* config,
                                        int sample_rate,
                                        int channels,
                                        int frame_duration_ms) {
    if (!config) {
        LINX_LOGE(AUDIO_PROCESSOR_TAG, "配置参数指针为空");
        return;
    }
    
    memset(config, 0, sizeof(audio_processor_config_t));
    
    config->sample_rate = sample_rate;
    config->channels = channels;
    config->frame_duration_ms = frame_duration_ms;
    config->frame_size = (sample_rate * frame_duration_ms) / 1000;
    config->enable_aec = true;
    config->enable_ns = true;
    config->enable_vad = true;
    config->vad_threshold = 0.5f;
    config->models_list = NULL;
    
    LINX_LOGI(AUDIO_PROCESSOR_TAG, "初始化默认配置: 采样率=%d, 声道数=%d, 帧时长=%dms, 帧大小=%d",
              sample_rate, channels, frame_duration_ms, config->frame_size);
}