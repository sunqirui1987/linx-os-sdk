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

#include "record_ai.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mpi_sys.h>
#include <utils/plat_log.h>

#define LOG_TAG "record_ai"

/**
 * @brief Configure AIO attributes based on recording configuration
 */
static void config_aio_attr_by_config(AIO_ATTR_S* dst, const record_ai_config_t* src)
{
    memset(dst, 0, sizeof(AIO_ATTR_S));
    
    dst->u32ChnCnt = src->channel_count;
    dst->enSamplerate = (AUDIO_SAMPLE_RATE_E)src->sample_rate;
    dst->enBitwidth = (AUDIO_BIT_WIDTH_E)src->bit_width;
    dst->enWorkmode = AIO_MODE_I2S_MASTER;
    dst->u32FrmNum = 4;
    dst->u32PtNumPerFrm = src->frame_size;
    dst->u32ChnCnt = src->channel_count;
    dst->u32ClkSel = 1;
    dst->enI2sType = AIO_I2STYPE_INNERCODEC;
    
    /* Set PCM format */
    if (src->bit_width == 8) {
        dst->enSoundmode = AUDIO_SOUND_MODE_MONO;
    } else if (src->bit_width == 16) {
        dst->enSoundmode = (src->channel_count == 1) ? AUDIO_SOUND_MODE_MONO : AUDIO_SOUND_MODE_STEREO;
    } else if (src->bit_width == 24 || src->bit_width == 32) {
        dst->enSoundmode = (src->channel_count == 1) ? AUDIO_SOUND_MODE_MONO : AUDIO_SOUND_MODE_STEREO;
    }
}

int record_ai_init(record_ai_context_t* ctx, const record_ai_config_t* config)
{
    if (!ctx || !config) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    memset(ctx, 0, sizeof(record_ai_context_t));
    memcpy(&ctx->config, config, sizeof(record_ai_config_t));
    
    /* Initialize mutex */
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        aloge("Failed to initialize mutex");
        return -1;
    }
    
    /* Initialize MPP system */
    memset(&ctx->sys_conf, 0, sizeof(MPP_SYS_CONF_S));
    ctx->sys_conf.nAlignWidth = 32;
    
    int ret = AW_MPI_SYS_SetConf(&ctx->sys_conf);
    if (ret != SUCCESS) {
        aloge("AW_MPI_SYS_SetConf failed: 0x%x", ret);
        goto error_cleanup;
    }
    
    ret = AW_MPI_SYS_Init();
    if (ret != SUCCESS) {
        aloge("AW_MPI_SYS_Init failed: 0x%x", ret);
        goto error_cleanup;
    }
    
    /* Configure AI device and channel */
    ctx->ai_dev = 0;
    ctx->ai_chn = 0;
    
    /* Configure AIO attributes */
    config_aio_attr_by_config(&ctx->aio_attr, config);
    
    /* Set AI channel attributes */
    memset(&ctx->ai_chn_attr, 0, sizeof(AI_CHN_ATTR_S));
    memcpy(&ctx->ai_chn_attr.stAioAttr, &ctx->aio_attr, sizeof(AIO_ATTR_S));
    
    /* Create AI device */
    ret = AW_MPI_AI_SetPubAttr(ctx->ai_dev, &ctx->aio_attr);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_SetPubAttr failed: 0x%x", ret);
        goto error_sys_exit;
    }
    
    ret = AW_MPI_AI_Enable(ctx->ai_dev);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_Enable failed: 0x%x", ret);
        goto error_sys_exit;
    }
    
    /* Create AI channel */
    ret = AW_MPI_AI_CreateChn(ctx->ai_dev, ctx->ai_chn, &ctx->ai_chn_attr);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_CreateChn failed: 0x%x", ret);
        goto error_ai_disable;
    }
    
    /* Set AI gain */
    if (config->ai_gain > 0) {
        ret = AW_MPI_AI_SetDevVolume(ctx->ai_dev, config->ai_gain);
        if (ret != SUCCESS) {
            alogw("AW_MPI_AI_SetDevVolume failed: 0x%x", ret);
        }
    }
    
    alogd("Record AI initialized successfully");
    return 0;
    
error_ai_disable:
    AW_MPI_AI_Disable(ctx->ai_dev);
error_sys_exit:
    AW_MPI_SYS_Exit();
error_cleanup:
    pthread_mutex_destroy(&ctx->mutex);
    return -1;
}

