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
    linx_audio_data_callback_t audio_callback;
    void* callback_user_data;
    
    // 状态管理
    bool is_running;
    pthread_mutex_t state_mutex;
} coreaudio_private_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static linx_audio_result_t coreaudio_initialize(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_deinitialize(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_start(linx_audio_driver_t* driver);
static linx_audio_result_t coreaudio_stop(linx_audio_driver_t* driver);

static linx_audio_result_t coreaudio_enumerate_devices(linx_audio_driver_t* driver,
                                                       linx_audio_device_info_t** devices,
                                                       uint32_t* device_count);
static linx_audio_result_t coreaudio_get_device_info(linx_audio_driver_t* driver,
                                                     uint32_t device_id,
                                                     linx_audio_device_info_t* info);
static linx_audio_result_t coreaudio_get_latency(linx_audio_driver_t* driver,
                                                  linx_audio_device_t* device,
                                                  uint32_t* latency_frames);

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


// ============================================================================
// 虚函数表
// ============================================================================

static const linx_audio_driver_vtable_t coreaudio_vtable = {
    .initialize = coreaudio_initialize,
    .deinitialize = coreaudio_deinitialize,
    .start = coreaudio_start,
    .stop = coreaudio_stop,
    .enumerate_devices = coreaudio_enumerate_devices,
    .get_device_info = coreaudio_get_device_info,
    .open_device = NULL,  // TODO: implement
    .close_device = NULL,  // TODO: implement
    .start_device = NULL,  // TODO: implement
    .stop_device = NULL,  // TODO: implement
    .pause_device = NULL,  // TODO: implement
    .resume_device = NULL,  // TODO: implement
    .read_data = NULL,  // TODO: implement
    .write_data = NULL,  // TODO: implement
    .set_device_config = NULL,  // TODO: implement
    .get_device_config = NULL,  // TODO: implement
    .set_volume = NULL,  // TODO: implement
    .get_volume = NULL,  // TODO: implement
    .set_mute = NULL,  // TODO: implement
    .get_mute = NULL,  // TODO: implement
    .get_device_state = NULL,  // TODO: implement
    .get_device_stats = NULL,  // TODO: implement
    .reset_device_stats = NULL,  // TODO: implement
    .get_latency = coreaudio_get_latency,
    .set_event_callback = NULL,  // TODO: implement
    .suspend = NULL,  // TODO: implement
    .resume = NULL,  // TODO: implement
    .destroy = NULL  // TODO: implement
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
    
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_deinitialize(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
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
    
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_start(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
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
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        
        // 启动输出AudioUnit
        status = AudioUnitInitialize(priv->output_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        
        status = AudioOutputUnitStart(priv->output_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
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
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        
        // 启动输入AudioUnit
        status = AudioUnitInitialize(priv->input_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
        
        status = AudioOutputUnitStart(priv->input_unit);
        if (status != noErr) {
            pthread_mutex_unlock(&priv->state_mutex);
            return LINX_AUDIO_ERROR_IO_ERROR;
        }
    }
    
    priv->is_running = true;
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t coreaudio_stop(linx_audio_driver_t* driver) {
    if (!driver || !driver->private_data) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    
    pthread_mutex_lock(&priv->state_mutex);
    
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
    
    pthread_mutex_unlock(&priv->state_mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 设备枚举和信息查询
// ============================================================================

// Fix function signature for enumerate_devices - change size_t* to uint32_t*
static linx_audio_result_t coreaudio_enumerate_devices(linx_audio_driver_t* driver,
                                                       linx_audio_device_info_t** devices,
                                                       uint32_t* device_count) {
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
        return LINX_AUDIO_ERROR_IO_ERROR;
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
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    // 分配设备信息数组
    linx_audio_device_info_t* device_infos = malloc(device_count_ca * sizeof(linx_audio_device_info_t));
    if (!device_infos) {
        free(device_ids);
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    // 填充设备信息
    uint32_t valid_device_count = 0;
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



static linx_audio_result_t coreaudio_get_device_info(linx_audio_driver_t* driver,
                                                     uint32_t device_id,
                                                     linx_audio_device_info_t* info) {
    if (!driver || !info) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    AudioDeviceID ca_device_id = (AudioDeviceID)device_id;
    memset(info, 0, sizeof(linx_audio_device_info_t));
    
    info->device_id = device_id;  // Use 'device_id' instead of 'id'
    
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
    
    // 获取制造商 - use 'description' field for manufacturer
    CFStringRef manufacturer = NULL;
    size = sizeof(manufacturer);
    result = get_device_property_data(ca_device_id,
                                     kAudioDevicePropertyDeviceManufacturerCFString,
                                     kAudioObjectPropertyScopeGlobal,
                                     &manufacturer, &size);
    if (result == LINX_AUDIO_SUCCESS && manufacturer) {
        // Store manufacturer info in description field since it exists in the struct
        char manufacturer_str[256];
        CFStringGetCString(manufacturer, manufacturer_str, sizeof(manufacturer_str), kCFStringEncodingUTF8);
        snprintf(info->name + strlen(info->name), sizeof(info->name) - strlen(info->name), " (%s)", manufacturer_str);
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
    
    // Set device state
    info->state = LINX_AUDIO_DEVICE_STATE_IDLE;
    
    // 设置基本属性 - use correct struct members
    info->default_params.channels = input_channels > output_channels ? input_channels : output_channels;
    info->default_params.sample_rate = 44100;
    info->default_params.format = LINX_AUDIO_FORMAT_FLOAT32;
    info->default_params.buffer_size = 512;
    
    // 设置格式信息 - format是枚举类型，不是结构体
    info->format = LINX_AUDIO_FORMAT_FLOAT32;
    
    // 获取支持的采样率范围
    info->min_sample_rate = 44100;
    info->max_sample_rate = 96000;
    
    // Set default device flag
    info->is_default = false; // Would need to check against system default
    
    // Set driver data to NULL for now
    info->driver_data = NULL;
    
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
    
    // 创建LinxOS音频缓冲区 - fix struct member names
    linx_audio_buffer_t output_buffer;
    output_buffer.data = ioData->mBuffers[0].mData;
    output_buffer.size = ioData->mBuffers[0].mDataByteSize;
    output_buffer.frames = inNumberFrames;  // Use 'frames' instead of 'frame_count'
    output_buffer.used = ioData->mBuffers[0].mDataByteSize;
    
    // Set default parameters
    output_buffer.params.sample_rate = 44100;
    output_buffer.params.channels = ioData->mBuffers[0].mNumberChannels;
    output_buffer.params.format = LINX_AUDIO_FORMAT_FLOAT32;
    output_buffer.params.buffer_size = inNumberFrames;
    
    output_buffer.timestamp = 0;
    output_buffer.is_readonly = false;
    
    // 调用用户回调 - fix callback signature
    linx_audio_result_t result = priv->audio_callback(NULL, &output_buffer, 
                                                     priv->callback_user_data);
    
    // 更新统计信息
    driver->stats.callback_count++;
    driver->stats.total_data_transferred += inNumberFrames * sizeof(float) * 2; // Assuming stereo
    
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
    
    // 创建LinxOS音频缓冲区 - fix struct member names
    linx_audio_buffer_t input_buffer;
    input_buffer.data = priv->input_buffer_list->mBuffers[0].mData;
    input_buffer.size = priv->input_buffer_list->mBuffers[0].mDataByteSize;
    input_buffer.frames = inNumberFrames;  // Use 'frames' instead of 'frame_count'
    input_buffer.used = priv->input_buffer_list->mBuffers[0].mDataByteSize;
    
    // Set default parameters
    input_buffer.params.sample_rate = 44100;
    input_buffer.params.channels = priv->input_buffer_list->mBuffers[0].mNumberChannels;
    input_buffer.params.format = LINX_AUDIO_FORMAT_FLOAT32;
    input_buffer.params.buffer_size = inNumberFrames;
    
    input_buffer.timestamp = 0;
    input_buffer.is_readonly = true;
    
    // 调用用户回调 - fix callback signature
    priv->audio_callback(NULL, &input_buffer, priv->callback_user_data);
    
    return noErr;
}

// ============================================================================
// 其他接口实现
// ============================================================================







// Fix function signature for get_latency - change to match audio_driver.h
static linx_audio_result_t coreaudio_get_latency(linx_audio_driver_t* driver,
                                                  linx_audio_device_t* device,
                                                  uint32_t* latency_frames) {
    if (!driver || !device || !latency_frames) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    coreaudio_private_t* priv = (coreaudio_private_t*)driver->private_data;
    if (!priv) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // For now, return a default latency value
    // In a real implementation, this would query the actual device latency
    *latency_frames = 512; // Default buffer size
    
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
        return LINX_AUDIO_ERROR_IO_ERROR;
    }
    
    OSStatus status = AudioComponentInstanceNew(component, unit);
    if (status != noErr) {
        return LINX_AUDIO_ERROR_IO_ERROR;
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
        return LINX_AUDIO_ERROR_IO_ERROR;
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
        return LINX_AUDIO_ERROR_IO_ERROR;
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
    
    return (status == noErr) ? LINX_AUDIO_SUCCESS : LINX_AUDIO_ERROR_IO_ERROR;
}

#endif // LINX_PLATFORM_MACOS