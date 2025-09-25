#ifndef AUDIO_V812_H
#define AUDIO_V812_H

#include "audio_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize audio V812 module
 * 
 * This function must be called before using any other audio functions.
 * It initializes the global audio context and prepares the system for
 * audio operations.
 * 
 * @return 0 on success, negative error code on failure
 */
int audio_v812_init(void);

/**
 * @brief Cleanup and destroy audio V812 module
 * 
 * This function cleans up all resources and should be called when
 * audio functionality is no longer needed.
 * 
 * @return 0 on success, negative error code on failure
 */
int audio_v812_destroy(void);

/* ========== Recording Functions ========== */

/**
 * @brief Initialize audio recording with specified configuration
 * 
 * @param sample_rate Sample rate in Hz (e.g., 8000, 16000, 44100)
 * @param channel_count Number of channels (1 for mono, 2 for stereo)
 * @param bit_width Bit width (16, 24, 32)
 * @param frame_size Frame size in samples
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_init(int sample_rate, int channel_count, int bit_width, int frame_size);

/**
 * @brief Start audio recording
 * 
 * @param data_callback Callback function to receive audio data
 *                      Signature: void callback(const void* data, size_t size, void* user_data)
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_start(void (*data_callback)(const void* data, size_t size, void* user_data),
                           void* user_data);

/**
 * @brief Stop audio recording
 * 
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_stop(void);

/**
 * @brief Get one audio frame from recording (blocking)
 * 
 * This function provides direct access to audio frames using the
 * AW_MPI_AI_GetFrame capability. It blocks until a frame is available
 * or timeout occurs.
 * 
 * @param data Pointer to store audio data pointer
 * @param size Pointer to store data size
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_get_frame(void** data, size_t* size, int timeout_ms);

/**
 * @brief Release audio frame after processing
 * 
 * This function must be called after processing a frame obtained
 * via audio_v812_record_get_frame() to release resources.
 * 
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_release_frame(void);

/**
 * @brief Set recording gain
 * 
 * @param gain Gain value (typically 0-15)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_set_gain(int gain);

/**
 * @brief Get recording gain
 * 
 * @param gain Pointer to store gain value
 * @return 0 on success, negative error code on failure
 */
int audio_v812_record_get_gain(int* gain);

/**
 * @brief Check if recording is active
 * 
 * @return true if recording, false otherwise
 */
bool audio_v812_is_recording(void);

/* ========== Playback Functions ========== */

/**
 * @brief Initialize audio playback with specified configuration
 * 
 * @param sample_rate Sample rate in Hz (e.g., 8000, 16000, 44100)
 * @param channel_count Number of channels (1 for mono, 2 for stereo)
 * @param bit_width Bit width (16, 24, 32)
 * @param frame_size Frame size in samples
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_init(int sample_rate, int channel_count, int bit_width, int frame_size);

/**
 * @brief Start audio playback
 * 
 * @param data_request_callback Callback function to request audio data
 *                              Signature: int callback(void* buffer, size_t size, void* user_data)
 *                              Return: number of bytes written to buffer, 0 for EOF, negative for error
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_start(int (*data_request_callback)(void* buffer, size_t size, void* user_data),
                         void* user_data);

/**
 * @brief Stop audio playback
 * 
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_stop(void);

/**
 * @brief Send audio data for playback
 * 
 * This function provides direct access to send audio frames using the
 * AW_MPI_AO_SendFrame capability.
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_send_data(const void* data, size_t size, int timeout_ms);

/**
 * @brief Set playback volume
 * 
 * @param volume Volume value (typically 0-15)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_set_volume(int volume);

/**
 * @brief Get playback volume
 * 
 * @param volume Pointer to store volume value
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_get_volume(int* volume);

/**
 * @brief Check if playback is active
 * 
 * @return true if playing, false otherwise
 */
bool audio_v812_is_playing(void);

/**
 * @brief Set end of stream for playback
 * 
 * @param eof_flag End of stream flag
 * @param immediate_flag Immediate flag (true for immediate stop)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_set_eof(bool eof_flag, bool immediate_flag);

/**
 * @brief Wait for playback to complete
 * 
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return 0 on success, negative error code on failure
 */
int audio_v812_play_wait_eof(int timeout_ms);

/* ========== Common Definitions ========== */

/**
 * @brief Common sample rates
 */
#define AUDIO_V812_SAMPLE_RATE_8K    8000
#define AUDIO_V812_SAMPLE_RATE_16K   16000
#define AUDIO_V812_SAMPLE_RATE_22K   22050
#define AUDIO_V812_SAMPLE_RATE_44K   44100
#define AUDIO_V812_SAMPLE_RATE_48K   48000

/**
 * @brief Common bit widths
 */
#define AUDIO_V812_BIT_WIDTH_16      16
#define AUDIO_V812_BIT_WIDTH_24      24
#define AUDIO_V812_BIT_WIDTH_32      32

/**
 * @brief Common channel counts
 */
#define AUDIO_V812_CHANNEL_MONO      1
#define AUDIO_V812_CHANNEL_STEREO    2

/**
 * @brief Common frame sizes
 */
#define AUDIO_V812_FRAME_SIZE_160    160   /* 10ms at 16kHz */
#define AUDIO_V812_FRAME_SIZE_320    320   /* 20ms at 16kHz */
#define AUDIO_V812_FRAME_SIZE_480    480   /* 30ms at 16kHz */
#define AUDIO_V812_FRAME_SIZE_1024   1024  /* Common frame size */

/**
 * @brief Error codes
 */
#define AUDIO_V812_SUCCESS           0
#define AUDIO_V812_ERROR_INVALID     -1
#define AUDIO_V812_ERROR_NOMEM       -2
#define AUDIO_V812_ERROR_TIMEOUT     -3
#define AUDIO_V812_ERROR_BUSY        -4
#define AUDIO_V812_ERROR_NOT_INIT    -5

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_V812_H__ */