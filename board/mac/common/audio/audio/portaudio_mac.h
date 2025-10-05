#ifndef PORTAUDIO_MAC_H
#define PORTAUDIO_MAC_H

#include "audio/audio_interface.h"
#define PORTAUDIO_AVAILABLE 1
#ifdef __APPLE__
    // Try to include PortAudio from different possible locations
    #if __has_include(<portaudio.h>)
        #include <portaudio.h>
        #define PORTAUDIO_AVAILABLE 1
    #elif __has_include(<portaudio/portaudio.h>)
        #include <portaudio/portaudio.h>
        #define PORTAUDIO_AVAILABLE 1
    #elif __has_include("portaudio.h")
        #include "portaudio.h"
        #define PORTAUDIO_AVAILABLE 1
    #else
        #warning "PortAudio header not found. PortAudio Mac implementation will be disabled."
        #define PORTAUDIO_AVAILABLE 0
        // Define minimal PortAudio types for compilation
        typedef void PaStream;
        typedef int PaError;
        typedef struct PaStreamParameters {
            int device;
            int channelCount;
            unsigned long sampleFormat;
            double suggestedLatency;
            void *hostApiSpecificStreamInfo;
        } PaStreamParameters;
        typedef struct PaStreamCallbackTimeInfo PaStreamCallbackTimeInfo;
        typedef unsigned long PaStreamCallbackFlags;
        typedef int (*PaStreamCallback)(const void *input, void *output,
                                       unsigned long frameCount,
                                       const PaStreamCallbackTimeInfo* timeInfo,
                                       PaStreamCallbackFlags statusFlags,
                                       void *userData);
        // Define minimal constants
        #define paNoError 0
        #define paNoDevice -1
        #define paInt16 0x00000008
        #define paClipOff 0x00000001
        #define paContinue 0
        #define paTimedOut -1
        #define paOutputUnderflowed -2
        #define paOutputOverflowed -3
    #endif
#endif

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 创建PortAudio Mac实现
 * 
 * 此函数创建一个基于PortAudio的音频接口实现，专门用于macOS平台。
 * 该实现提供：
 * - 完整的音频输入输出功能
 * - 音量控制
 * - 全双工音频处理
 * - 低延迟音频流
 * 
 * @return AudioInterface实例，失败时返回NULL
 */
AudioInterface* portaudio_mac_create(void);

#ifdef __cplusplus
}
#endif

#endif // PORTAUDIO_MAC_H