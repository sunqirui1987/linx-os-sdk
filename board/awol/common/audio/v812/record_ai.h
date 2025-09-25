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

#ifndef _RECORD_AI_H_
#define _RECORD_AI_H_

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <plat_type.h>
#include <mm_common.h>
#include <media_common_aio.h>
#include <mpi_ai.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio recording configuration structure
 */
typedef struct {
    int sample_rate;        /**< Sample rate (e.g., 8000, 16000, 44100) */
    int channel_count;      /**< Number of channels (1 for mono, 2 for stereo) */
    int bit_width;          /**< Bit width (16, 24, 32) */
    int frame_size;         /**< Frame size in samples */
    int mic_num;            /**< Number of microphones */
    int ai_gain;            /**< Audio input gain */
    bool ans_enable;        /**< Audio noise suppression enable */
    int ans_mode;           /**< Audio noise suppression mode */
    bool agc_enable;        /**< Automatic gain control enable */
    float agc_target_db;    /**< AGC target dB */
    float agc_max_gain_db;  /**< AGC maximum gain dB */
} record_ai_config_t;

/**
 * @brief Audio recording context structure
 */
typedef struct {
    record_ai_config_t config;
    
    /* MPP system configuration */
    MPP_SYS_CONF_S sys_conf;
    AUDIO_DEV ai_dev;
    AI_CHN ai_chn;
    AI_CHN_ATTR_S ai_chn_attr;
    AIO_ATTR_S aio_attr;
    
    /* Recording state */
    bool is_recording;
    pthread_mutex_t mutex;
    
    /* Callback function for audio data */
    void (*data_callback)(const void* data, size_t size, void* user_data);
    void* user_data;
} record_ai_context_t;

/**
 * @brief Initialize audio recording context
 * 
 * @param ctx Pointer to recording context
 * @param config Pointer to recording configuration
 * @return 0 on success, negative error code on failure
 */
int record_ai_init(record_ai_context_t* ctx, const record_ai_config_t* config);

/**
 * @brief Start audio recording
 * 
 * @param ctx Pointer to recording context
 * @param data_callback Callback function to receive audio data
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int record_ai_start(record_ai_context_t* ctx, 
                   void (*data_callback)(const void* data, size_t size, void* user_data),
                   void* user_data);

/**
 * @brief Stop audio recording
 * 
 * @param ctx Pointer to recording context
 * @return 0 on success, negative error code on failure
 */
int record_ai_stop(record_ai_context_t* ctx);

/**
 * @brief Get one audio frame (blocking)
 * 
 * @param ctx Pointer to recording context
 * @param frame Pointer to audio frame structure
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return 0 on success, negative error code on failure
 */
int record_ai_get_frame(record_ai_context_t* ctx, AUDIO_FRAME_S* frame, int timeout_ms);

/**
 * @brief Release audio frame
 * 
 * @param ctx Pointer to recording context
 * @param frame Pointer to audio frame structure
 * @return 0 on success, negative error code on failure
 */
int record_ai_release_frame(record_ai_context_t* ctx, AUDIO_FRAME_S* frame);

/**
 * @brief Set audio input gain
 * 
 * @param ctx Pointer to recording context
 * @param gain Gain value
 * @return 0 on success, negative error code on failure
 */
int record_ai_set_gain(record_ai_context_t* ctx, int gain);

/**
 * @brief Get audio input gain
 * 
 * @param ctx Pointer to recording context
 * @param gain Pointer to store gain value
 * @return 0 on success, negative error code on failure
 */
int record_ai_get_gain(record_ai_context_t* ctx, int* gain);

/**
 * @brief Cleanup and destroy recording context
 * 
 * @param ctx Pointer to recording context
 * @return 0 on success, negative error code on failure
 */
int record_ai_destroy(record_ai_context_t* ctx);

/**
 * @brief Check if recording is active
 * 
 * @param ctx Pointer to recording context
 * @return true if recording, false otherwise
 */
bool record_ai_is_recording(const record_ai_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* _RECORD_AI_H_ */