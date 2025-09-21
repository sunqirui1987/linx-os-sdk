#ifndef PORTAUDIO_MAC_H
#define PORTAUDIO_MAC_H

#include "audio_interface.h"
// 使用相对路径或系统路径包含PortAudio
#ifdef __APPLE__
    #include <portaudio.h>
#endif
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * PortAudio implementation data structure
 */
typedef struct {
    PaStream* input_stream;
    PaStream* output_stream;
    PaStreamParameters input_params;
    PaStreamParameters output_params;
    
    // Ring buffers for audio data
    short* record_buffer;
    short* play_buffer;
    size_t record_buffer_size;
    size_t play_buffer_size;
    size_t record_read_pos;
    size_t record_write_pos;
    size_t play_read_pos;
    size_t play_write_pos;
    
    // Thread synchronization
    pthread_mutex_t record_mutex;
    pthread_mutex_t play_mutex;
    pthread_cond_t record_cond;
    pthread_cond_t play_cond;
    
    // State flags
    bool record_thread_running;
    bool play_thread_running;
    pthread_t record_thread;
    pthread_t play_thread;
} PortAudioMacData;

/**
 * Create PortAudio Mac implementation
 */
AudioInterface* portaudio_mac_create(void);

/**
 * PortAudio callback functions
 */
int portaudio_record_callback(const void* input_buffer, void* output_buffer,
                             unsigned long frame_count,
                             const PaStreamCallbackTimeInfo* time_info,
                             PaStreamCallbackFlags status_flags,
                             void* user_data);

int portaudio_play_callback(const void* input_buffer, void* output_buffer,
                           unsigned long frame_count,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data);

#ifdef __cplusplus
}
#endif

#endif // PORTAUDIO_MAC_H