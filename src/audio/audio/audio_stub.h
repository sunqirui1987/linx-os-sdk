#ifndef AUDIO_STUB_H
#define AUDIO_STUB_H

#include "audio_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stub audio implementation data structure
 * This is a placeholder implementation for platforms without audio support
 */
typedef struct {
    bool initialized;
    bool recording;
    bool playing;
} AudioStubData;

/**
 * Create stub audio implementation
 * @return AudioInterface instance or NULL on failure
 */
AudioInterface* audio_stub_create(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_STUB_H