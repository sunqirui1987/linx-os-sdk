/**
 * @file audio_pipeline.c
 * @brief 音频管道实现
 * @details 提供音频处理管道的创建、管理和执行功能
 */

#include "audio_pipeline.h"
#include "../core/types.h"
#include "../plugins/plugin_interface.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 管道节点实现
 */
typedef struct pipeline_node {
    linx_pipeline_node_type_t type;
    linx_pipeline_node_state_t state;
    
    // 节点数据
    union {
        struct {
            linx_plugin_base_t* plugin;
            linx_plugin_config_t config;
        } plugin_node;
        
        struct {
            linx_audio_buffer_t* buffer;
            size_t buffer_size;
        } buffer_node;
        
        struct {
            linx_audio_format_info_t format;
        } format_node;
    } data;
    
    // 连接信息
    struct pipeline_node** inputs;
    size_t input_count;
    size_t input_capacity;
    
    struct pipeline_node** outputs;
    size_t output_count;
    size_t output_capacity;
    
    // 处理缓冲区
    linx_audio_buffer_t* input_buffer;
    linx_audio_buffer_t* output_buffer;
    
    // 节点ID和名称
    uint32_t id;
    char name[64];
    
    // 统计信息
    uint64_t samples_processed;
    uint64_t processing_time_us;
    
    // 下一个节点（链表）
    struct pipeline_node* next;
} pipeline_node_impl_t;

/**
 * @brief 音频管道实现
 */
typedef struct {
    // 基础信息
    uint32_t id;
    char name[64];
    linx_pipeline_state_t state;
    
    // 节点管理
    pipeline_node_impl_t* nodes;
    size_t node_count;
    uint32_t next_node_id;
    
    // 音频格式
    linx_audio_format_info_t format;
    
    // 处理配置
    linx_audio_pipeline_config_t config;
    
    // 线程同步
    pthread_mutex_t mutex;
    pthread_cond_t state_cond;
    
    // 处理线程
    pthread_t processing_thread;
    bool thread_running;
    
    // 事件回调
    linx_audio_event_callback_t event_callback;
    void* event_callback_data;
    
    // 统计信息
    uint64_t frames_processed;
    uint64_t total_processing_time_us;
    uint32_t underruns;
    uint32_t overruns;
    
    // 内存管理
    linx_audio_buffer_t* temp_buffers;
    size_t temp_buffer_count;
} audio_pipeline_impl_t;

// ============================================================================
// 内部函数声明
// ============================================================================

static pipeline_node_impl_t* create_node(linx_pipeline_node_type_t type, const char* name);
static void destroy_node(pipeline_node_impl_t* node);
static linx_audio_result_t connect_nodes(pipeline_node_impl_t* source, pipeline_node_impl_t* dest);
static linx_audio_result_t disconnect_nodes(pipeline_node_impl_t* source, pipeline_node_impl_t* dest);
static linx_audio_result_t process_node(pipeline_node_impl_t* node);
static linx_audio_result_t process_pipeline_graph(audio_pipeline_impl_t* pipeline);
static void* processing_thread_func(void* arg);
static linx_audio_result_t allocate_temp_buffers(audio_pipeline_impl_t* pipeline);
static void free_temp_buffers(audio_pipeline_impl_t* pipeline);

// ============================================================================
// 管道创建和销毁
// ============================================================================

