#include "portaudio_mac.h"
#include "../log/linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declarations
static void portaudio_mac_init(AudioInterface* self);
static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate, 
                                     int frame_size, int channels, int periods, 
                                     int buffer_size, int period_size);
static bool portaudio_mac_read(AudioInterface* self, short* buffer, size_t frame_size);
static bool portaudio_mac_write(AudioInterface* self, short* buffer, size_t frame_size);
static void portaudio_mac_record(AudioInterface* self);
static void portaudio_mac_play(AudioInterface* self);
static void portaudio_mac_destroy(AudioInterface* self);

// VTable for PortAudio Mac implementation
static const AudioInterfaceVTable portaudio_mac_vtable = {
    .init = portaudio_mac_init,
    .set_config = portaudio_mac_set_config,
    .read = portaudio_mac_read,
    .write = portaudio_mac_write,
    .record = portaudio_mac_record,
    .play = portaudio_mac_play,
    .destroy = portaudio_mac_destroy
};

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

static void portaudio_mac_init(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        LOG_ERROR("Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
        return;
    }
    
    self->is_initialized = true;
    LOG_INFO("PortAudio initialized successfully");
}

static void portaudio_mac_set_config(AudioInterface* self, unsigned int sample_rate, 
                                     int frame_size, int channels, int periods, 
                                     int buffer_size, int period_size) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    // Set up input parameters
    data->input_params.device = Pa_GetDefaultInputDevice();
    if (data->input_params.device == paNoDevice) {
        LOG_ERROR("No default input device found");
        return;
    }
    
    const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(data->input_params.device);
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

int portaudio_record_callback(const void* input_buffer, void* output_buffer,
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
    size_t available_space = data->record_buffer_size - 
                            ((data->record_write_pos - data->record_read_pos + data->record_buffer_size) % data->record_buffer_size);
    
    if (samples_to_write <= available_space) {
        for (size_t i = 0; i < samples_to_write; i++) {
            data->record_buffer[data->record_write_pos] = input[i];
            data->record_write_pos = (data->record_write_pos + 1) % data->record_buffer_size;
        }
        pthread_cond_signal(&data->record_cond);
    }
    
    pthread_mutex_unlock(&data->record_mutex);
    
    return paContinue;
}

int portaudio_play_callback(const void* input_buffer, void* output_buffer,
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
    size_t available_data = (data->play_write_pos - data->play_read_pos + data->play_buffer_size) % data->play_buffer_size;
    
    if (samples_to_read <= available_data) {
        for (size_t i = 0; i < samples_to_read; i++) {
            output[i] = data->play_buffer[data->play_read_pos];
            data->play_read_pos = (data->play_read_pos + 1) % data->play_buffer_size;
        }
    } else {
        // Not enough data, output silence
        memset(output, 0, samples_to_read * sizeof(short));
    }
    
    pthread_mutex_unlock(&data->play_mutex);
    
    return paContinue;
}

static bool portaudio_mac_read(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer) {
        LOG_ERROR("Invalid parameters for read");
        return false;
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
            return false;
        }
        
        available_data = (data->record_write_pos - data->record_read_pos + data->record_buffer_size) % data->record_buffer_size;
    }
    
    if (available_data >= samples_needed) {
        for (size_t i = 0; i < samples_needed; i++) {
            buffer[i] = data->record_buffer[data->record_read_pos];
            data->record_read_pos = (data->record_read_pos + 1) % data->record_buffer_size;
        }
        pthread_mutex_unlock(&data->record_mutex);
        return true;
    }
    
    pthread_mutex_unlock(&data->record_mutex);
    return false;
}

static bool portaudio_mac_write(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer) {
        LOG_ERROR("Invalid parameters for write");
        return false;
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
        return true;
    }
    
    pthread_mutex_unlock(&data->play_mutex);
    return false;
}

static void portaudio_mac_record(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (self->is_recording) {
        LOG_WARN("Already recording");
        return;
    }
    
    PaError err = Pa_OpenStream(&data->input_stream,
                               &data->input_params,
                               NULL, // no output
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               portaudio_record_callback,
                               self);
    
    if (err != paNoError) {
        LOG_ERROR("Failed to open input stream: %s", Pa_GetErrorText(err));
        return;
    }
    
    err = Pa_StartStream(data->input_stream);
    if (err != paNoError) {
        LOG_ERROR("Failed to start input stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->input_stream);
        return;
    }
    
    self->is_recording = true;
    LOG_INFO("Recording started");
}

static void portaudio_mac_play(AudioInterface* self) {
    if (!self || !self->impl_data) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    PortAudioMacData* data = (PortAudioMacData*)self->impl_data;
    
    if (self->is_playing) {
        LOG_WARN("Already playing");
        return;
    }
    
    PaError err = Pa_OpenStream(&data->output_stream,
                               NULL, // no input
                               &data->output_params,
                               self->sample_rate,
                               self->frame_size,
                               paClipOff,
                               portaudio_play_callback,
                               self);
    
    if (err != paNoError) {
        LOG_ERROR("Failed to open output stream: %s", Pa_GetErrorText(err));
        return;
    }
    
    err = Pa_StartStream(data->output_stream);
    if (err != paNoError) {
        LOG_ERROR("Failed to start output stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(data->output_stream);
        return;
    }
    
    self->is_playing = true;
    LOG_INFO("Playback started");
}

static void portaudio_mac_destroy(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return;
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
    
    if (self->is_initialized) {
        Pa_Terminate();
    }
    
    free(self);
    
    LOG_INFO("PortAudio Mac implementation destroyed");
}