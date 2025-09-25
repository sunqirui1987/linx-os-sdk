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
 * Create PortAudio Mac implementation
 */
AudioInterface* portaudio_mac_create(void);


#ifdef __cplusplus
}
#endif

#endif // PORTAUDIO_MAC_H