linx_audio_pipeline_t* linx_audio_pipeline_create(const linx_audio_pipeline_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    audio_pipeline_impl_t* pipeline = malloc(sizeof(audio_pipeline_impl_t));
    if (!pipeline) {
        return NULL;
    }
    
    memset(pipeline, 0, sizeof(audio_pipeline_impl_t));
    
    // 初始化基础信息
    pipeline->id = config->id;
    strncpy(pipeline->name, config->name ? config->name : "Unnamed Pipeline", 
            sizeof(pipeline->name) - 1);
    pipeline->state = LINX_PIPELINE_STATE_STOPPED;
    pipeline->next_node_id = 1;
    
    // 复制配置
    pipeline->config = *config;
    
    // 初始化音频格式
    pipeline->format = config->format;
    
    // 初始化线程同步
    if (pthread_mutex_init(&pipeline->mutex, NULL) != 0) {
        free(pipeline);
        return NULL;
    }
    
    if (pthread_cond_init(&pipeline->state_cond, NULL) != 0) {
        pthread_mutex_destroy(&pipeline->mutex);
        free(pipeline);
        return NULL;
    }
    
    // 分配临时缓冲区
    if (allocate_temp_buffers(pipeline) != LINX_AUDIO_SUCCESS) {
        pthread_cond_destroy(&pipeline->state_cond);
        pthread_mutex_destroy(&pipeline->mutex);
        free(pipeline);
        return NULL;
    }
    
    return (linx_audio_pipeline_t*)pipeline;
}

void linx_audio_pipeline_destroy(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    // 停止管道
    linx_audio_pipeline_stop(pipeline);
    
    // 等待处理线程结束
    if (impl->thread_running) {
        impl->thread_running = false;
        pthread_join(impl->processing_thread, NULL);
    }
    
    // 销毁所有节点
    pipeline_node_impl_t* node = impl->nodes;
    while (node) {
        pipeline_node_impl_t* next = node->next;
        destroy_node(node);
        node = next;
    }
    
    // 释放临时缓冲区
    free_temp_buffers(impl);
    
    // 清理线程同步对象
    pthread_cond_destroy(&impl->state_cond);
    pthread_mutex_destroy(&impl->mutex);
    
    free(impl);
}

// ============================================================================
// 管道控制
// ============================================================================

