#include "portaudio_mac.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



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


// Forward declarations for vtable functions
static int portaudio_mac_init(AudioInterface* self);
static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate,
                                    int frame_size, int channels, int periods,
                                    int buffer_size, int period_size);
static int portaudio_mac_read(AudioInterface* self, short* buffer, size_t frame_size);
static int portaudio_mac_write(AudioInterface* self, short* buffer, size_t frame_size);
static int portaudio_mac_record(AudioInterface* self);
static int portaudio_mac_init_play(AudioInterface* self);
static bool portaudio_mac_is_play_buffer_empty(AudioInterface* self);
static int portaudio_mac_destroy(AudioInterface* self);

// VTable for PortAudio Mac implementation
static const AudioInterfaceVTable portaudio_mac_vtable = {
    .init = portaudio_mac_init,
    .set_config = portaudio_mac_set_config,
    .read = portaudio_mac_read,
    .write = portaudio_mac_write,
    .record = portaudio_mac_record,
    .init_play = portaudio_mac_init_play,
    .is_play_buffer_empty = portaudio_mac_is_play_buffer_empty,
    .destroy = portaudio_mac_destroy
};


/**
 * PortAudio callback functions
 */
static int _portaudio_record_callback(const void* input_buffer, void* output_buffer,
                             unsigned long frame_count,
                             const PaStreamCallbackTimeInfo* time_info,
                             PaStreamCallbackFlags status_flags,
                             void* user_data);

static int _portaudio_play_callback(const void* input_buffer, void* output_buffer,
                           unsigned long frame_count,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data);

AudioInterface* portaudio_mac_create(void) {
    AudioInterface* interface = (AudioInterface*)malloc(sizeof(AudioInterface));
    if (!interface) {
        LOG_ERROR("Failed to allocate memory for AudioInterface");
        return NULL;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)malloc(sizeof(PortAudioMacData));
    if (!data) {
        LOG_ERROR("Failed to allocate memory for PortAudioMacData");
        free(interface);
        return NULL;
    }
    
    // Initialize structure
    memset(interface, 0, sizeof(AudioInterface));
    memset(data, 0, sizeof(PortAudioMacData));
    
    interface->vtable = &portaudio_mac_vtable;
    interface->impl_data = data;
    
    // Initialize mutexes and conditions
    pthread_mutex_init(&data->record_mutex, NULL);
    pthread_mutex_init(&data->play_mutex, NULL);
    pthread_cond_init(&data->record_cond, NULL);
    pthread_cond_init(&data->play_cond, NULL);
    
    return interface;
}

static int portaudio_mac_init(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return -1;
    }
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        LOG_ERROR("Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
        return -1;
    }
    
    self->is_initialized = true;
    LOG_INFO("PortAudio initialized successfully");
    return 0; // Success
}

static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate, 
                                     int frame_size, int channels, int periods, 
                                     int buffer_size, int period_size) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // Store configuration in AudioInterface structure
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    // Set up input parameters
    data->input_params.device = Pa_GetDefaultInputDevice();
    if (data->input_params.device == paNoDevice) {
        LOG_ERROR("No default input device found");
        return;
    }
    
    const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(data->input_params.device);
    if (!inputDeviceInfo) {
        LOG_ERROR("Failed to get input device info");
        return;
    }
    
    // Use the minimum of requested channels and device max channels
    int inputChannels = (channels <= inputDeviceInfo->maxInputChannels) ? channels : inputDeviceInfo->maxInputChannels;
    
    data->input_params.channelCount = inputChannels;
    data->input_params.sampleFormat = paInt16;
    data->input_params.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
    data->input_params.hostApiSpecificStreamInfo = NULL;
    
    LOG_INFO("Input device: %s, channels: %d (requested: %d, max: %d)", 
             inputDeviceInfo->name, inputChannels, channels, inputDeviceInfo->maxInputChannels);
    
    // Set up output parameters
    data->output_params.device = Pa_GetDefaultOutputDevice();
    if (data->output_params.device == paNoDevice) {
        LOG_ERROR("No default output device found");
        return;
    }
    
    const PaDeviceInfo* outputDeviceInfo = Pa_GetDeviceInfo(data->output_params.device);
    if (!outputDeviceInfo) {
        LOG_ERROR("Failed to get output device info");
        return;
    }
    
    // Use the minimum of requested channels and device max channels
    int outputChannels = (channels <= outputDeviceInfo->maxOutputChannels) ? channels : outputDeviceInfo->maxOutputChannels;
    
    data->output_params.channelCount = outputChannels;
    data->output_params.sampleFormat = paInt16;
    data->output_params.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
    data->output_params.hostApiSpecificStreamInfo = NULL;
    
    LOG_INFO("Output device: %s, channels: %d (requested: %d, max: %d)", 
             outputDeviceInfo->name, outputChannels, channels, outputDeviceInfo->maxOutputChannels);
    
    // Allocate ring buffers
    data->record_buffer_size = buffer_size * channels;
    data->play_buffer_size = buffer_size * channels;
    
    data->record_buffer = (short*)malloc(data->record_buffer_size * sizeof(short));
    data->play_buffer = (short*)malloc(data->play_buffer_size * sizeof(short));
    
    if (!data->record_buffer || !data->play_buffer) {
        LOG_ERROR("Failed to allocate audio buffers");
        return;
    }
    
    memset(data->record_buffer, 0, data->record_buffer_size * sizeof(short));
    memset(data->play_buffer, 0, data->play_buffer_size * sizeof(short));
    
    LOG_INFO("Audio configuration set: %u Hz, %d channels, %d frame size", 
                  sample_rate, channels, frame_size);
}

