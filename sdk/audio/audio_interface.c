#include "audio_interface.h"
#include "../log/linx_log.h"

void audio_interface_init(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->init) {
        LOG_ERROR("Invalid audio interface or vtable");
        return;
    }
    self->vtable->init(self);
}

void audio_interface_set_config(AudioInterface* self, unsigned int sample_rate, 
                               int frame_size, int channels, int periods, 
                               int buffer_size, int period_size) {
    if (!self) {
        LOG_ERROR("Invalid audio interface");
        return;
    }
    
    self->sample_rate = sample_rate;
    self->frame_size = frame_size;
    self->channels = channels;
    self->periods = periods;
    self->buffer_size = buffer_size;
    self->period_size = period_size;
    
    if (self->vtable && self->vtable->set_config) {
        self->vtable->set_config(self, sample_rate, frame_size, channels, 
                                periods, buffer_size, period_size);
    }
}

bool audio_interface_read(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->vtable || !self->vtable->read) {
        LOG_ERROR("Invalid audio interface or vtable");
        return false;
    }
    return self->vtable->read(self, buffer, frame_size);
}

bool audio_interface_write(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->vtable || !self->vtable->write) {
        LOG_ERROR("Invalid audio interface or vtable");
        return false;
    }
    return self->vtable->write(self, buffer, frame_size);
}

void audio_interface_record(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->record) {
        LOG_ERROR("Invalid audio interface or vtable");
        return;
    }
    self->vtable->record(self);
}

void audio_interface_play(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->play) {
        LOG_ERROR("Invalid audio interface or vtable");
        return;
    }
    self->vtable->play(self);
}

void audio_interface_destroy(AudioInterface* self) {
    if (!self) {
        return;
    }
    
    if (self->vtable && self->vtable->destroy) {
        self->vtable->destroy(self);
    }
}