linx_audio_result_t linx_audio_pipeline_start(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    if (impl->state == LINX_PIPELINE_STATE_RUNNING) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    // 初始化所有节点
    pipeline_node_impl_t* node = impl->nodes;
    while (node) {
        if (node->type == LINX_PIPELINE_NODE_TYPE_PLUGIN && node->data.plugin_node.plugin) {
            linx_audio_result_t result = node->data.plugin_node.plugin->vtable->start(
                node->data.plugin_node.plugin);
            if (result != LINX_AUDIO_SUCCESS) {
                pthread_mutex_unlock(&impl->mutex);
                return result;
            }
        }
        node->state = LINX_PIPELINE_NODE_STATE_RUNNING;
        node = node->next;
    }
    
    // 启动处理线程
    impl->thread_running = true;
    if (pthread_create(&impl->processing_thread, NULL, processing_thread_func, impl) != 0) {
        impl->thread_running = false;
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_THREAD_ERROR;
    }
    
    impl->state = LINX_PIPELINE_STATE_RUNNING;
    pthread_cond_broadcast(&impl->state_cond);
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_pipeline_stop(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    if (impl->state == LINX_PIPELINE_STATE_STOPPED) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_SUCCESS;
    }
    
    // 停止处理线程
    impl->thread_running = false;
    impl->state = LINX_PIPELINE_STATE_STOPPING;
    pthread_cond_broadcast(&impl->state_cond);
    pthread_mutex_unlock(&impl->mutex);
    
    // 等待线程结束
    if (impl->processing_thread) {
        pthread_join(impl->processing_thread, NULL);
        impl->processing_thread = 0;
    }
    
    pthread_mutex_lock(&impl->mutex);
    
    // 停止所有节点
    pipeline_node_impl_t* node = impl->nodes;
    while (node) {
        if (node->type == LINX_PIPELINE_NODE_TYPE_PLUGIN && node->data.plugin_node.plugin) {
            node->data.plugin_node.plugin->vtable->stop(node->data.plugin_node.plugin);
        }
        node->state = LINX_PIPELINE_NODE_STATE_STOPPED;
        node = node->next;
    }
    
    impl->state = LINX_PIPELINE_STATE_STOPPED;
    pthread_cond_broadcast(&impl->state_cond);
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_pipeline_pause(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    if (impl->state != LINX_PIPELINE_STATE_RUNNING) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    impl->state = LINX_PIPELINE_STATE_PAUSED;
    pthread_cond_broadcast(&impl->state_cond);
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_pipeline_resume(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    if (impl->state != LINX_PIPELINE_STATE_PAUSED) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    impl->state = LINX_PIPELINE_STATE_RUNNING;
    pthread_cond_broadcast(&impl->state_cond);
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 节点管理
// ============================================================================

uint32_t linx_audio_pipeline_add_plugin_node(linx_audio_pipeline_t* pipeline,
                                             linx_plugin_base_t* plugin,
                                             const linx_plugin_config_t* config,
                                             const char* name) {
    if (!pipeline || !plugin) {
        return 0;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    pipeline_node_impl_t* node = create_node(LINX_PIPELINE_NODE_TYPE_PLUGIN, name);
    if (!node) {
        pthread_mutex_unlock(&impl->mutex);
        return 0;
    }
    
    node->id = impl->next_node_id++;
    node->data.plugin_node.plugin = plugin;
    if (config) {
        node->data.plugin_node.config = *config;
    }
    
    // 添加到链表
    node->next = impl->nodes;
    impl->nodes = node;
    impl->node_count++;
    
    pthread_mutex_unlock(&impl->mutex);
    
    return node->id;
}

uint32_t linx_audio_pipeline_add_buffer_node(linx_audio_pipeline_t* pipeline,
                                             size_t buffer_size,
                                             const char* name) {
    if (!pipeline || buffer_size == 0) {
        return 0;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    pipeline_node_impl_t* node = create_node(LINX_PIPELINE_NODE_TYPE_BUFFER, name);
    if (!node) {
        pthread_mutex_unlock(&impl->mutex);
        return 0;
    }
    
    node->id = impl->next_node_id++;
    node->data.buffer_node.buffer_size = buffer_size;
    
    // 分配缓冲区
    node->data.buffer_node.buffer = malloc(sizeof(linx_audio_buffer_t));
    if (!node->data.buffer_node.buffer) {
        destroy_node(node);
        pthread_mutex_unlock(&impl->mutex);
        return 0;
    }
    
    node->data.buffer_node.buffer->data = malloc(buffer_size);
    if (!node->data.buffer_node.buffer->data) {
        free(node->data.buffer_node.buffer);
        destroy_node(node);
        pthread_mutex_unlock(&impl->mutex);
        return 0;
    }
    
    node->data.buffer_node.buffer->size = buffer_size;
    node->data.buffer_node.buffer->frame_count = 0;
    node->data.buffer_node.buffer->channels = impl->format.channels;
    
    // 添加到链表
    node->next = impl->nodes;
    impl->nodes = node;
    impl->node_count++;
    
    pthread_mutex_unlock(&impl->mutex);
    
    return node->id;
}

linx_audio_result_t linx_audio_pipeline_remove_node(linx_audio_pipeline_t* pipeline, uint32_t node_id) {
    if (!pipeline || node_id == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    // 查找节点
    pipeline_node_impl_t* prev = NULL;
    pipeline_node_impl_t* node = impl->nodes;
    
    while (node && node->id != node_id) {
        prev = node;
        node = node->next;
    }
    
    if (!node) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_NOT_FOUND;
    }
    
    // 断开所有连接
    for (size_t i = 0; i < node->input_count; i++) {
        disconnect_nodes(node->inputs[i], node);
    }
    for (size_t i = 0; i < node->output_count; i++) {
        disconnect_nodes(node, node->outputs[i]);
    }
    
    // 从链表中移除
    if (prev) {
        prev->next = node->next;
    } else {
        impl->nodes = node->next;
    }
    impl->node_count--;
    
    // 销毁节点
    destroy_node(node);
    
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

linx_audio_result_t linx_audio_pipeline_connect_nodes(linx_audio_pipeline_t* pipeline,
                                                      uint32_t source_id,
                                                      uint32_t dest_id) {
    if (!pipeline || source_id == 0 || dest_id == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    // 查找源节点和目标节点
    pipeline_node_impl_t* source = NULL;
    pipeline_node_impl_t* dest = NULL;
    pipeline_node_impl_t* node = impl->nodes;
    
    while (node) {
        if (node->id == source_id) {
            source = node;
        }
        if (node->id == dest_id) {
            dest = node;
        }
        node = node->next;
    }
    
    if (!source || !dest) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_NOT_FOUND;
    }
    
    linx_audio_result_t result = connect_nodes(source, dest);
    pthread_mutex_unlock(&impl->mutex);
    
    return result;
}

linx_audio_result_t linx_audio_pipeline_disconnect_nodes(linx_audio_pipeline_t* pipeline,
                                                         uint32_t source_id,
                                                         uint32_t dest_id) {
    if (!pipeline || source_id == 0 || dest_id == 0) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    // 查找源节点和目标节点
    pipeline_node_impl_t* source = NULL;
    pipeline_node_impl_t* dest = NULL;
    pipeline_node_impl_t* node = impl->nodes;
    
    while (node) {
        if (node->id == source_id) {
            source = node;
        }
        if (node->id == dest_id) {
            dest = node;
        }
        node = node->next;
    }
    
    if (!source || !dest) {
        pthread_mutex_unlock(&impl->mutex);
        return LINX_AUDIO_ERROR_NOT_FOUND;
    }
    
    linx_audio_result_t result = disconnect_nodes(source, dest);
    pthread_mutex_unlock(&impl->mutex);
    
    return result;
}

// ============================================================================
// 状态查询
// ============================================================================

linx_pipeline_state_t linx_audio_pipeline_get_state(linx_audio_pipeline_t* pipeline) {
    if (!pipeline) {
        return LINX_PIPELINE_STATE_ERROR;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    linx_pipeline_state_t state = impl->state;
    pthread_mutex_unlock(&impl->mutex);
    
    return state;
}

linx_audio_result_t linx_audio_pipeline_get_stats(linx_audio_pipeline_t* pipeline,
                                                  linx_pipeline_stats_t* stats) {
    if (!pipeline || !stats) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    audio_pipeline_impl_t* impl = (audio_pipeline_impl_t*)pipeline;
    
    pthread_mutex_lock(&impl->mutex);
    
    memset(stats, 0, sizeof(linx_pipeline_stats_t));
    stats->node_count = impl->node_count;
    stats->frames_processed = impl->frames_processed;
    stats->total_processing_time_us = impl->total_processing_time_us;
    stats->underruns = impl->underruns;
    stats->overruns = impl->overruns;
    
    if (impl->frames_processed > 0) {
        stats->average_processing_time_us = impl->total_processing_time_us / impl->frames_processed;
    }
    
    pthread_mutex_unlock(&impl->mutex);
    
    return LINX_AUDIO_SUCCESS;
}

// ============================================================================
// 内部函数实现
// ============================================================================

static pipeline_node_impl_t* create_node(linx_pipeline_node_type_t type, const char* name) {
    pipeline_node_impl_t* node = malloc(sizeof(pipeline_node_impl_t));
    if (!node) {
        return NULL;
    }
    
    memset(node, 0, sizeof(pipeline_node_impl_t));
    
    node->type = type;
    node->state = LINX_PIPELINE_NODE_STATE_STOPPED;
    
    if (name) {
        strncpy(node->name, name, sizeof(node->name) - 1);
    } else {
        snprintf(node->name, sizeof(node->name), "Node_%d", (int)type);
    }
    
    return node;
}

static void destroy_node(pipeline_node_impl_t* node) {
    if (!node) {
        return;
    }
    
    // 释放连接数组
    if (node->inputs) {
        free(node->inputs);
    }
    if (node->outputs) {
        free(node->outputs);
    }
    
    // 释放缓冲区
    if (node->input_buffer) {
        if (node->input_buffer->data) {
            free(node->input_buffer->data);
        }
        free(node->input_buffer);
    }
    if (node->output_buffer) {
        if (node->output_buffer->data) {
            free(node->output_buffer->data);
        }
        free(node->output_buffer);
    }
    
    // 释放节点特定数据
    if (node->type == LINX_PIPELINE_NODE_TYPE_BUFFER && node->data.buffer_node.buffer) {
        if (node->data.buffer_node.buffer->data) {
            free(node->data.buffer_node.buffer->data);
        }
        free(node->data.buffer_node.buffer);
    }
    
    free(node);
}

static linx_audio_result_t connect_nodes(pipeline_node_impl_t* source, pipeline_node_impl_t* dest) {
    if (!source || !dest) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 检查是否已经连接
    for (size_t i = 0; i < source->output_count; i++) {
        if (source->outputs[i] == dest) {
            return LINX_AUDIO_SUCCESS; // 已经连接
        }
    }
    
    // 扩展输出数组
    if (source->output_count >= source->output_capacity) {
        size_t new_capacity = source->output_capacity == 0 ? 4 : source->output_capacity * 2;
        pipeline_node_impl_t** new_outputs = realloc(source->outputs, 
                                                     new_capacity * sizeof(pipeline_node_impl_t*));
        if (!new_outputs) {
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        source->outputs = new_outputs;
        source->output_capacity = new_capacity;
    }
    
    // 扩展输入数组
    if (dest->input_count >= dest->input_capacity) {
        size_t new_capacity = dest->input_capacity == 0 ? 4 : dest->input_capacity * 2;
        pipeline_node_impl_t** new_inputs = realloc(dest->inputs, 
                                                    new_capacity * sizeof(pipeline_node_impl_t*));
        if (!new_inputs) {
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        dest->inputs = new_inputs;
        dest->input_capacity = new_capacity;
    }
    
    // 添加连接
    source->outputs[source->output_count++] = dest;
    dest->inputs[dest->input_count++] = source;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t disconnect_nodes(pipeline_node_impl_t* source, pipeline_node_impl_t* dest) {
    if (!source || !dest) {
        return LINX_AUDIO_ERROR_INVALID_PARAM;
    }
    
    // 从源节点的输出列表中移除
    for (size_t i = 0; i < source->output_count; i++) {
        if (source->outputs[i] == dest) {
            // 移动后续元素
            for (size_t j = i; j < source->output_count - 1; j++) {
                source->outputs[j] = source->outputs[j + 1];
            }
            source->output_count--;
            break;
        }
    }
    
    // 从目标节点的输入列表中移除
    for (size_t i = 0; i < dest->input_count; i++) {
        if (dest->inputs[i] == source) {
            // 移动后续元素
            for (size_t j = i; j < dest->input_count - 1; j++) {
                dest->inputs[j] = dest->inputs[j + 1];
            }
            dest->input_count--;
            break;
        }
    }
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t process_node(pipeline_node_impl_t* node) {
    if (!node || node->state != LINX_PIPELINE_NODE_STATE_RUNNING) {
        return LINX_AUDIO_ERROR_INVALID_STATE;
    }
    
    switch (node->type) {
        case LINX_PIPELINE_NODE_TYPE_PLUGIN:
            if (node->data.plugin_node.plugin && node->data.plugin_node.plugin->vtable->process) {
                return node->data.plugin_node.plugin->vtable->process(
                    node->data.plugin_node.plugin,
                    node->input_buffer,
                    node->output_buffer);
            }
            break;
            
        case LINX_PIPELINE_NODE_TYPE_BUFFER:
            // 缓冲区节点只是复制数据
            if (node->input_buffer && node->output_buffer) {
                size_t copy_size = node->input_buffer->size < node->output_buffer->size ?
                                  node->input_buffer->size : node->output_buffer->size;
                memcpy(node->output_buffer->data, node->input_buffer->data, copy_size);
                node->output_buffer->frame_count = node->input_buffer->frame_count;
                node->output_buffer->channels = node->input_buffer->channels;
            }
            break;
            
        default:
            return LINX_AUDIO_ERROR_NOT_SUPPORTED;
    }
    
    node->samples_processed += node->output_buffer ? node->output_buffer->frame_count : 0;
    
    return LINX_AUDIO_SUCCESS;
}

static linx_audio_result_t process_pipeline_graph(audio_pipeline_impl_t* pipeline) {
    // 简单的顺序处理，实际实现中应该使用拓扑排序
    pipeline_node_impl_t* node = pipeline->nodes;
    while (node) {
        if (node->state == LINX_PIPELINE_NODE_STATE_RUNNING) {
            linx_audio_result_t result = process_node(node);
            if (result != LINX_AUDIO_SUCCESS) {
                return result;
            }
        }
        node = node->next;
    }
    
    return LINX_AUDIO_SUCCESS;
}

static void* processing_thread_func(void* arg) {
    audio_pipeline_impl_t* pipeline = (audio_pipeline_impl_t*)arg;
    
    while (pipeline->thread_running) {
        pthread_mutex_lock(&pipeline->mutex);
        
        // 等待运行状态
        while (pipeline->state == LINX_PIPELINE_STATE_PAUSED && pipeline->thread_running) {
            pthread_cond_wait(&pipeline->state_cond, &pipeline->mutex);
        }
        
        if (!pipeline->thread_running) {
            pthread_mutex_unlock(&pipeline->mutex);
            break;
        }
        
        if (pipeline->state == LINX_PIPELINE_STATE_RUNNING) {
            // 处理管道
            linx_audio_result_t result = process_pipeline_graph(pipeline);
            if (result != LINX_AUDIO_SUCCESS) {
                // 处理错误
                pipeline->state = LINX_PIPELINE_STATE_ERROR;
                pthread_cond_broadcast(&pipeline->state_cond);
            } else {
                pipeline->frames_processed++;
            }
        }
        
        pthread_mutex_unlock(&pipeline->mutex);
        
        // 短暂休眠以避免过度占用CPU
        usleep(1000); // 1ms
    }
    
    return NULL;
}

static linx_audio_result_t allocate_temp_buffers(audio_pipeline_impl_t* pipeline) {
    // 根据配置分配临时缓冲区
    size_t buffer_size = pipeline->config.buffer_size * pipeline->format.channels * sizeof(float);
    size_t buffer_count = 8; // 默认分配8个临时缓冲区
    
    pipeline->temp_buffers = malloc(buffer_count * sizeof(linx_audio_buffer_t));
    if (!pipeline->temp_buffers) {
        return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < buffer_count; i++) {
        pipeline->temp_buffers[i].data = malloc(buffer_size);
        if (!pipeline->temp_buffers[i].data) {
            // 清理已分配的缓冲区
            for (size_t j = 0; j < i; j++) {
                free(pipeline->temp_buffers[j].data);
            }
            free(pipeline->temp_buffers);
            return LINX_AUDIO_ERROR_OUT_OF_MEMORY;
        }
        
        pipeline->temp_buffers[i].size = buffer_size;
        pipeline->temp_buffers[i].frame_count = pipeline->config.buffer_size;
        pipeline->temp_buffers[i].channels = pipeline->format.channels;
    }
    
    pipeline->temp_buffer_count = buffer_count;
    
    return LINX_AUDIO_SUCCESS;
}

static void free_temp_buffers(audio_pipeline_impl_t* pipeline) {
    if (pipeline->temp_buffers) {
        for (size_t i = 0; i < pipeline->temp_buffer_count; i++) {
            if (pipeline->temp_buffers[i].data) {
                free(pipeline->temp_buffers[i].data);
            }
        }
        free(pipeline->temp_buffers);
        pipeline->temp_buffers = NULL;
        pipeline->temp_buffer_count = 0;
    }
}