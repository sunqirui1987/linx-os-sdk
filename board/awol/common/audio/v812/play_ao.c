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

#include "play_ao.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <mpi_sys.h>
#include <utils/plat_log.h>

#define LOG_TAG "play_ao"

/* Default frame buffer count */
#define DEFAULT_FRAME_COUNT 8

/**
 * @brief Frame manager function implementations
 */
static AUDIO_FRAME_S* frame_manager_prefetch_first_idle_frame(void* thiz)
{
    play_ao_frame_manager_t* manager = (play_ao_frame_manager_t*)thiz;
    play_ao_frame_node_t* first_node;
    AUDIO_FRAME_S* frame_info;
    
    pthread_mutex_lock(&manager->lock);
    if (!list_empty(&manager->idle_list)) {
        first_node = list_first_entry(&manager->idle_list, play_ao_frame_node_t, list);
        frame_info = &first_node->audio_frame;
    } else {
        frame_info = NULL;
    }
    pthread_mutex_unlock(&manager->lock);
    
    return frame_info;
}

static int frame_manager_use_frame(void* thiz, AUDIO_FRAME_S* frame)
{
    int ret = 0;
    play_ao_frame_manager_t* manager = (play_ao_frame_manager_t*)thiz;
    
    if (!frame) {
        aloge("Invalid frame pointer");
        return -1;
    }
    
    pthread_mutex_lock(&manager->lock);
    play_ao_frame_node_t* first_node = list_first_entry_or_null(&manager->idle_list, 
                                                               play_ao_frame_node_t, list);
    if (first_node) {
        if (&first_node->audio_frame == frame) {
            list_move_tail(&first_node->list, &manager->using_list);
        } else {
            aloge("Frame mismatch: %p != %p", frame, &first_node->audio_frame);
            ret = -1;
        }
    } else {
        aloge("Idle list is empty");
        ret = -1;
    }
    pthread_mutex_unlock(&manager->lock);
    
    return ret;
}

static int frame_manager_release_frame(void* thiz, unsigned int frame_id)
{
    int ret = 0;
    play_ao_frame_manager_t* manager = (play_ao_frame_manager_t*)thiz;
    play_ao_frame_node_t* node;
    bool found = false;
    
    pthread_mutex_lock(&manager->lock);
    list_for_each_entry(node, &manager->using_list, list) {
        if (node->audio_frame.mId == frame_id) {
            list_move_tail(&node->list, &manager->idle_list);
            found = true;
            break;
        }
    }
    
    if (!found) {
        aloge("Frame ID %u not found in using list", frame_id);
        ret = -1;
    }
    pthread_mutex_unlock(&manager->lock);
    
    return ret;
}

/**
 * @brief Initialize frame manager
 */
static int init_frame_manager(play_ao_frame_manager_t* manager, int frame_count, 
                             const play_ao_config_t* config, int buffer_size)
{
    if (!manager || !config) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    memset(manager, 0, sizeof(play_ao_frame_manager_t));
    
    INIT_LIST_HEAD(&manager->idle_list);
    INIT_LIST_HEAD(&manager->using_list);
    
    if (pthread_mutex_init(&manager->lock, NULL) != 0) {
        aloge("Failed to initialize mutex");
        return -1;
    }
    
    /* Allocate frame nodes */
    for (int i = 0; i < frame_count; i++) {
        play_ao_frame_node_t* node = malloc(sizeof(play_ao_frame_node_t));
        if (!node) {
            aloge("Failed to allocate frame node");
            goto error_cleanup;
        }
        
        memset(node, 0, sizeof(play_ao_frame_node_t));
        
        /* Allocate buffer for audio data */
        node->audio_frame.mpAddr = malloc(buffer_size);
        if (!node->audio_frame.mpAddr) {
            aloge("Failed to allocate frame buffer");
            free(node);
            goto error_cleanup;
        }
        
        node->audio_frame.mLen = buffer_size;
        node->audio_frame.mId = i;
        
        list_add_tail(&node->list, &manager->idle_list);
        manager->node_count++;
    }
    
    /* Set function pointers */
    manager->prefetch_first_idle_frame = frame_manager_prefetch_first_idle_frame;
    manager->use_frame = frame_manager_use_frame;
    manager->release_frame = frame_manager_release_frame;
    
    return 0;
    
error_cleanup:
    /* Cleanup allocated nodes */
    play_ao_frame_node_t* node, *tmp;
    list_for_each_entry_safe(node, tmp, &manager->idle_list, list) {
        list_del(&node->list);
        if (node->audio_frame.mpAddr) {
            free(node->audio_frame.mpAddr);
        }
        free(node);
    }
    pthread_mutex_destroy(&manager->lock);
    return -1;
}