int _portaudio_record_callback(const void* input_buffer, void* output_buffer,
                             unsigned long frame_count,
                             const PaStreamCallbackTimeInfo* time_info,
                             PaStreamCallbackFlags status_flags,
                             void* user_data) {
    AudioInterface* interface = (AudioInterface*)user_data;
    PortAudioMacData* data = (PortAudioMacData*)interface->impl_data;
    const short* input = (const short*)input_buffer;
    
    if (!input || !data) {
        return paContinue;
    }
    
    pthread_mutex_lock(&data->record_mutex);
    
    size_t samples_to_write = frame_count * interface->channels;
    // Calculate used space in the ring buffer
    size_t used_space = (data->record_write_pos - data->record_read_pos + data->record_buffer_size) % data->record_buffer_size;
    // Available space is buffer size minus used space minus 1 (to distinguish full from empty)
    size_t available_space = data->record_buffer_size - used_space - 1;
    
    if (samples_to_write <= available_space) {
        for (size_t i = 0; i < samples_to_write; i++) {
            data->record_buffer[data->record_write_pos] = input[i];
            data->record_write_pos = (data->record_write_pos + 1) % data->record_buffer_size;
        }
        pthread_cond_signal(&data->record_cond);
    } else {
        // Buffer overflow - log warning but continue
        LOG_WARN("Record buffer overflow, dropping %lu samples", samples_to_write);
    }
    
    pthread_mutex_unlock(&data->record_mutex);
    
    return paContinue;
}

int _portaudio_play_callback(const void* input_buffer, void* output_buffer,
                           unsigned long frame_count,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data) {
    AudioInterface* interface = (AudioInterface*)user_data;
    PortAudioMacData* data = (PortAudioMacData*)interface->impl_data;
    short* output = (short*)output_buffer;
    
    if (!output || !data) {
        return paContinue;
    }
    
    pthread_mutex_lock(&data->play_mutex);
    
    size_t samples_to_read = frame_count * interface->channels;
    // Calculate available data in the ring buffer
    size_t available_data = (data->play_write_pos - data->play_read_pos + data->play_buffer_size) % data->play_buffer_size;
    
    if (samples_to_read <= available_data) {
        for (size_t i = 0; i < samples_to_read; i++) {
            output[i] = data->play_buffer[data->play_read_pos];
            data->play_read_pos = (data->play_read_pos + 1) % data->play_buffer_size;
        }
    } else {
        // Not enough data, output silence
        memset(output, 0, samples_to_read * sizeof(short));
        LOG_DEBUG("Play buffer underrun, outputting silence for %lu samples", samples_to_read);
    }
    
    pthread_mutex_unlock(&data->play_mutex);
    
    return paContinue;
}

static int portaudio_mac_read(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer) {
        LOG_ERROR("Invalid parameters for read");
        return -1;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    size_t samples_needed = frame_size * self->channels;
    
    pthread_mutex_lock(&data->record_mutex);
    
    size_t available_data = (data->record_write_pos - data->record_read_pos + data->record_buffer_size) % data->record_buffer_size;
    
    if (available_data < samples_needed) {
        // Wait for more data
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1; // 1 second timeout
        
        int result = pthread_cond_timedwait(&data->record_cond, &data->record_mutex, &timeout);
        if (result != 0) {
            pthread_mutex_unlock(&data->record_mutex);
            return -1;
        }
        
        available_data = (data->record_write_pos - data->record_read_pos + data->record_buffer_size) % data->record_buffer_size;
    }
    
    if (available_data >= samples_needed) {
        for (size_t i = 0; i < samples_needed; i++) {
            buffer[i] = data->record_buffer[data->record_read_pos];
            data->record_read_pos = (data->record_read_pos + 1) % data->record_buffer_size;
        }
        pthread_mutex_unlock(&data->record_mutex);
        return 0; // Success
    }
    
    pthread_mutex_unlock(&data->record_mutex);
    return -1;
}

