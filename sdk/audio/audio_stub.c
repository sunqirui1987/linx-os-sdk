#include "audio_stub.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for vtable functions
static void audio_stub_init(AudioInterface* self);
static void audio_stub_set_config(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                 int channels, int periods, int buffer_size, int period_size);
static bool audio_stub_read(AudioInterface* self, short* buffer, size_t frame_size);
static bool audio_stub_write(AudioInterface* self, short* buffer, size_t frame_size);
static void audio_stub_record(AudioInterface* self);
static void audio_stub_play(AudioInterface* self);
static void audio_stub_destroy(AudioInterface* self);

// Stub vtable
static const AudioInterfaceVTable audio_stub_vtable = {
    .init = audio_stub_init,
    .set_config = audio_stub_set_config,
    .read = audio_stub_read,
    .write = audio_stub_write,
    .record = audio_stub_record,
    .play = audio_stub_play,
    .destroy = audio_stub_destroy
};

AudioInterface* audio_stub_create(void) {
    AudioInterface* interface = (AudioInterface*)malloc(sizeof(AudioInterface));
    if (!interface) {
        return NULL;
    }
    
    AudioStubData* data = (AudioStubData*)malloc(sizeof(AudioStubData));
    if (!data) {
        free(interface);
        return NULL;
    }
    
    // Initialize interface
    memset(interface, 0, sizeof(AudioInterface));
    interface->vtable = &audio_stub_vtable;
    interface->impl_data = data;
    
    // Initialize stub data
    memset(data, 0, sizeof(AudioStubData));
    
    return interface;
}

static void audio_stub_init(AudioInterface* self) {
    if (!self) return;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) return;
    
    data->initialized = true;
    self->is_initialized = true;
}

static void audio_stub_set_config(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                                 int channels, int periods, int buffer_size, int period_size) {
    if (!self) return;
    
    // Store configuration in the interface
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
}

static bool audio_stub_read(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !buffer) return false;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data || !data->recording) return false;
    
    // Fill buffer with silence (zeros)
    memset(buffer, 0, frame_size * sizeof(short));
    return true; // Success (stub implementation - returns silence)
}

static bool audio_stub_write(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !buffer) return false;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data || !data->playing) return false;
    
    // Stub implementation - just discard the data
    (void)frame_size; // Suppress unused parameter warning
    return true; // Success (stub implementation - discards data)
}

static void audio_stub_record(AudioInterface* self) {
    if (!self) return;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) return;
    
    data->recording = true;
    self->is_recording = true;
}

static void audio_stub_play(AudioInterface* self) {
    if (!self) return;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (!data) return;
    
    data->playing = true;
    self->is_playing = true;
}

static void audio_stub_destroy(AudioInterface* self) {
    if (!self) return;
    
    AudioStubData* data = (AudioStubData*)self->impl_data;
    if (data) {
        free(data);
    }
    free(self);
}