/**
 * @brief Destroy frame manager
 */
static int destroy_frame_manager(play_ao_frame_manager_t* manager)
{
    if (!manager) {
        return -EINVAL;
    }
    
    pthread_mutex_lock(&manager->lock);
    
    /* Free all frame nodes */
    play_ao_frame_node_t* node, *tmp;
    list_for_each_entry_safe(node, tmp, &manager->idle_list, list) {
        list_del(&node->list);
        if (node->audio_frame.mpAddr) {
            free(node->audio_frame.mpAddr);
        }
        free(node);
    }
    
    list_for_each_entry_safe(node, tmp, &manager->using_list, list) {
        list_del(&node->list);
        if (node->audio_frame.mpAddr) {
            free(node->audio_frame.mpAddr);
        }
        free(node);
    }
    
    pthread_mutex_unlock(&manager->lock);
    pthread_mutex_destroy(&manager->lock);
    
    memset(manager, 0, sizeof(play_ao_frame_manager_t));
    return 0;
}

/**
 * @brief Configure AIO attributes based on playback configuration
 */
static void config_aio_attr_by_config(AIO_ATTR_S* dst, const play_ao_config_t* src)
{
    memset(dst, 0, sizeof(AIO_ATTR_S));
    
    dst->u32ChnCnt = src->channel_count;
    dst->enSamplerate = (AUDIO_SAMPLE_RATE_E)src->sample_rate;
    dst->enBitwidth = (AUDIO_BIT_WIDTH_E)src->bit_width;
    dst->enWorkmode = AIO_MODE_I2S_MASTER;
    dst->u32FrmNum = 4;
    dst->u32PtNumPerFrm = src->frame_size;
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

/**
 * @brief AO callback wrapper
 */
static ERRORTYPE ao_callback_wrapper(void* cookie, MPP_CHN_S* chn, MPP_EVENT_TYPE event, void* event_data)
{
    play_ao_context_t* ctx = (play_ao_context_t*)cookie;
    
    switch (event) {
        case MPP_EVENT_RELEASE_AUDIO_BUFFER: {
            AUDIO_FRAME_S* frame = (AUDIO_FRAME_S*)event_data;
            if (frame) {
                ctx->frame_manager.release_frame(&ctx->frame_manager, frame->mId);
                
                pthread_mutex_lock(&ctx->wait_frame_lock);
                if (ctx->wait_frame_flag) {
                    ctx->wait_frame_flag = false;
                    cdx_sem_up(&ctx->sem_frame_come);
                }
                pthread_mutex_unlock(&ctx->wait_frame_lock);
            }
            break;
        }
        case MPP_EVENT_NOTIFY_EOF: {
            alogd("Received EOF event");
            cdx_sem_up(&ctx->sem_eof_come);
            break;
        }
        default:
            break;
    }
    
    return SUCCESS;
}

int play_ao_init(play_ao_context_t* ctx, const play_ao_config_t* config)
{
    if (!ctx || !config) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    memset(ctx, 0, sizeof(play_ao_context_t));
    memcpy(&ctx->config, config, sizeof(play_ao_config_t));
    
    /* Initialize mutexes and semaphores */
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        aloge("Failed to initialize mutex");
        return -1;
    }
    
    if (pthread_mutex_init(&ctx->wait_frame_lock, NULL) != 0) {
        aloge("Failed to initialize wait frame mutex");
        goto error_cleanup_mutex;
    }
    
    if (cdx_sem_init(&ctx->sem_frame_come, 0) != 0) {
        aloge("Failed to initialize frame semaphore");
        goto error_cleanup_wait_mutex;
    }
    
    if (cdx_sem_init(&ctx->sem_eof_come, 0) != 0) {
        aloge("Failed to initialize EOF semaphore");
        goto error_cleanup_frame_sem;
    }
    
    /* Initialize MPP system */
    memset(&ctx->sys_conf, 0, sizeof(MPP_SYS_CONF_S));
    ctx->sys_conf.nAlignWidth = 32;
    
    int ret = AW_MPI_SYS_SetConf(&ctx->sys_conf);
    if (ret != SUCCESS) {
        aloge("AW_MPI_SYS_SetConf failed: 0x%x", ret);
        goto error_cleanup_eof_sem;
    }
    
    ret = AW_MPI_SYS_Init();
    if (ret != SUCCESS) {
        aloge("AW_MPI_SYS_Init failed: 0x%x", ret);
        goto error_cleanup_eof_sem;
    }
    
    /* Configure AO device and channel */
    ctx->ao_dev = 0;
    ctx->ao_chn = 0;
    
    /* Configure AIO attributes */
    config_aio_attr_by_config(&ctx->aio_attr, config);
    
    /* Create AO device */
    ret = AW_MPI_AO_SetPubAttr(ctx->ao_dev, &ctx->aio_attr);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_SetPubAttr failed: 0x%x", ret);
        goto error_sys_exit;
    }
    
    ret = AW_MPI_AO_Enable(ctx->ao_dev);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_Enable failed: 0x%x", ret);
        goto error_sys_exit;
    }
    
    /* Create AO channel */
    ret = AW_MPI_AO_CreateChn(ctx->ao_dev, ctx->ao_chn, &ctx->aio_attr);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_CreateChn failed: 0x%x", ret);
        goto error_ao_disable;
    }
    
    /* Set callback */
    MPP_CHN_S ao_chn = {MOD_ID_AO, ctx->ao_dev, ctx->ao_chn};
    ret = AW_MPI_SYS_RegisterEventHandler(&ao_chn, ao_callback_wrapper, ctx);
    if (ret != SUCCESS) {
        aloge("AW_MPI_SYS_RegisterEventHandler failed: 0x%x", ret);
        goto error_ao_destroy;
    }
    
    /* Initialize frame manager */
    int buffer_size = config->frame_size * config->channel_count * (config->bit_width / 8);
    ret = init_frame_manager(&ctx->frame_manager, DEFAULT_FRAME_COUNT, config, buffer_size);
    if (ret != 0) {
        aloge("Failed to initialize frame manager");
        goto error_ao_destroy;
    }
    
    /* Set AO volume */
    if (config->ao_volume >= 0) {
        ret = AW_MPI_AO_SetDevVolume(ctx->ao_dev, config->ao_volume);
        if (ret != SUCCESS) {
            alogw("AW_MPI_AO_SetDevVolume failed: 0x%x", ret);
        }
    }
    
    alogd("Play AO initialized successfully");
    return 0;
    
