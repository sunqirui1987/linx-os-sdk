#ifndef AUDIO_INTERFACE_H
#define AUDIO_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Audio interface structure for C99 compatibility
 */
typedef struct AudioInterface AudioInterface;

/**
 * Audio interface function pointers
 */
typedef struct {
    int (*init)(AudioInterface* self);
    void (*set_config)(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                      int channels, int periods, int buffer_size, int period_size);
    int (*read)(AudioInterface* self, short* buffer, size_t frame_size);
    int (*write)(AudioInterface* self, short* buffer, size_t frame_size);
    int (*record)(AudioInterface* self);
    int (*play)(AudioInterface* self);
    bool (*is_play_buffer_empty)(AudioInterface* self);
    int (*destroy)(AudioInterface* self);
} AudioInterfaceVTable;

/**
 * Base audio interface structure
 */
struct AudioInterface {
    const AudioInterfaceVTable* vtable;
    void* impl_data;  // Implementation-specific data
    
    // Common configuration
    unsigned int sample_rate;
    int frame_size;
    int channels;
    int periods;
    int buffer_size;
    int period_size;
    
    // State
    bool is_recording;
    bool is_playing;
    bool is_initialized;
};

/**
 * Initialize audio interface
 */
int audio_interface_init(AudioInterface* self);

/**
 * Set audio configuration
 */
void audio_interface_set_config(AudioInterface* self, unsigned int sample_rate, 
                               int frame_size, int channels, int periods, 
                               int buffer_size, int period_size);

/**
 * Read audio data
 */
int audio_interface_read(AudioInterface* self, short* buffer, size_t frame_size);

/**
 * Write audio data
 */
int audio_interface_write(AudioInterface* self, short* buffer, size_t frame_size);

/**
 * Start recording
 */
int audio_interface_record(AudioInterface* self);

/**
 * Start playing
 */
int audio_interface_play(AudioInterface* self);

/**
 * Check if play buffer is empty
 */
bool audio_interface_is_play_buffer_empty(AudioInterface* self);

/**
 * Destroy audio interface
 */
int audio_interface_destroy(AudioInterface* self);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_INTERFACE_H