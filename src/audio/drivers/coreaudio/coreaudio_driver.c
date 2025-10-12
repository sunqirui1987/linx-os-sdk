/**
 * @file coreaudio_driver.c
 * @brief CoreAudio驱动实现（macOS）
 * @details 基于CoreAudio框架的音频驱动实现
 */

#ifdef LINX_PLATFORM_MACOS

#include "audio_driver.h"
#include "../core/types.h"
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief CoreAudio设备信息
 */
typedef struct {
    AudioDeviceID device_id;
    AudioObjectPropertyScope scope;
    bool is_input;
    bool is_output;
} coreaudio_device_info_t;

/**
 * @brief CoreAudio驱动私有数据
 */
typedef struct {
    // 设备管理
    coreaudio_device_info_t* devices;
    uint32_t device_count;
    uint32_t device_capacity;
    
    // 当前活跃设备
    AudioDeviceID current_input_device;
    AudioDeviceID current_output_device;
    
    // AudioUnit
    AudioUnit input_unit;
    AudioUnit output_unit;
    
    // 音频格式
    AudioStreamBasicDescription input_format;
    AudioStreamBasicDescription output_format;
    
    // 缓冲区管理
    AudioBufferList* input_buffer_list;
    AudioBufferList* output_buffer_list;
    
    // 回调数据
    linx_audio_callback_t audio_callback;
    void* callback_user_data;
    
    // 状态管理
    bool is_running;
    pthread_mutex_t state_mutex;
    
    // 统计信息
    uint64_t frames_processed;
    uint64_t callback_count;
    uint64_t total_callback_time_us;
} coreaudio_private_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t coreaudio_initialize(linx_audio_driver_t* driver);
static void coreaudio_deinitialize(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_start(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_stop(linx_audio_driver_t* driver);

static linx_audio_result_t coreaudio_enumerate_devices(linx_audio_driver_t* driver,
                                                       linx_audio_device_info_t** devices,
                                                       size_t* device_count);
static void coreaudio_free_device_list(linx_audio_driver_t* driver,
                                       linx_audio_device_info_t* devices,
                                       size_t device_count);
static linx_audio_result_t coreaudio_get_device_info(linx_audio_driver_t* driver,
                                                     uint32_t device_id,
                                                     linx_audio_device_info_t* info);

static bool coreaudio_is_format_supported(linx_audio_driver_t* driver,
                                          uint32_t device_id,
                                          const linx_audio_format_info_t* format);

static linx_audio_driver_state_t coreaudio_get_state(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_get_stats(linx_audio_driver_t* driver,
                                               linx_audio_driver_stats_t* stats);
static linx_audio_result_t coreaudio_get_latency(linx_audio_driver_t* driver,
                                                 uint32_t* input_latency,
                                                 uint32_t* output_latency);

static linx_audio_result_t coreaudio_set_callback(linx_audio_driver_t* driver,
                                                  linx_audio_callback_t callback,
                                                  void* user_data);
static linx_audio_result_t coreaudio_update_config(linx_audio_driver_t* driver,
                                                   const linx_audio_driver_config_t* config);

// CoreAudio回调函数
static OSStatus coreaudio_input_callback(void* inRefCon,
                                        AudioUnitRenderActionFlags* ioActionFlags,
                                        const AudioTimeStamp* inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList* ioData);

static OSStatus coreaudio_output_callback(void* inRefCon,
                                         AudioUnitRenderActionFlags* ioActionFlags,
                                         const AudioTimeStamp* inTimeStamp,
                                         UInt32 inBusNumber,
                                         UInt32 inNumberFrames,
                                         AudioBufferList* ioData);

// 辅助函数
static linx_audio_result_t setup_audio_unit(AudioUnit* unit, bool is_input, 
                                            AudioDeviceID device_id,
                                            const AudioStreamBasicDescription* format);
static linx_audio_result_t get_device_property_data(AudioDeviceID device_id,
                                                    AudioObjectPropertySelector selector,
                                                    AudioObjectPropertyScope scope,
                                                    void* data, UInt32* size);
static linx_audio_result_t convert_ca_format_to_linx(const AudioStreamBasicDescription* ca_format,
                                                     linx_audio_format_info_t* linx_format);
static linx_audio_result_t convert_linx_format_to_ca(const linx_audio_format_info_t* linx_format,
                                                     AudioStreamBasicDescription* ca_format);

// ============================================================================
// 虚函数表
// ============================================================================

static const linx_audio_driver_vtable_t coreaudio_vtable = {
    .initialize = coreaudio_initialize,
    .deinitialize = coreaudio_deinitialize,
    .start = coreaudio_start,
    .stop = coreaudio_stop,
    .enumerate_devices = coreaudio_enumerate_devices,
    .free_device_list = coreaudio_free_device_list,
    .get_device_info = coreaudio_get_device_info,
    .is_format_supported = coreaudio_is_format_supported,
    .get_state = coreaudio_get_state,
    .get_stats = coreaudio_get_stats,
    .get_latency = coreaudio_get_latency,
    .set_callback = coreaudio_set_callback,
    .update_config = coreaudio_update_config
};

// ============================================================================
// 公共接口实现
// ============================================================================

linx_audio_driver_t* linx_coreaudio_driver_create(void) {
    linx_audio_driver_t* driver = malloc(sizeof(linx_audio_driver_t));
    if (!driver) {
        return NULL;
    }
    
    memset(driver, 0, sizeof(linx_audio_driver_t));
    
    // 分配私有数据
    coreaudio_private_t* priv = malloc(sizeof(coreaudio_private_t));
    if (!priv) {
        free(driver);
        return NULL;
    }
    
    memset(priv, 0, sizeof(coreaudio_private_t));
    
    // 初始化互斥锁
    if (pthread_mutex_init(&priv->state_mutex, NULL) != 0) {
        free(priv);
        free(driver);
        return NULL;
    }
    
    // 设置驱动属性
    driver->vtable = &coreaudio_vtable;
    driver->type = LINX_AUDIO_DRIVER_TYPE_COREAUDIO;
    driver->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    driver->private_data = priv;
    
    return driver;
}

// ============================================================================
// 驱动生命周期管理
// ============================================================================

static linx_audio_result_t coreaudio_initialize(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
    if (driver->state != LINX_AUDIO_DRIVER_STATE_UNINITIALIZED) {
        pthread_mutex_unlock(&priv->state_mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    // 初始化设备列表
    priv->device_capacity = 16;
    priv->devices = malloc(priv->device_capacity * sizeof(coreaudio_device_info_t));
    if (!priv->devices) {
        pthread_mutex_unlock(&priv->state_mutex);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    // 获取默认设备
    UInt32 size = sizeof(AudioDeviceID);
    AudioObjectPropertyAddress property_address = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    
    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                &property_address,
                                                0, NULL,
                                                &size,
                                                &priv->current_output_device);
    if (status != noErr) {
        priv->current_output_device = kAudioObjectUnknown;
    }
    
    property_address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                       &property_address,
                                       0, NULL,
                                       &size,
                                       &priv->current_input_device);
    if (status != noErr) {
        priv->current_input_device = kAudioObjectUnknown;
    }
    
    driver->state = LINX_AUDIO_DRIVER_STATE_INITIALIZED;
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static void coreaudio_deinitialize(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    // 停止驱动
    coreaudio_stop(driver);
    
    pthread_mutex_lock(&priv->state_mutex);
    
    // 清理AudioUnit
    if (priv->input_unit) {
        AudioUnitUninitialize(priv->input_unit);
        AudioComponentInstanceDispose(priv->input_unit);
        priv->input_unit = NULL;
    }
    
    if (priv->output_unit) {
        AudioUnitUninitialize(priv->output_unit);
        AudioComponentInstanceDispose(priv->output_unit);
        priv->output_unit = NULL;
    }
    
    // 清理缓冲区
    if (priv->input_buffer_list) {
        free(priv->input_buffer_list);
        priv->input_buffer_list = NULL;
    }
    
    if (priv->output_buffer_list) {
        free(priv->output_buffer_list);
        priv->output_buffer_list = NULL;
    }
    
    // 清理设备列表
    if (priv->devices) {
        free(priv->devices);
        priv->devices = NULL;
    }
    
    driver->state = LINX_AUDIO_DRIVER_STATE_UNINITIALIZED;
    pthread_mutex_unlock(&priv->state_mutex);
}

static linx_audio_result_t coreaudio_start(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
    if (driver->state != LINX_AUDIO_DRIVER_STATE_INITIALIZED) {
        pthread_mutex_unlock(&priv->state_mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    // 设置默认格式
    priv->output_format.mSampleRate = driver->config.format.sample_rate;
    priv->output_format.mFormatID = kAudioFormatLinearPCM;
    priv->output_format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    priv->output_format.mChannelsPerFrame = driver->config.format.channels;
    priv->output_format.mBitsPerChannel = 32;
    priv->output_format.mBytesPerFrame = priv->output_format.mChannelsPerFrame * sizeof(float);
    priv->output_format.mBytesPerPacket = priv->output_format.mBytesPerFrame;
    priv->output_format.mFramesPerPacket = 1;
    
    priv->input_format = priv->output_format;
    
    // 设置输出AudioUnit
    if (driver->config.enable_output && priv->current_output_device != kAudioObjectUnknown) {
        linx_audio_result_t result = setup_audio_unit(&priv->output_unit, false,
                                                      priv->current_output_device,
                                                      &priv->output_format);
        if (result != LINX_AUDIO_SUCCESS) {
            pthread_mutex_unlock(&priv->state_mutex);
            return result;
        }
        
        // 设置输出回调
        AURenderCallbackStruct callback_struct;
        callback_struct.inputProc = coreaudio_output_callback;
        callback_struct.inputProcRefCon = driver;
        
        OSStatus status = AudioUnitSetProperty(priv->output_unit,
                                              kAudioUnitProperty_SetRenderCallback,
                                              kAudioUnitScope_Input,
                                              0,
                                              &callback_struct,
                                              sizeof(callback_struct));
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
        
        // 启动输出AudioUnit
        status = AudioUnitInitialize(priv->output_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
        
        status = AudioOutputUnitStart(priv->output_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
    }
    
    // 设置输入AudioUnit
    if (driver->config.enable_input && priv->current_input_device != kAudioObjectUnknown) {
        linx_audio_result_t result = setup_audio_unit(&priv->input_unit, true,
                                                      priv->current_input_device,
                                                      &priv->input_format);
        if (result != LINX_AUDIO_SUCCESS) {
            pthread_mutex_unlock(&priv->state_mutex);
            return result;
        }
        
        // 设置输入回调
        AURenderCallbackStruct callback_struct;
        callback_struct.inputProc = coreaudio_input_callback;
        callback_struct.inputProcRefCon = driver;
        
        OSStatus status = AudioUnitSetProperty(priv->input_unit,
                                              kAudioOutputUnitProperty_SetInputCallback,
                                              kAudioUnitScope_Global,
                                              0,
                                              &callback_struct,
                                              sizeof(callback_struct));
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
        
        // 启动输入AudioUnit
        status = AudioUnitInitialize(priv->input_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
        
        status = AudioOutputUnitStart(priv->input_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_DEVICE_ERROR;
        }
    }
    
    priv->is_running = true;
    driver->state = LINX_AUDIO_DRIVER_STATE_STARTED;
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_stop(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
    if (driver->state != LINX_AUDIO_DRIVER_STATE_STARTED) {
        pthread_mutex_unlock(&priv->state_mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    priv->is_running = false;
    
    // 停止AudioUnit
    if (priv->output_unit) {
        AudioOutputUnitStop(priv->output_unit);
        AudioUnitUninitialize(priv->output_unit);
    }
    
    if (priv->input_unit) {
        AudioOutputUnitStop(priv->input_unit);
        AudioUnitUninitialize(priv->input_unit);
    }
    
    driver->state = LINX_AUDIO_DRIVER_STATE_STOPPED;
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 设备枚举和信息查询
// ============================================================================

static linx_audio_result_t coreaudio_enumerate_devices(linx_audio_driver_t* driver,
                                                       linx_audio_device_info_t** devices,
                                                       size_t* device_count) {
    if (!driver || !devices || !device_count) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 获取设备列表
    AudioObjectPropertyAddress property_address = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    
    UInt32 size = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                                    &property_address,
                                                    0, NULL,
                                                    &size);
    if (status != noErr) {
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    UInt32 device_count_ca = size / sizeof(AudioDeviceID);
    AudioDeviceID* device_ids = malloc(size);
    if (!device_ids) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                       &property_address,
                                       0, NULL,
                                       &size,
                                       device_ids);
    if (status != noErr) {
        free(device_ids);
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    // 分配设备信息数组
    linx_audio_device_info_t* device_infos = malloc(device_count_ca * sizeof(linx_audio_device_info_t));
    if (!device_infos) {
        free(device_ids);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    // 填充设备信息
    size_t valid_device_count = 0;
    for (UInt32 i = 0; i < device_count_ca; i++) {
        linx_audio_device_info_t* info = &device_infos[valid_device_count];
        
        if (coreaudio_get_device_info(driver, device_ids[i], info) == LINX_AUDIO_SUCCESS) {
            valid_device_count++;
        }
    }
    
    free(device_ids);
    
    *devices = device_infos;
    *device_count = valid_device_count;
    
    return LINX_AUDIO_SUCCESS;
}

static void coreaudio_free_device_list(linx_audio_driver_t* driver,
                                       linx_audio_device_info_t* devices,
                                       size_t device_count) {
    (void)driver;
    (void)device_count;
    
    if (devices) {
        free(devices);
    }
}

static linx_audio_result_t coreaudio_get_device_info(linx_audio_driver_t* driver,
                                                     uint32_t device_id,
                                                     linx_audio_device_info_t* info) {
    if (!driver || !info) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    AudioDeviceID ca_device_id = (AudioDeviceID)device_id;
    memset(info, 0, sizeof(linx_audio_device_info_t));
    
    info->id = device_id;
    
    // 获取设备名称
    CFStringRef device_name = NULL;
    UInt32 size = sizeof(device_name);
    linx_audio_result_t result = get_device_property_data(ca_device_id,
                                                         kAudioDevicePropertyDeviceNameCFString,
                                                         kAudioObjectPropertyScopeGlobal,
                                                         &device_name, &size);
    if (result == LINX_AUDIO_SUCCESS && device_name) {
        CFStringGetCString(device_name, info->name, sizeof(info->name), kCFStringEncodingUTF8);
        CFRelease(device_name);
    }
    
    // 获取制造商
    CFStringRef manufacturer = NULL;
    size = sizeof(manufacturer);
    result = get_device_property_data(ca_device_id,
                                     kAudioDevicePropertyDeviceManufacturerCFString,
                                     kAudioObjectPropertyScopeGlobal,
                                     &manufacturer, &size);
    if (result == LINX_AUDIO_SUCCESS && manufacturer) {
        CFStringGetCString(manufacturer, info->description, sizeof(info->description), kCFStringEncodingUTF8);
        CFRelease(manufacturer);
    }
    
    // 检查设备类型
    UInt32 input_channels = 0;
    UInt32 output_channels = 0;
    
    size = sizeof(input_channels);
    get_device_property_data(ca_device_id,
                            kAudioDevicePropertyStreamConfiguration,
                            kAudioDevicePropertyScopeInput,
                            &input_channels, &size);
    
    size = sizeof(output_channels);
    get_device_property_data(ca_device_id,
                            kAudioDevicePropertyStreamConfiguration,
                            kAudioDevicePropertyScopeOutput,
                            &output_channels, &size);
    
    if (input_channels > 0 && output_channels > 0) {
        info->type = LINX_AUDIO_DEVICE_TYPE_DUPLEX;
    } else if (input_channels > 0) {
        info->type = LINX_AUDIO_DEVICE_TYPE_INPUT;
    } else if (output_channels > 0) {
        info->type = LINX_AUDIO_DEVICE_TYPE_OUTPUT;
    } else {
        info->type = LINX_AUDIO_DEVICE_TYPE_UNKNOWN;
    }
    
    // 设置基本属性
    info->min_channels = 1;
    info->max_channels = input_channels > output_channels ? input_channels : output_channels;
    info->is_available = true;
    
    // 获取支持的采样率
    info->supported_sample_rates[0] = 44100;
    info->supported_sample_rates[1] = 48000;
    info->supported_sample_rates[2] = 96000;
    info->sample_rate_count = 3;
    
    // 设置缓冲区大小
    info->min_buffer_size = 64;
    info->max_buffer_size = 4096;
    info->preferred_buffer_size = 512;
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 回调函数实现
// ============================================================================

static OSStatus coreaudio_output_callback(void* inRefCon,
                                         AudioUnitRenderActionFlags* ioActionFlags,
                                         const AudioTimeStamp* inTimeStamp,
                                         UInt32 inBusNumber,
                                         UInt32 inNumberFrames,
                                         AudioBufferList* ioData) {
    (void)ioActionFlags;
    (void)inTimeStamp;
    (void)inBusNumber;
    
    linx_audio_driver_t* driver = (linx_audio_driver_t*)inRefCon;
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    if (!priv->is_running || !priv->audio_callback) {
        // 静音输出
        for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
            memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
        }
        return noErr;
    }
    
    // 创建LinxOS音频缓冲区
    linx_audio_buffer_t output_buffer;
    output_buffer.data = ioData->mBuffers[0].mData;
    output_buffer.size = ioData->mBuffers[0].mDataByteSize;
    output_buffer.frame_count = inNumberFrames;
    output_buffer.channels = ioData->mBuffers[0].mNumberChannels;
    
    // 调用用户回调
    linx_audio_result_t result = priv->audio_callback(NULL, &output_buffer, 
                                                     inNumberFrames, 
                                                     priv->callback_user_data);
    
    // 更新统计信息
    priv->frames_processed += inNumberFrames;
    priv->callback_count++;
    
    return (result == LINX_AUDIO_SUCCESS) ? noErr : kAudioUnitErr_CannotDoInCurrentContext;
}

static OSStatus coreaudio_input_callback(void* inRefCon,
                                        AudioUnitRenderActionFlags* ioActionFlags,
                                        const AudioTimeStamp* inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList* ioData) {
    (void)ioData;
    
    linx_audio_driver_t* driver = (linx_audio_driver_t*)inRefCon;
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    if (!priv->is_running || !priv->audio_callback) {
        return noErr;
    }
    
    // 渲染输入数据
    OSStatus status = AudioUnitRender(priv->input_unit,
                                     ioActionFlags,
                                     inTimeStamp,
                                     inBusNumber,
                                     inNumberFrames,
                                     priv->input_buffer_list);
    if (status != noErr) {
        return status;
    }
    
    // 创建LinxOS音频缓冲区
    linx_audio_buffer_t input_buffer;
    input_buffer.data = priv->input_buffer_list->mBuffers[0].mData;
    input_buffer.size = priv->input_buffer_list->mBuffers[0].mDataByteSize;
    input_buffer.frame_count = inNumberFrames;
    input_buffer.channels = priv->input_buffer_list->mBuffers[0].mNumberChannels;
    
    // 调用用户回调
    priv->audio_callback(&input_buffer, NULL, inNumberFrames, priv->callback_user_data);
    
    return noErr;
}

// ============================================================================
// 其他接口实现
// ============================================================================

static bool coreaudio_is_format_supported(linx_audio_driver_t* driver,
                                          uint32_t device_id,
                                          const linx_audio_format_info_t* format) {
    (void)driver;
    (void)device_id;
    
    if (!format) {
        return false;
    }
    
    // 简单的格式检查
    return (format->sample_rate >= 8000 && format->sample_rate <= 192000 &&
            format->channels >= 1 && format->channels <= 8 &&
            format->format == LINX_AUDIO_FORMAT_FLOAT32);
}

static linx_audio_driver_state_t coreaudio_get_state(linx_audio_driver_t* driver) {
    if (!driver) {
        return LINX_AUDIO_DRIVER_STATE_ERROR;
    }
    
    return driver->state;
}

static linx_audio_result_t coreaudio_get_stats(linx_audio_driver_t* driver,
                                               linx_audio_driver_stats_t* stats) {
    if (!driver || !stats || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
    memset(stats, 0, sizeof(linx_audio_driver_stats_t));
    stats->frames_processed = priv->frames_processed;
    stats->callback_count = priv->callback_count;
    stats->total_callback_time_us = priv->total_callback_time_us;
    
    if (priv->callback_count > 0) {
        stats->average_processing_time_us = priv->total_callback_time_us / priv->callback_count;
    }
    
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_get_latency(linx_audio_driver_t* driver,
                                                 uint32_t* input_latency,
                                                 uint32_t* output_latency) {
    if (!driver) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 返回典型的CoreAudio延迟值
    if (input_latency) {
        *input_latency = 512; // 约11.6ms @ 44.1kHz
    }
    if (output_latency) {
        *output_latency = 512; // 约11.6ms @ 44.1kHz
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_set_callback(linx_audio_driver_t* driver,
                                                  linx_audio_callback_t callback,
                                                  void* user_data) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    priv->audio_callback = callback;
    priv->callback_user_data = user_data;
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_update_config(linx_audio_driver_t* driver,
                                                   const linx_audio_driver_config_t* config) {
    if (!driver || !config) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 更新配置
    driver->config = *config;
    
    // 如果驱动正在运行，需要重启以应用新配置
    if (driver->state == LINX_AUDIO_DRIVER_STATE_STARTED) {
        coreaudio_stop(driver);
        return coreaudio_start(driver);
    }
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 辅助函数实现
// ============================================================================

static linx_audio_result_t setup_audio_unit(AudioUnit* unit, bool is_input,
                                            AudioDeviceID device_id,
                                            const AudioStreamBasicDescription* format) {
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = is_input ? kAudioUnitSubType_HALOutput : kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    AudioComponent component = AudioComponentFindNext(NULL, &desc);
    if (!component) {
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    OSStatus status = AudioComponentInstanceNew(component, unit);
    if (status != noErr) {
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    // 设置设备
    status = AudioUnitSetProperty(*unit,
                                 kAudioOutputUnitProperty_CurrentDevice,
                                 kAudioUnitScope_Global,
                                 0,
                                 &device_id,
                                 sizeof(device_id));
    if (status != noErr) {
        AudioComponentInstanceDispose(*unit);
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    // 设置格式
    AudioUnitScope scope = is_input ? kAudioUnitScope_Output : kAudioUnitScope_Input;
    status = AudioUnitSetProperty(*unit,
                                 kAudioUnitProperty_StreamFormat,
                                 scope,
                                 is_input ? 1 : 0,
                                 format,
                                 sizeof(*format));
    if (status != noErr) {
        AudioComponentInstanceDispose(*unit);
        return LINX_AUDIO_ERROR_DEVICE_ERROR;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t get_device_property_data(AudioDeviceID device_id,
                                                    AudioObjectPropertySelector selector,
                                                    AudioObjectPropertyScope scope,
                                                    void* data, UInt32* size) {
    AudioObjectPropertyAddress property_address = {
        selector,
        scope,
        kAudioObjectPropertyElementMain
    };
    
    OSStatus status = AudioObjectGetPropertyData(device_id,
                                                &property_address,
                                                0, NULL,
                                                size,
                                                data);
    
    return (status == noErr) ? LINX_AUDIO_SUCCESS : LINX_AUDIO_ERROR_DEVICE_ERROR;
}

#endif // LINX_PLATFORM_MACOS