error_ao_destroy:
    AW_MPI_AO_DestroyChn(ctx->ao_dev, ctx->ao_chn);
error_ao_disable:
    AW_MPI_AO_Disable(ctx->ao_dev);
error_sys_exit:
    AW_MPI_SYS_Exit();
error_cleanup_eof_sem:
    cdx_sem_deinit(&ctx->sem_eof_come);
error_cleanup_frame_sem:
    cdx_sem_deinit(&ctx->sem_frame_come);
error_cleanup_wait_mutex:
    pthread_mutex_destroy(&ctx->wait_frame_lock);
error_cleanup_mutex:
    pthread_mutex_destroy(&ctx->mutex);
    return -1;
}

int play_ao_start(play_ao_context_t* ctx,
                 int (*data_request_callback)(void* buffer, size_t size, void* user_data),
                 void* user_data)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    if (ctx->is_playing) {
        pthread_mutex_unlock(&ctx->mutex);
        alogw("Playback already started");
        return 0;
    }
    
    /* Start AO channel */
    int ret = AW_MPI_AO_StartChn(ctx->ao_dev, ctx->ao_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_StartChn failed: 0x%x", ret);
        pthread_mutex_unlock(&ctx->mutex);
        return -1;
    }
    
    ctx->data_request_callback = data_request_callback;
    ctx->user_data = user_data;
    ctx->is_playing = true;
    ctx->eof_flag = false;
    
    pthread_mutex_unlock(&ctx->mutex);
    
    alogd("Playback started");
    return 0;
}

int play_ao_stop(play_ao_context_t* ctx)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    if (!ctx->is_playing) {
        pthread_mutex_unlock(&ctx->mutex);
        alogw("Playback not started");
        return 0;
    }
    
    /* Stop AO channel */
    int ret = AW_MPI_AO_StopChn(ctx->ao_dev, ctx->ao_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_StopChn failed: 0x%x", ret);
    }
    
    ctx->is_playing = false;
    ctx->data_request_callback = NULL;
    ctx->user_data = NULL;
    
    pthread_mutex_unlock(&ctx->mutex);
    
    alogd("Playback stopped");
    return 0;
}

int play_ao_send_frame(play_ao_context_t* ctx, const AUDIO_FRAME_S* frame, int timeout_ms)
{
    if (!ctx || !frame) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    if (!ctx->is_playing) {
        aloge("Playback not started");
        return -1;
    }
    
    /* Send audio frame to AO channel */
    int ret = AW_MPI_AO_SendFrame(ctx->ao_dev, ctx->ao_chn, frame, timeout_ms);
    if (ret != SUCCESS) {
        if (ret != ERR_AO_BUF_FULL) {
            aloge("AW_MPI_AO_SendFrame failed: 0x%x", ret);
        }
        return -1;
    }
    
    return 0;
}

