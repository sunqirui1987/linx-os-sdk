/**
 * @file audio_v812.c
 * @brief V812 audio implementation for Allwinner V812 SoC
 */

#include "audio_v812.h"
#include "v812/record_ai.h"
#include "v812/play_ao.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

// Global mutex for thread safety
static pthread_mutex_t g_v812_mutex = PTHREAD_MUTEX_INITIALIZER;

// V812 implementation functions
static int audio_v812_init_impl(AudioInterface* self);
static void audio_v812_set_config_impl(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                       int channels, int periods, int buffer_size, int period_size);
static int audio_v812_read_impl(AudioInterface* self, short* buffer, size_t frame_size);
static int audio_v812_write_impl(AudioInterface* self, short* buffer, size_t frame_size);
static int audio_v812_record_impl(AudioInterface* self);
static int audio_v812_init_play_impl(AudioInterface* self);
static bool audio_v812_is_play_buffer_empty_impl(AudioInterface* self);
static int audio_v812_destroy_impl(AudioInterface* self);

// V812 vtable
static const AudioInterfaceVTable audio_v812_vtable = {
    .init = audio_v812_init_impl,
    .set_config = audio_v812_set_config_impl,
    .read = audio_v812_read_impl,
    .write = audio_v812_write_impl,
    .record = audio_v812_record_impl,
    .init_play = audio_v812_init_play_impl,
    .is_play_buffer_empty = audio_v812_is_play_buffer_empty_impl,
    .destroy = audio_v812_destroy_impl
};

static int audio_v812_init_impl(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (v812_data->initialized) {
        pthread_mutex_unlock(&g_v812_mutex);
        return 0; // Already initialized
    }
    
    // Initialize V812 specific data
    v812_data->recording = false;
    v812_data->playing = false;
    v812_data->record_ctx = NULL;
    v812_data->play_ctx = NULL;
    v812_data->bit_width = 16;
    v812_data->mic_num = 1;
    v812_data->ai_gain = 8;
    v812_data->ao_volume = 8;
    
    v812_data->initialized = true;
    self->is_initialized = true;
    
    pthread_mutex_unlock(&g_v812_mutex);
    return 0;
}

static void audio_v812_set_config_impl(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                       int channels, int periods, int buffer_size, int period_size) {
    if (!self || !self->impl_data) {
        return;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized) {
        pthread_mutex_unlock(&g_v812_mutex);
        return;
    }
    
    // Update interface configuration
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    // Update V812 specific configuration
    v812_data->mic_num = channels;
    
    pthread_mutex_unlock(&g_v812_mutex);
}

static int audio_v812_read_impl(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer || frame_size == 0) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized || !v812_data->recording || !v812_data->record_ctx) {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    void* frame_data = NULL;
    size_t data_size = 0;
    
    // Get frame from V812 recording context
    int ret = record_ai_get_frame((record_ai_context_t*)v812_data->record_ctx, 
                                 &frame_data, &data_size, 1000); // 1 second timeout
    
    if (ret != 0 || !frame_data || data_size == 0) {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    // Copy data to user buffer (convert bytes to samples)
    size_t bytes_to_copy = frame_size * sizeof(short);
    if (data_size < bytes_to_copy) {
        bytes_to_copy = data_size;
    }
    memcpy(buffer, frame_data, bytes_to_copy);
    
    // Release the frame
    record_ai_release_frame((record_ai_context_t*)v812_data->record_ctx);
    
    pthread_mutex_unlock(&g_v812_mutex);
    return (int)(bytes_to_copy / sizeof(short)); // Return number of samples
}

static int audio_v812_write_impl(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->impl_data || !buffer || frame_size == 0) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized || !v812_data->playing || !v812_data->play_ctx) {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    // Send frame to V812 playback context
    size_t bytes_to_send = frame_size * sizeof(short);
    int ret = play_ao_send_frame((play_ao_context_t*)v812_data->play_ctx, 
                                buffer, bytes_to_send, 1000); // 1 second timeout
    
    pthread_mutex_unlock(&g_v812_mutex);
    return (ret == 0) ? (int)frame_size : -1;
}