static int portaudio_mac_write(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer) {
        LOG_ERROR("Invalid parameters for write");
        return -1;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    size_t samples_to_write = frame_size * self->channels;
    
    pthread_mutex_lock(&data->play_mutex);
    
    size_t available_space = data->play_buffer_size - 
                            ((data->play_write_pos - data->play_read_pos + data->play_buffer_size) % data->play_buffer_size);
    
    if (available_space >= samples_to_write) {
        for (size_t i = 0; i < samples_to_write; i++) {
            data->play_buffer[data->play_write_pos] = buffer[i];
            data->play_write_pos = (data->play_write_pos + 1) % data->play_buffer_size;
        }
        pthread_mutex_unlock(&data->play_mutex);
        return 0; // Success
    }
    
    pthread_mutex_unlock(&data->play_mutex);
    return -1;
}

static int portaudio_mac_record(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return -1;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (self->is_recording) {
        LOG_WARN("Already recording");
        return 0; // Already recording is not an error
    }
    
    // Validate that configuration has been set
    if (self->sample_rate == 0 || self->frame_size == 0 || self->channels == 0) {
        LOG_ERROR("Audio configuration not set. Call set_config first.");
        return -1;
    }
    
    // Validate input device
    if (data->input_params.device == paNoDevice) {
        LOG_ERROR("No input device configured");
        return -1;
    }
    
    // Check if device is still valid
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(data->input_params.device);
    if (!deviceInfo) {
        LOG_ERROR("Input device is no longer available");
        return -1;
    }
    
    LOG_INFO("Opening input stream: device=%s, rate=%u, channels=%d, frame_size=%d", 
             deviceInfo->name, self->sample_rate, self->channels, self->frame_size);
    
    PaError err = Pa_OpenStream(&data->input_stream,
                               &data->input_params,
                               NULL, // no output
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               _portaudio_record_callback,
                               self);
    
    if (err != paNoError) {
        LOG_ERROR("Failed to open input stream: %s", Pa_GetErrorText(err));
        return -1;
    }
    
    err = Pa_StartStream(data->input_stream);
    if (err != paNoError) {
        LOG_ERROR("Failed to start input stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->input_stream);
        return -1;
    }
    
    self->is_recording = true;
    LOG_INFO("Recording started");
    return 0; // Success
}

static int portaudio_mac_init_play(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return -1;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (self->is_playing) {
        LOG_WARN("Already playing");
        return 0; // Already playing is not an error
    }
    
    // Validate that configuration has been set
    if (self->sample_rate == 0 || self->frame_size == 0 || self->channels == 0) {
        LOG_ERROR("Audio configuration not set. Call set_config first.");
        return -1;
    }
    
    // Validate output device
    if (data->output_params.device == paNoDevice) {
        LOG_ERROR("No output device configured");
        return -1;
    }
    
    // Check if device is still valid
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(data->output_params.device);
    if (!deviceInfo) {
        LOG_ERROR("Output device is no longer available");
        return -1;
    }
    
    LOG_INFO("Opening output stream: device=%s, rate=%u, channels=%d, frame_size=%d", 
             deviceInfo->name, self->sample_rate, self->channels, self->frame_size);
    
    PaError err = Pa_OpenStream(&data->output_stream,
                               NULL, // no input
                               &data->output_params,
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               _portaudio_play_callback,
                               self);
    
    if (err != paNoError) {
        LOG_ERROR("Failed to open output stream: %s", Pa_GetErrorText(err));
        return -1;
    }
    
    err = Pa_StartStream(data->output_stream);
    if (err != paNoError) {
        LOG_ERROR("Failed to start output stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->output_stream);
        return -1;
    }
    
    self->is_playing = true;
    LOG_INFO("Playback started");
    return 0; // Success
}

static bool portaudio_mac_is_play_buffer_empty(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return true; // 如果接口无效，认为缓冲区为空
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // 如果没有在播放，认为缓冲区为空
    if (!self->is_playing) {
        return true;
    }
    
    // 检查播放缓冲区是否有数据
    pthread_mutex_lock(&data->play_mutex);
    size_t available_data = (data->play_write_pos - data->play_read_pos + data->play_buffer_size) % data->play_buffer_size;
    pthread_mutex_unlock(&data->play_mutex);
    
    // 如果缓冲区中没有数据，认为为空
    return (available_data == 0);
}

static int portaudio_mac_destroy(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // Stop and close streams
    if (data->input_stream) {
        Pa_StopStream(data->input_stream);
        Pa_CloseStream(data->input_stream);
    }
    
    if (data->output_stream) {
        Pa_StopStream(data->output_stream);
        Pa_CloseStream(data->output_stream);
    }
    
    // Clean up buffers
    if (data->record_buffer) {
        free(data->record_buffer);
    }
    
    if (data->play_buffer) {
        free(data->play_buffer);
    }
    
    // Clean up synchronization objects
    pthread_mutex_destroy(&data->record_mutex);
    pthread_mutex_destroy(&data->play_mutex);
    pthread_cond_destroy(&data->record_cond);
    pthread_cond_destroy(&data->play_cond);
    
    free(data);
    self->impl_data = NULL;
    
    if (self->is_initialized) {
        Pa_Terminate();
    }
    
    LOG_INFO("PortAudio Mac implementation destroyed");
    return 0; // Success
}