AUDIO_FRAME_S* play_ao_get_idle_frame(play_ao_context_t* ctx)
{
    if (!ctx) {
        aloge("Invalid context");
        return NULL;
    }
    
    pthread_mutex_lock(&ctx->wait_frame_lock);
    AUDIO_FRAME_S* frame = ctx->frame_manager.prefetch_first_idle_frame(&ctx->frame_manager);
    if (!frame) {
        ctx->wait_frame_flag = true;
        pthread_mutex_unlock(&ctx->wait_frame_lock);
        
        /* Wait for frame to become available */
        if (cdx_sem_down_timedwait(&ctx->sem_frame_come, 500) == 0) {
            frame = ctx->frame_manager.prefetch_first_idle_frame(&ctx->frame_manager);
        }
    } else {
        pthread_mutex_unlock(&ctx->wait_frame_lock);
    }
    
    return frame;
}

int play_ao_submit_frame(play_ao_context_t* ctx, AUDIO_FRAME_S* frame)
{
    if (!ctx || !frame) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    /* Mark frame as in use */
    int ret = ctx->frame_manager.use_frame(&ctx->frame_manager, frame);
    if (ret != 0) {
        aloge("Failed to mark frame as in use");
        return -1;
    }
    
    /* Send frame for playback */
    ret = play_ao_send_frame(ctx, frame, 0);
    if (ret != 0) {
        /* Release frame if send failed */
        ctx->frame_manager.release_frame(&ctx->frame_manager, frame->mId);
        return -1;
    }
    
    return 0;
}

int play_ao_set_volume(play_ao_context_t* ctx, int volume)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AO_SetDevVolume(ctx->ao_dev, volume);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_SetDevVolume failed: 0x%x", ret);
        return -1;
    }
    
    ctx->config.ao_volume = volume;
    return 0;
}

int play_ao_get_volume(play_ao_context_t* ctx, int* volume)
{
    if (!ctx || !volume) {
        aloge("Invalid parameters");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AO_GetDevVolume(ctx->ao_dev, volume);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_GetDevVolume failed: 0x%x", ret);
        return -1;
    }
    
    return 0;
}

int play_ao_set_eof(play_ao_context_t* ctx, bool eof_flag, bool immediate_flag)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    int ret = AW_MPI_AO_SetStreamEof(ctx->ao_dev, ctx->ao_chn, eof_flag ? 1 : 0, immediate_flag ? 1 : 0);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_SetStreamEof failed: 0x%x", ret);
        return -1;
    }
    
    ctx->eof_flag = eof_flag;
    return 0;
}

int play_ao_wait_eof(play_ao_context_t* ctx, int timeout_ms)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    if (timeout_ms < 0) {
        cdx_sem_down(&ctx->sem_eof_come);
    } else {
        if (cdx_sem_down_timedwait(&ctx->sem_eof_come, timeout_ms) != 0) {
            return -1; /* Timeout */
        }
    }
    
    return 0;
}

bool play_ao_is_playing(const play_ao_context_t* ctx)
{
    if (!ctx) {
        return false;
    }
    
    return ctx->is_playing;
}

int play_ao_destroy(play_ao_context_t* ctx)
{
    if (!ctx) {
        aloge("Invalid context");
        return -EINVAL;
    }
    
    /* Stop playback if active */
    if (ctx->is_playing) {
        play_ao_stop(ctx);
    }
    
    /* Destroy frame manager */
    destroy_frame_manager(&ctx->frame_manager);
    
    /* Destroy AO channel */
    int ret = AW_MPI_AO_DestroyChn(ctx->ao_dev, ctx->ao_chn);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_DestroyChn failed: 0x%x", ret);
    }
    
    /* Disable AO device */
    ret = AW_MPI_AO_Disable(ctx->ao_dev);
    if (ret != SUCCESS) {
        aloge("AW_MPI_AO_Disable failed: 0x%x", ret);
    }
    
    /* Exit MPP system */
    AW_MPI_SYS_Exit();
    
    /* Cleanup synchronization objects */
    cdx_sem_deinit(&ctx->sem_eof_come);
    cdx_sem_deinit(&ctx->sem_frame_come);
    pthread_mutex_destroy(&ctx->wait_frame_lock);
    pthread_mutex_destroy(&ctx->mutex);
    
    /* Clear context */
    memset(ctx, 0, sizeof(play_ao_context_t));
    
    alogd("Play AO destroyed");
    return 0;
}