static int audio_v812_record_impl(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized) {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    if (!v812_data->recording) {
        // Initialize recording context if not exists
        if (!v812_data->record_ctx) {
            int ret = record_ai_init((record_ai_context_t**)&v812_data->record_ctx,
                                   self->sample_rate,
                                   self->channels,
                                   v812_data->bit_width,
                                   self->frame_size,
                                   v812_data->mic_num,
                                   v812_data->ai_gain);
            if (ret != 0) {
                pthread_mutex_unlock(&g_v812_mutex);
                return -1;
            }
        }
        
        // Start recording
        int ret = record_ai_start((record_ai_context_t*)v812_data->record_ctx);
        if (ret == 0) {
            v812_data->recording = true;
            self->is_recording = true;
        } else {
            pthread_mutex_unlock(&g_v812_mutex);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&g_v812_mutex);
    return 0;
}

static int audio_v812_init_play_impl(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized) {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    // Initialize playback context if not exists
    if (!v812_data->play_ctx) {
        int ret = play_ao_init((play_ao_context_t**)&v812_data->play_ctx,
                              self->sample_rate,
                              self->channels,
                              v812_data->bit_width,
                              self->frame_size,
                              v812_data->ao_volume);
        if (ret != 0) {
            pthread_mutex_unlock(&g_v812_mutex);
            return -1;
        }
    }
    
    // Start playback
    int ret = play_ao_start((play_ao_context_t*)v812_data->play_ctx);
    if (ret == 0) {
        v812_data->playing = true;
        self->is_playing = true;
    } else {
        pthread_mutex_unlock(&g_v812_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&g_v812_mutex);
    return 0;
}

static bool audio_v812_is_play_buffer_empty_impl(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return true;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    bool is_empty = true;
    if (v812_data->initialized && v812_data->playing && v812_data->play_ctx) {
        // For V812, we consider buffer empty if not playing
        is_empty = !play_ao_is_playing((play_ao_context_t*)v812_data->play_ctx);
    }
    
    pthread_mutex_unlock(&g_v812_mutex);
    return is_empty;
}

static int audio_v812_destroy_impl(AudioInterface* self) {
    if (!self || !self->impl_data) {
        return -1;
    }
    
    AudioV812Data* v812_data = (AudioV812Data*)self->impl_data;
    
    pthread_mutex_lock(&g_v812_mutex);
    
    if (!v812_data->initialized) {
        pthread_mutex_unlock(&g_v812_mutex);
        return 0;
    }
    
    // Stop and destroy recording context
    if (v812_data->record_ctx) {
        if (v812_data->recording) {
            record_ai_stop((record_ai_context_t*)v812_data->record_ctx);
        }
        record_ai_destroy((record_ai_context_t*)v812_data->record_ctx);
        v812_data->record_ctx = NULL;
        v812_data->recording = false;
    }
    
    // Stop and destroy playback context
    if (v812_data->play_ctx) {
        if (v812_data->playing) {
            play_ao_stop((play_ao_context_t*)v812_data->play_ctx);
        }
        play_ao_destroy((play_ao_context_t*)v812_data->play_ctx);
        v812_data->play_ctx = NULL;
        v812_data->playing = false;
    }
    
    v812_data->initialized = false;
    self->is_initialized = false;
    self->is_recording = false;
    self->is_playing = false;
    
    pthread_mutex_unlock(&g_v812_mutex);
    return 0;
}

AudioInterface* audio_v812_create(void) {
    // Allocate AudioInterface
    AudioInterface* interface = (AudioInterface*)malloc(sizeof(AudioInterface));
    if (!interface) {
        return NULL;
    }
    
    // Allocate V812 specific data
    AudioV812Data* v812_data = (AudioV812Data*)malloc(sizeof(AudioV812Data));
    if (!v812_data) {
        free(interface);
        return NULL;
    }
    
    // Initialize V812 data
    memset(v812_data, 0, sizeof(AudioV812Data));
    v812_data->initialized = false;
    v812_data->recording = false;
    v812_data->playing = false;
    v812_data->record_ctx = NULL;
    v812_data->play_ctx = NULL;
    v812_data->bit_width = 16;
    v812_data->mic_num = 1;
    v812_data->ai_gain = 8;
    v812_data->ao_volume = 8;
    
    // Initialize AudioInterface
    memset(interface, 0, sizeof(AudioInterface));
    interface->vtable = &audio_v812_vtable;
    interface->impl_data = v812_data;
    interface->sample_rate = 16000;
    interface->channels = 1;
    interface->frame_size = 320;
    interface->periods = 4;
    interface->buffer_size = 4096;
    interface->period_size = 1024;
    interface->is_initialized = false;
    interface->is_recording = false;
    interface->is_playing = false;
    
    return interface;
}