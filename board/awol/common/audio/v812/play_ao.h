/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PLAY_AO_H_
#define _PLAY_AO_H_

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <plat_type.h>
#include <mm_common.h>
#include <media_common_aio.h>
#include <mpi_ao.h>
#include <tsemaphore.h>
#include <cdx_list.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio playback configuration structure
 */
typedef struct {
    int sample_rate;        /**< Sample rate (e.g., 8000, 16000, 44100) */
    int channel_count;      /**< Number of channels (1 for mono, 2 for stereo) */
    int bit_width;          /**< Bit width (16, 24, 32) */
    int frame_size;         /**< Frame size in samples */
    int ao_volume;          /**< Audio output volume */
    int ao_soft_volume;     /**< Software volume control */
    bool save_data_flag;    /**< Flag to save output data */
} play_ao_config_t;

/**
 * @brief Audio frame node for frame management
 */
typedef struct play_ao_frame_node {
    AUDIO_FRAME_S audio_frame;
    struct list_head list;
} play_ao_frame_node_t;

/**
 * @brief Audio frame manager for buffering
 */
typedef struct {
    struct list_head idle_list;     /**< List of idle frames */
    struct list_head using_list;    /**< List of frames in use */
    int node_count;                 /**< Total number of frame nodes */
    pthread_mutex_t lock;           /**< Mutex for thread safety */
    
    /* Function pointers for frame management */
    AUDIO_FRAME_S* (*prefetch_first_idle_frame)(void* thiz);
    int (*use_frame)(void* thiz, AUDIO_FRAME_S* frame);
    int (*release_frame)(void* thiz, unsigned int frame_id);
} play_ao_frame_manager_t;

/**
 * @brief Audio playback context structure
 */
typedef struct {
    play_ao_config_t config;
    
    /* MPP system configuration */
    MPP_SYS_CONF_S sys_conf;
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    AIO_ATTR_S aio_attr;
    
    /* Frame management */
    play_ao_frame_manager_t frame_manager;
    pthread_mutex_t wait_frame_lock;
    bool wait_frame_flag;
    cdx_sem_t sem_frame_come;
    cdx_sem_t sem_eof_come;
    
    /* Playback state */
    bool is_playing;
    bool eof_flag;
    pthread_mutex_t mutex;
    
    /* Callback function for frame requests */
    int (*data_request_callback)(void* buffer, size_t size, void* user_data);
    void* user_data;
} play_ao_context_t;

/**
 * @brief Initialize audio playback context
 * 
 * @param ctx Pointer to playback context
 * @param config Pointer to playback configuration
 * @return 0 on success, negative error code on failure
 */
int play_ao_init(play_ao_context_t* ctx, const play_ao_config_t* config);

/**
 * @brief Start audio playback
 * 
 * @param ctx Pointer to playback context
 * @param data_request_callback Callback function to request audio data
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int play_ao_start(play_ao_context_t* ctx,
                 int (*data_request_callback)(void* buffer, size_t size, void* user_data),
                 void* user_data);

/**
 * @brief Stop audio playback
 * 
 * @param ctx Pointer to playback context
 * @return 0 on success, negative error code on failure
 */
int play_ao_stop(play_ao_context_t* ctx);

/**
 * @brief Send audio frame for playback (blocking)
 * 
 * @param ctx Pointer to playback context
 * @param frame Pointer to audio frame structure
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking)
 * @return 0 on success, negative error code on failure
 */
int play_ao_send_frame(play_ao_context_t* ctx, const AUDIO_FRAME_S* frame, int timeout_ms);

/**
 * @brief Set audio output volume
 * 
 * @param ctx Pointer to playback context
 * @param volume Volume value
 * @return 0 on success, negative error code on failure
 */
int play_ao_set_volume(play_ao_context_t* ctx, int volume);

/**
 * @brief Get audio output volume
 * 
 * @param ctx Pointer to playback context
 * @param volume Pointer to store volume value
 * @return 0 on success, negative error code on failure
 */
int play_ao_get_volume(play_ao_context_t* ctx, int* volume);

/**
 * @brief Set end of stream flag
 * 
 * @param ctx Pointer to playback context
 * @param eof_flag End of stream flag
 * @param immediate_flag Immediate flag
 * @return 0 on success, negative error code on failure
 */
int play_ao_set_eof(play_ao_context_t* ctx, bool eof_flag, bool immediate_flag);

/**
 * @brief Wait for playback to complete
 * 
 * @param ctx Pointer to playback context
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return 0 on success, negative error code on failure
 */
int play_ao_wait_eof(play_ao_context_t* ctx, int timeout_ms);

/**
 * @brief Cleanup and destroy playback context
 * 
 * @param ctx Pointer to playback context
 * @return 0 on success, negative error code on failure
 */
int play_ao_destroy(play_ao_context_t* ctx);

/**
 * @brief Check if playback is active
 * 
 * @param ctx Pointer to playback context
 * @return true if playing, false otherwise
 */
bool play_ao_is_playing(const play_ao_context_t* ctx);

/**
 * @brief Get an idle frame for filling with audio data
 * 
 * @param ctx Pointer to playback context
 * @return Pointer to idle frame, NULL if none available
 */
AUDIO_FRAME_S* play_ao_get_idle_frame(play_ao_context_t* ctx);

/**
 * @brief Submit a filled frame for playback
 * 
 * @param ctx Pointer to playback context
 * @param frame Pointer to filled audio frame
 * @return 0 on success, negative error code on failure
 */
int play_ao_submit_frame(play_ao_context_t* ctx, AUDIO_FRAME_S* frame);

#ifdef __cplusplus
}
#endif

#endif /* _PLAY_AO_H_ */