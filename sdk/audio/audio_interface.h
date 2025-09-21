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
    void (*init)(AudioInterface* self);
    void (*set_config)(AudioInterface* self, unsigned int sample_rate, int frame_size, 
                      int channels, int periods, int buffer_size, int period_size);
    bool (*read)(AudioInterface* self, short* buffer, size_t frame_size);
    bool (*write)(AudioInterface* self, short* buffer, size_t frame_size);
    void (*record)(AudioInterface* self);
    void (*play)(AudioInterface* self);
    void (*destroy)(AudioInterface* self);
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
void audio_interface_init(AudioInterface* self);

/**
 * Set audio configuration
 */
void audio_interface_set_config(AudioInterface* self, unsigned int sample_rate, 
                               int frame_size, int channels, int periods, 
                               int buffer_size, int period_size);

/**
 * Read audio data
 */
bool audio_interface_read(AudioInterface* self, short* buffer, size_t frame_size);

/**
 * Write audio data
 */
bool audio_interface_write(AudioInterface* self, short* buffer, size_t frame_size);

/**
 * Start recording
 */
void audio_interface_record(AudioInterface* self);

/**
 * Start playing
 */
void audio_interface_play(AudioInterface* self);

/**
 * Destroy audio interface
 */
void audio_interface_destroy(AudioInterface* self);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_INTERFACE_H