int record_ai_start(record_ai_context_t* ctx, 
                   void (*data_callback)(const void* data, size_t size, void* user_data),
                   void* user_data)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    if (ctx->is_recording) {
        pthread_mutex_unlock(&ctx->mutex);
        alogw("Recording already started");
        return 0;
    }
    
    /* Enable AI channel */
    int ret = AW_MPI_AI_EnableChn(ctx->ai_dev, ctx->ai_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_EnableChn failed: 0x%x", ret);
        pthread_mutex_unlock(&ctx->mutex);
        return -1;
    }
    
    ctx->data_callback = data_callback;
    ctx->user_data = user_data;
    ctx->is_recording = true;
    
    pthread_mutex_unlock(&ctx->mutex);
    
    alogd("Recording started");
    return 0;
}

int record_ai_stop(record_ai_context_t* ctx)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    if (!ctx->is_recording) {
        pthread_mutex_unlock(&ctx->mutex);
        alogw("Recording not started");
        return 0;
    }
    
    /* Disable AI channel */
    int ret = AW_MPI_AI_DisableChn(ctx->ai_dev, ctx->ai_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_DisableChn failed: 0x%x", ret);
    }
    
    ctx->is_recording = false;
    ctx->data_callback = NULL;
    ctx->user_data = NULL;
    
    pthread_mutex_unlock(&ctx->mutex);
    
    alogd("Recording stopped");
    return 0;
}

int record_ai_get_frame(record_ai_context_t* ctx, AUDIO_FRAME_S* frame, int timeout_ms)
{
    if (!ctx || !frame) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    if (!ctx->is_recording) {
        aloge("Recording not started");
        return -1;
    }
    
    /* Get audio frame from AI channel */
    int ret = AW_MPI_AI_GetFrame(ctx->ai_dev, ctx->ai_chn, frame, NULL, timeout_ms);
    if (ret != SUCCESS) {
        if (ret != ERR_AI_BUF_EMPTY) {
            aloge("AW_MPI_AI_GetFrame failed: 0x%x", ret);
        }
        return -1;
    }
    
    /* Call data callback if set */
    if (ctx->data_callback && frame->mpAddr && frame->mLen > 0) {
        ctx->data_callback(frame->mpAddr, frame->mLen, ctx->user_data);
    }
    
    return 0;
}

int record_ai_release_frame(record_ai_context_t* ctx, AUDIO_FRAME_S* frame)
{
    if (!ctx || !frame) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AI_ReleaseFrame(ctx->ai_dev, ctx->ai_chn, frame, NULL);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_ReleaseFrame failed: 0x%x", ret);
        return -1;
    }
    
    return 0;
}

int record_ai_set_gain(record_ai_context_t* ctx, int gain)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AI_SetDevVolume(ctx->ai_dev, gain);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_SetDevVolume failed: 0x%x", ret);
        return -1;
    }
    
    ctx->config.ai_gain = gain;
    return 0;
}

int record_ai_get_gain(record_ai_context_t* ctx, int* gain)
{
    if (!ctx || !gain) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AI_GetDevVolume(ctx->ai_dev, gain);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_GetDevVolume failed: 0x%x", ret);
        return -1;
    }
    
    return 0;
}

bool record_ai_is_recording(const record_ai_context_t* ctx)
{
    if (!ctx) {
        return false;
    }
    
    return ctx->is_recording;
}

int record_ai_destroy(record_ai_context_t* ctx)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    /* Stop recording if active */
    if (ctx->is_recording) {
        record_ai_stop(ctx);
    }
    
    /* Reset and destroy AI channel */
    int ret = AW_MPI_AI_ResetChn(ctx->ai_dev, ctx->ai_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_ResetChn failed: 0x%x", ret);
    }
    
    ret = AW_MPI_AI_DestroyChn(ctx->ai_dev, ctx->ai_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_DestroyChn failed: 0x%x", ret);
    }
    
    /* Disable AI device */
    ret = AW_MPI_AI_Disable(ctx->ai_dev);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AI_Disable failed: 0x%x", ret);
    }
    
    /* Exit MPP system */
    AW_MPI_SYS_Exit();
    
    /* Cleanup mutex */
    pthread_mutex_destroy(&ctx->mutex);
    
    /* Clear context */
    memset(ctx, 0, sizeof(record_ai_context_t));
    
    alogd("Record AI destroyed");
    return 0;
}