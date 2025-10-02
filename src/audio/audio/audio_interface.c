#include "audio_interface.h"
#include "../log/linx_log.h"

int audio_interface_init(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->init) {
        LOG_ERROR("Invalid audio interface or vtable");
        return -1;
    }
    return self->vtable->init(self);
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

int audio_interface_read(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->vtable || !self->vtable->read) {
        LOG_ERROR("Invalid audio interface or vtable");
        return -1;
    }
    return self->vtable->read(self, buffer, frame_size);
}

int audio_interface_write(AudioInterface* self, short* buffer, size_t frame_size) {
    if (!self || !self->vtable || !self->vtable->write) {
        LOG_ERROR("Invalid audio interface or vtable");
        return -1;
    }
    return self->vtable->write(self, buffer, frame_size);
}

int audio_interface_record(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->record) {
        LOG_ERROR("Invalid audio interface or vtable");
        return -1;

    }
    return self->vtable->record(self);
}

int audio_interface_init_play(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->init_play) {
        LOG_ERROR("Invalid audio interface or vtable");
       return -1;
    }
    return self->vtable->init_play(self);
}

bool audio_interface_is_play_buffer_empty(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->is_play_buffer_empty) {
        LOG_ERROR("Invalid audio interface or vtable");
        return true; // 默认返回true，表示缓冲区为空
    }
    return self->vtable->is_play_buffer_empty(self);
}

int audio_interface_destroy(AudioInterface* self) {
    if (!self || !self->vtable || !self->vtable->destroy) {
        LOG_ERROR("Invalid audio interface or vtable");
       return -1;
    }
    
    return self->vtable->destroy(self);
}