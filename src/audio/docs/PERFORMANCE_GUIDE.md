# LinxOS 音频系统性能优化指南

## 目录

1. [概述](#概述)
2. [性能指标](#性能指标)
3. [延迟优化](#延迟优化)
4. [吞吐量优化](#吞吐量优化)
5. [内存优化](#内存优化)
6. [CPU优化](#cpu优化)
7. [I/O优化](#io优化)
8. [实时性能](#实时性能)
9. [平台特定优化](#平台特定优化)
10. [性能监控](#性能监控)
11. [故障排除](#故障排除)
12. [最佳实践](#最佳实践)

---

## 概述

LinxOS音频系统的性能优化涉及多个层面，从硬件配置到软件算法，从内存管理到线程调度。本指南提供了全面的性能优化策略和最佳实践。

### 性能目标

- **延迟**：< 10ms (专业音频应用)
- **吞吐量**：支持192kHz/32bit多通道音频
- **CPU使用率**：< 20% (正常负载)
- **内存使用**：最小化内存分配和碎片
- **稳定性**：零音频丢失和故障

---

## 性能指标

### 1. 代码优化最佳实践

```c
// 1. 避免在实时线程中进行内存分配
void audio_process_realtime_good(const float* input, float* output,
                                uint32_t frame_count, void* context) {
    // 使用预分配的缓冲区
    audio_context_t* ctx = (audio_context_t*)context;
    float* temp_buffer = ctx->temp_buffer;  // 预分配
    
    // 处理音频数据
    for (uint32_t i = 0; i < frame_count; i++) {
        temp_buffer[i] = input[i] * ctx->gain;
        output[i] = apply_filter(temp_buffer[i], &ctx->filter_state);
    }
}

// 2. 使用栈分配而非堆分配
void audio_process_stack_allocation(const float* input, float* output,
                                   uint32_t frame_count) {
    // 好的做法：栈分配
    float temp_buffer[1024];  // 假设frame_count <= 1024
    
    if (frame_count <= 1024) {
        // 使用栈缓冲区
        memcpy(temp_buffer, input, frame_count * sizeof(float));
        // 处理...
    } else {
        // 回退到预分配的大缓冲区
        // 或者分块处理
    }
}

// 3. 缓存友好的数据访问模式
void audio_process_cache_friendly(float** channels, uint32_t channel_count,
                                 uint32_t frame_count) {
    // 好的做法：按帧处理（空间局部性）
    for (uint32_t frame = 0; frame < frame_count; frame++) {
        for (uint32_t ch = 0; ch < channel_count; ch++) {
            channels[ch][frame] = process_sample(channels[ch][frame]);
        }
    }
}

// 4. 避免分支预测失败
void audio_process_branchless_clipping(const float* input, float* output,
                                      uint32_t frame_count, float threshold) {
    for (uint32_t i = 0; i < frame_count; i++) {
        float sample = input[i];
        // 无分支限幅
        output[i] = fmaxf(-threshold, fminf(threshold, sample));
    }
}
```

### 2. 内存管理最佳实践

```c
// 内存池使用示例
typedef struct audio_memory_best_practices {
    audio_memory_pool_t* small_pool;    // 小对象池
    audio_memory_pool_t* buffer_pool;   // 音频缓冲区池
    
    // 预分配的工作缓冲区
    float* work_buffer_1;
    float* work_buffer_2;
    size_t work_buffer_size;
    
    // 对象缓存
    audio_filter_t* filter_cache[16];
    uint32_t filter_cache_count;
} audio_memory_best_practices_t;

// 智能缓冲区管理
audio_buffer_t* audio_get_buffer_smart(audio_memory_best_practices_t* mgr,
                                      size_t required_size) {
    // 1. 尝试从池中获取
    audio_buffer_t* buffer = audio_pool_get_buffer(mgr->buffer_pool, required_size);
    if (buffer) {
        return buffer;
    }
    
    // 2. 检查工作缓冲区
    if (required_size <= mgr->work_buffer_size) {
        // 使用预分配的工作缓冲区
        static audio_buffer_t work_buffer_wrapper;
        work_buffer_wrapper.data = mgr->work_buffer_1;
        work_buffer_wrapper.size = mgr->work_buffer_size;
        return &work_buffer_wrapper;
    }
    
    // 3. 最后回退到系统分配
    return audio_buffer_alloc_system(required_size);
}
```

### 3. 线程同步最佳实践

```c
// 无锁编程最佳实践
typedef struct audio_lockfree_best_practices {
    // 使用原子操作而非锁
    atomic_uint_fast32_t ref_count;
    atomic_bool processing_active;
    
    // 内存屏障的正确使用
    volatile uint32_t write_index;
    volatile uint32_t read_index;
} audio_lockfree_best_practices_t;

// 正确的无锁队列实现
bool audio_enqueue_lockfree(audio_lockfree_queue_t* queue, const void* item) {
    uint32_t current_tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    uint32_t next_tail = (current_tail + 1) % queue->capacity;
    
    // 检查队列是否满
    if (next_tail == atomic_load_explicit(&queue->head, memory_order_acquire)) {
        return false;  // 队列满
    }
    
    // 写入数据
    memcpy(&queue->data[current_tail * queue->item_size], item, queue->item_size);
    
    // 更新尾指针（释放语义）
    atomic_store_explicit(&queue->tail, next_tail, memory_order_release);
    return true;
}

// 读写锁的正确使用
typedef struct audio_config_manager {
    audio_config_t config;
    pthread_rwlock_t config_lock;
} audio_config_manager_t;

// 读取配置（多个线程可以同时读取）
audio_result_t audio_get_config(audio_config_manager_t* mgr, audio_config_t* config) {
    pthread_rwlock_rdlock(&mgr->config_lock);
    *config = mgr->config;  // 快速拷贝
    pthread_rwlock_unlock(&mgr->config_lock);
    return AUDIO_RESULT_SUCCESS;
}

// 更新配置（独占访问）
audio_result_t audio_update_config(audio_config_manager_t* mgr, 
                                  const audio_config_t* new_config) {
    pthread_rwlock_wrlock(&mgr->config_lock);
    mgr->config = *new_config;
    pthread_rwlock_unlock(&mgr->config_lock);
    return AUDIO_RESULT_SUCCESS;
}
```

### 4. 实时性能最佳实践

```c
// 实时线程的最佳实践
void* audio_realtime_thread_best_practices(void* arg) {
    audio_rt_context_t* ctx = (audio_rt_context_t*)arg;
    
    // 1. 设置线程属性
    configure_realtime_thread();
    
    // 2. 预热缓存
    audio_warmup_cache(ctx);
    
    // 3. 主处理循环
    while (ctx->running) {
        uint64_t cycle_start = audio_get_time_us();
        
        // 4. 处理音频数据
        audio_result_t result = audio_process_cycle(ctx);
        
        // 5. 监控性能
        uint64_t cycle_end = audio_get_time_us();
        audio_update_rt_stats(ctx, cycle_start, cycle_end);
        
        // 6. 错误处理
        if (result != AUDIO_RESULT_SUCCESS) {
            audio_handle_rt_error(ctx, result);
        }
        
        // 7. 等待下一个周期
        audio_wait_next_cycle(ctx);
    }
    
    return NULL;
}

// 缓存预热
void audio_warmup_cache(audio_rt_context_t* ctx) {
    // 访问所有关键数据结构
    volatile float dummy = 0.0f;
    
    // 预热音频缓冲区
    for (uint32_t i = 0; i < ctx->buffer_size; i++) {
        dummy += ctx->input_buffer[i];
        ctx->output_buffer[i] = 0.0f;
    }
    
    // 预热处理函数
    audio_process_dummy_cycle(ctx);
    
    // 预热内存池
    void* temp_ptrs[16];
    for (int i = 0; i < 16; i++) {
        temp_ptrs[i] = audio_pool_alloc(ctx->memory_pool, 64);
    }
    for (int i = 0; i < 16; i++) {
        audio_pool_free(ctx->memory_pool, temp_ptrs[i]);
    }
}
```

### 5. 错误处理最佳实践

```c
// 分层错误处理
typedef enum {
    AUDIO_ERROR_SEVERITY_INFO = 0,
    AUDIO_ERROR_SEVERITY_WARNING,
    AUDIO_ERROR_SEVERITY_ERROR,
    AUDIO_ERROR_SEVERITY_CRITICAL,
    AUDIO_ERROR_SEVERITY_FATAL
} audio_error_severity_t;

// 错误处理策略
audio_result_t audio_handle_error_best_practices(audio_error_context_t* error_ctx) {
    switch (error_ctx->severity) {
        case AUDIO_ERROR_SEVERITY_INFO:
            // 仅记录日志
            audio_log_info("Audio info: %s", error_ctx->message);
            break;
            
        case AUDIO_ERROR_SEVERITY_WARNING:
            // 记录日志并可能调整参数
            audio_log_warning("Audio warning: %s", error_ctx->message);
            audio_adjust_parameters_for_warning(error_ctx);
            break;
            
        case AUDIO_ERROR_SEVERITY_ERROR:
            // 尝试恢复
            audio_log_error("Audio error: %s", error_ctx->message);
            return audio_attempt_recovery(error_ctx);
            
        case AUDIO_ERROR_SEVERITY_CRITICAL:
            // 重启音频子系统
            audio_log_critical("Critical audio error: %s", error_ctx->message);
            return audio_restart_subsystem(error_ctx);
            
        case AUDIO_ERROR_SEVERITY_FATAL:
            // 安全关闭
            audio_log_fatal("Fatal audio error: %s", error_ctx->message);
            return audio_safe_shutdown(error_ctx);
    }
    
    return AUDIO_RESULT_SUCCESS;
}

// 渐进式降级策略
audio_result_t audio_graceful_degradation(audio_stream_t* stream, 
                                         audio_error_context_t* error) {
    switch (error->error_code) {
        case AUDIO_RESULT_HIGH_LATENCY:
            // 增加缓冲区大小
            return audio_increase_buffer_size(stream);
            
        case AUDIO_RESULT_CPU_OVERLOAD:
            // 降低处理质量
            return audio_reduce_processing_quality(stream);
            
        case AUDIO_RESULT_MEMORY_PRESSURE:
            // 释放非关键缓冲区
            return audio_free_non_critical_buffers(stream);
            
        case AUDIO_RESULT_DEVICE_ERROR:
            // 切换到备用设备
            return audio_switch_to_fallback_device(stream);
            
        default:
            return AUDIO_RESULT_ERROR;
    }
}
```

### 6. 测试和验证最佳实践

```c
// 性能测试框架
typedef struct audio_perf_test_suite {
    char name[64];
    
    // 测试用例
    struct {
        char name[64];
        void (*test_func)(audio_perf_test_context_t* ctx);
        uint32_t iterations;
        uint64_t max_time_us;
        bool enabled;
    } test_cases[AUDIO_MAX_TEST_CASES];
    uint32_t test_count;
    
    // 测试结果
    struct {
        uint64_t min_time_us;
        uint64_t max_time_us;
        uint64_t avg_time_us;
        bool passed;
    } results[AUDIO_MAX_TEST_CASES];
} audio_perf_test_suite_t;

// 基准测试
void audio_benchmark_processing_latency(audio_perf_test_context_t* ctx) {
    const uint32_t frame_count = 512;
    const uint32_t iterations = 1000;
    
    float* input = ctx->test_input_buffer;
    float* output = ctx->test_output_buffer;
    
    uint64_t total_time = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    for (uint32_t i = 0; i < iterations; i++) {
        uint64_t start_time = audio_get_time_us();
        
        // 执行音频处理
        audio_process_test_data(input, output, frame_count);
        
        uint64_t end_time = audio_get_time_us();
        uint64_t elapsed = end_time - start_time;
        
        total_time += elapsed;
        min_time = (elapsed < min_time) ? elapsed : min_time;
        max_time = (elapsed > max_time) ? elapsed : max_time;
    }
    
    // 记录结果
    ctx->results.min_latency_us = min_time;
    ctx->results.max_latency_us = max_time;
    ctx->results.avg_latency_us = total_time / iterations;
    
    // 验证性能要求
    ctx->results.passed = (ctx->results.avg_latency_us < ctx->max_allowed_latency_us);
}

// 压力测试
void audio_stress_test_memory_allocation(audio_perf_test_context_t* ctx) {
    const uint32_t allocation_count = 10000;
    const size_t allocation_size = 1024;
    
    void** allocations = malloc(allocation_count * sizeof(void*));
    uint64_t start_time = audio_get_time_us();
    
    // 分配内存
    for (uint32_t i = 0; i < allocation_count; i++) {
        allocations[i] = audio_alloc(allocation_size);
        if (!allocations[i]) {
            ctx->results.passed = false;
            break;
        }
    }
    
    // 释放内存
    for (uint32_t i = 0; i < allocation_count; i++) {
        if (allocations[i]) {
            audio_free(allocations[i]);
        }
    }
    
    uint64_t end_time = audio_get_time_us();
    ctx->results.total_time_us = end_time - start_time;
    
    free(allocations);
}
```

### 7. 配置优化最佳实践

```c
// 自适应配置系统
typedef struct audio_adaptive_config {
    // 当前配置
    audio_config_t current_config;
    
    // 性能历史
    float cpu_usage_history[AUDIO_HISTORY_SIZE];
    uint32_t latency_history[AUDIO_HISTORY_SIZE];
    uint32_t underrun_history[AUDIO_HISTORY_SIZE];
    uint32_t history_index;
    
    // 自适应参数
    bool auto_adjust_enabled;
    uint32_t adjustment_interval_ms;
    float cpu_target_percent;
    uint32_t latency_target_us;
    
    // 调整策略
    audio_adjustment_strategy_t strategy;
} audio_adaptive_config_t;

// 自动配置调整
audio_result_t audio_auto_adjust_config(audio_adaptive_config_t* adaptive) {
    if (!adaptive->auto_adjust_enabled) {
        return AUDIO_RESULT_SUCCESS;
    }
    
    // 计算平均性能指标
    float avg_cpu = audio_calculate_average(adaptive->cpu_usage_history, AUDIO_HISTORY_SIZE);
    uint32_t avg_latency = audio_calculate_average_uint32(adaptive->latency_history, AUDIO_HISTORY_SIZE);
    uint32_t underrun_rate = audio_calculate_underrun_rate(adaptive->underrun_history, AUDIO_HISTORY_SIZE);
    
    // 决定调整策略
    if (avg_cpu > adaptive->cpu_target_percent * 1.2f) {
        // CPU使用率过高，降低质量
        return audio_reduce_processing_load(&adaptive->current_config);
    } else if (avg_latency > adaptive->latency_target_us * 1.5f) {
        // 延迟过高，减小缓冲区
        return audio_reduce_buffer_size(&adaptive->current_config);
    } else if (underrun_rate > 5) {
        // 下溢过多，增加缓冲区
        return audio_increase_buffer_size(&adaptive->current_config);
    } else if (avg_cpu < adaptive->cpu_target_percent * 0.5f && 
               avg_latency < adaptive->latency_target_us * 0.8f) {
        // 性能有余量，提高质量
        return audio_increase_processing_quality(&adaptive->current_config);
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

---

## 总结

LinxOS音频系统的性能优化是一个多层面的工程，需要从硬件到软件、从算法到架构进行全方位的考虑。本指南提供的策略和最佳实践可以帮助开发者：

1. **实现低延迟**：通过优化缓冲区大小、使用零拷贝技术和实时调度
2. **提高吞吐量**：通过SIMD优化、并行处理和流水线技术
3. **优化内存使用**：通过内存池、缓存优化和智能分配策略
4. **提升实时性能**：通过无锁编程、实时线程配置和性能监控
5. **确保系统稳定性**：通过错误处理、性能诊断和自适应配置

遵循这些最佳实践，可以构建出高性能、低延迟、稳定可靠的音频系统。

### 1. 关键性能指标 (KPI)

```c
// 性能指标结构
typedef struct audio_performance_metrics {
    // 延迟指标
    uint32_t input_latency_us;      // 输入延迟 (微秒)
    uint32_t output_latency_us;     // 输出延迟 (微秒)
    uint32_t total_latency_us;      // 总延迟 (微秒)
    uint32_t buffer_latency_us;     // 缓冲延迟 (微秒)
    
    // 吞吐量指标
    uint64_t samples_processed;     // 处理的样本数
    uint32_t sample_rate;           // 采样率
    uint32_t channels;              // 通道数
    uint32_t bit_depth;             // 位深度
    float throughput_mbps;          // 吞吐量 (Mbps)
    
    // CPU指标
    float cpu_usage_percent;        // CPU使用率
    uint64_t cpu_cycles;            // CPU周期数
    uint32_t context_switches;      // 上下文切换次数
    
    // 内存指标
    size_t memory_used;             // 使用的内存
    size_t memory_peak;             // 峰值内存
    uint32_t allocations;           // 分配次数
    uint32_t deallocations;         // 释放次数
    
    // 质量指标
    uint32_t underruns;             // 缓冲区下溢
    uint32_t overruns;              // 缓冲区上溢
    uint32_t dropouts;              // 音频丢失
    float thd_percent;              // 总谐波失真
    float snr_db;                   // 信噪比
} audio_performance_metrics_t;
```

### 2. 性能测量工具

```c
// 性能计时器
typedef struct audio_perf_timer {
    uint64_t start_time;
    uint64_t end_time;
    uint64_t total_time;
    uint32_t call_count;
    uint64_t min_time;
    uint64_t max_time;
    uint64_t avg_time;
} audio_perf_timer_t;

// 性能分析器
typedef struct audio_profiler {
    audio_perf_timer_t timers[AUDIO_MAX_TIMERS];
    uint32_t timer_count;
    bool enabled;
    
    // 采样配置
    uint32_t sample_rate;
    uint32_t sample_count;
    
    // 输出配置
    char output_file[256];
    bool real_time_output;
} audio_profiler_t;

// 性能测量宏
#define AUDIO_PERF_START(profiler, timer_id) \
    if ((profiler)->enabled) { \
        (profiler)->timers[timer_id].start_time = audio_get_time_us(); \
    }

#define AUDIO_PERF_END(profiler, timer_id) \
    if ((profiler)->enabled) { \
        (profiler)->timers[timer_id].end_time = audio_get_time_us(); \
        audio_perf_update_timer(&(profiler)->timers[timer_id]); \
    }
```

---

## 延迟优化

### 1. 缓冲区大小优化

```c
// 延迟计算
uint32_t calculate_buffer_latency(uint32_t buffer_size_frames, uint32_t sample_rate) {
    return (buffer_size_frames * 1000000) / sample_rate;  // 微秒
}

// 最优缓冲区大小选择
uint32_t select_optimal_buffer_size(uint32_t target_latency_us, uint32_t sample_rate) {
    uint32_t min_frames = 64;   // 最小缓冲区
    uint32_t max_frames = 2048; // 最大缓冲区
    
    uint32_t target_frames = (target_latency_us * sample_rate) / 1000000;
    
    // 向上取整到2的幂
    uint32_t frames = min_frames;
    while (frames < target_frames && frames < max_frames) {
        frames *= 2;
    }
    
    return frames;
}

// 动态缓冲区调整
audio_result_t adjust_buffer_size_dynamic(audio_stream_t* stream, 
                                         uint32_t current_latency_us,
                                         uint32_t target_latency_us) {
    if (current_latency_us > target_latency_us * 1.2) {
        // 延迟过高，减小缓冲区
        uint32_t new_size = stream->buffer_size_frames / 2;
        if (new_size >= 64) {
            return audio_stream_set_buffer_size(stream, new_size);
        }
    } else if (current_latency_us < target_latency_us * 0.8) {
        // 延迟过低，可能不稳定，增大缓冲区
        uint32_t new_size = stream->buffer_size_frames * 2;
        if (new_size <= 2048) {
            return audio_stream_set_buffer_size(stream, new_size);
        }
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 2. 零拷贝技术

```c
// 零拷贝缓冲区实现
typedef struct audio_zero_copy_ring_buffer {
    void* memory_region;        // 内存区域
    size_t region_size;         // 区域大小
    
    volatile uint32_t write_pos; // 写位置
    volatile uint32_t read_pos;  // 读位置
    
    uint32_t frame_size;        // 帧大小
    uint32_t capacity_frames;   // 容量(帧)
    
    // 内存映射
    bool use_mmap;
    int fd;
    
    // 原子操作
    atomic_uint_fast32_t ref_count;
} audio_zero_copy_ring_buffer_t;

// 零拷贝写入
audio_result_t zero_copy_write(audio_zero_copy_ring_buffer_t* buffer,
                              const void* data, uint32_t frame_count,
                              void** write_ptr, uint32_t* available_frames) {
    uint32_t write_pos = atomic_load(&buffer->write_pos);
    uint32_t read_pos = atomic_load(&buffer->read_pos);
    
    // 计算可用空间
    uint32_t available = (read_pos - write_pos - 1 + buffer->capacity_frames) 
                        % buffer->capacity_frames;
    
    if (available < frame_count) {
        return AUDIO_RESULT_BUFFER_FULL;
    }
    
    // 直接返回写入指针，避免拷贝
    *write_ptr = (char*)buffer->memory_region + 
                 (write_pos * buffer->frame_size);
    *available_frames = min(frame_count, 
                           buffer->capacity_frames - write_pos);
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 3. 实时调度优化

```c
// 实时线程配置
audio_result_t configure_realtime_thread(pthread_t thread, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    
    // 设置实时调度策略
    if (pthread_setschedparam(thread, SCHED_FIFO, &param) != 0) {
        return AUDIO_RESULT_THREAD_ERROR;
    }
    
    // 锁定内存页
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        // 非致命错误，继续执行
    }
    
    return AUDIO_RESULT_SUCCESS;
}

// CPU亲和性设置
audio_result_t set_cpu_affinity(pthread_t thread, int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        return AUDIO_RESULT_THREAD_ERROR;
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

---

## 吞吐量优化

### 1. SIMD优化

```c
// SSE优化的音频混合
void audio_mix_stereo_f32_sse(const float* input1, const float* input2,
                              float* output, uint32_t frame_count) {
    uint32_t simd_frames = frame_count & ~3;  // 4帧对齐
    uint32_t i;
    
    for (i = 0; i < simd_frames; i += 4) {
        __m128 a = _mm_load_ps(&input1[i]);
        __m128 b = _mm_load_ps(&input2[i]);
        __m128 result = _mm_add_ps(a, b);
        _mm_store_ps(&output[i], result);
    }
    
    // 处理剩余帧
    for (; i < frame_count; i++) {
        output[i] = input1[i] + input2[i];
    }
}

// AVX优化的音频处理
void audio_process_f32_avx(const float* input, float* output,
                          uint32_t frame_count, float gain) {
    __m256 gain_vec = _mm256_set1_ps(gain);
    uint32_t avx_frames = frame_count & ~7;  // 8帧对齐
    uint32_t i;
    
    for (i = 0; i < avx_frames; i += 8) {
        __m256 data = _mm256_load_ps(&input[i]);
        __m256 result = _mm256_mul_ps(data, gain_vec);
        _mm256_store_ps(&output[i], result);
    }
    
    // 处理剩余帧
    for (; i < frame_count; i++) {
        output[i] = input[i] * gain;
    }
}

// NEON优化 (ARM)
#ifdef __ARM_NEON__
void audio_mix_stereo_f32_neon(const float* input1, const float* input2,
                               float* output, uint32_t frame_count) {
    uint32_t neon_frames = frame_count & ~3;  // 4帧对齐
    uint32_t i;
    
    for (i = 0; i < neon_frames; i += 4) {
        float32x4_t a = vld1q_f32(&input1[i]);
        float32x4_t b = vld1q_f32(&input2[i]);
        float32x4_t result = vaddq_f32(a, b);
        vst1q_f32(&output[i], result);
    }
    
    // 处理剩余帧
    for (; i < frame_count; i++) {
        output[i] = input1[i] + input2[i];
    }
}
#endif
```

### 2. 并行处理

```c
// 并行音频处理框架
typedef struct audio_parallel_task {
    void (*process_func)(void* input, void* output, uint32_t count, void* params);
    void* input_data;
    void* output_data;
    uint32_t data_count;
    void* parameters;
    
    // 同步对象
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool completed;
} audio_parallel_task_t;

// 工作线程池
typedef struct audio_thread_pool {
    pthread_t* threads;
    uint32_t thread_count;
    
    // 任务队列
    audio_parallel_task_t* task_queue;
    uint32_t queue_size;
    uint32_t queue_head;
    uint32_t queue_tail;
    
    // 同步对象
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_condition;
    
    bool shutdown;
} audio_thread_pool_t;

// 并行音频处理
audio_result_t process_audio_parallel(audio_thread_pool_t* pool,
                                     const float* input, float* output,
                                     uint32_t frame_count, uint32_t channels) {
    uint32_t frames_per_thread = frame_count / pool->thread_count;
    uint32_t remaining_frames = frame_count % pool->thread_count;
    
    // 分配任务
    for (uint32_t i = 0; i < pool->thread_count; i++) {
        uint32_t start_frame = i * frames_per_thread;
        uint32_t count = frames_per_thread;
        
        // 最后一个线程处理剩余帧
        if (i == pool->thread_count - 1) {
            count += remaining_frames;
        }
        
        audio_parallel_task_t task = {
            .process_func = audio_process_chunk,
            .input_data = (void*)(input + start_frame * channels),
            .output_data = (void*)(output + start_frame * channels),
            .data_count = count * channels,
            .parameters = NULL,
            .completed = false
        };
        
        // 添加到任务队列
        audio_thread_pool_add_task(pool, &task);
    }
    
    // 等待所有任务完成
    return audio_thread_pool_wait_all(pool);
}
```

### 3. 流水线处理

```c
// 流水线阶段
typedef struct audio_pipeline_stage {
    char name[64];
    void (*process_func)(void* input, void* output, uint32_t count);
    
    // 缓冲区
    void* input_buffer;
    void* output_buffer;
    uint32_t buffer_size;
    
    // 同步对象
    pthread_mutex_t mutex;
    pthread_cond_t input_ready;
    pthread_cond_t output_ready;
    
    // 状态
    bool input_available;
    bool output_consumed;
    pthread_t thread;
} audio_pipeline_stage_t;

// 流水线处理器
typedef struct audio_pipeline_processor {
    audio_pipeline_stage_t* stages;
    uint32_t stage_count;
    
    // 全局同步
    pthread_barrier_t start_barrier;
    pthread_barrier_t end_barrier;
    
    bool running;
} audio_pipeline_processor_t;
```

---

## 内存优化

### 1. 内存池管理

```c
// 分层内存池
typedef struct audio_memory_tier {
    size_t block_size;          // 块大小
    uint32_t block_count;       // 块数量
    uint32_t free_count;        // 空闲块数
    
    void* memory_region;        // 内存区域
    void** free_list;           // 空闲列表
    
    // 统计信息
    uint64_t allocations;
    uint64_t deallocations;
    uint32_t peak_usage;
    
    pthread_mutex_t mutex;
} audio_memory_tier_t;

// 内存池管理器
typedef struct audio_memory_pool_manager {
    audio_memory_tier_t tiers[AUDIO_MEMORY_TIER_COUNT];
    
    // 全局统计
    size_t total_allocated;
    size_t total_capacity;
    uint32_t fragmentation_percent;
    
    // 配置
    bool enable_compaction;     // 启用内存整理
    uint32_t compaction_threshold; // 整理阈值
} audio_memory_pool_manager_t;

// 智能内存分配
void* audio_smart_alloc(audio_memory_pool_manager_t* manager, size_t size) {
    // 选择合适的内存层
    for (int i = 0; i < AUDIO_MEMORY_TIER_COUNT; i++) {
        if (size <= manager->tiers[i].block_size) {
            return audio_memory_tier_alloc(&manager->tiers[i]);
        }
    }
    
    // 回退到系统分配
    return malloc(size);
}
```

### 2. 缓存优化

```c
// 缓存友好的数据结构
typedef struct audio_cache_aligned_buffer {
    alignas(64) float data[];   // 64字节对齐
} audio_cache_aligned_buffer_t;

// 数据预取
void audio_prefetch_data(const void* data, size_t size) {
    const char* ptr = (const char*)data;
    const char* end = ptr + size;
    
    while (ptr < end) {
        __builtin_prefetch(ptr, 0, 3);  // 预取到L1缓存
        ptr += 64;  // 缓存行大小
    }
}

// 缓存优化的音频拷贝
void audio_copy_optimized(void* dst, const void* src, size_t size) {
    // 预取源数据
    audio_prefetch_data(src, size);
    
    // 使用非临时存储指令
    const char* s = (const char*)src;
    char* d = (char*)dst;
    size_t aligned_size = size & ~63;  // 64字节对齐
    
    for (size_t i = 0; i < aligned_size; i += 64) {
        __m256i data1 = _mm256_load_si256((__m256i*)(s + i));
        __m256i data2 = _mm256_load_si256((__m256i*)(s + i + 32));
        
        _mm256_stream_si256((__m256i*)(d + i), data1);
        _mm256_stream_si256((__m256i*)(d + i + 32), data2);
    }
    
    // 处理剩余字节
    memcpy(d + aligned_size, s + aligned_size, size - aligned_size);
    
    // 内存屏障
    _mm_sfence();
}
```

### 3. 内存映射优化

```c
// 大文件内存映射
typedef struct audio_mmap_file {
    int fd;
    void* mapped_addr;
    size_t file_size;
    size_t page_size;
    
    // 访问模式
    int access_pattern;         // 顺序/随机访问
    bool use_huge_pages;        // 使用大页
    bool populate_pages;        // 预填充页面
} audio_mmap_file_t;

// 优化的内存映射
audio_result_t audio_mmap_file_optimized(const char* filename,
                                        audio_mmap_file_t* mmap_file) {
    mmap_file->fd = open(filename, O_RDONLY);
    if (mmap_file->fd == -1) {
        return AUDIO_RESULT_FILE_ERROR;
    }
    
    // 获取文件大小
    struct stat st;
    if (fstat(mmap_file->fd, &st) == -1) {
        close(mmap_file->fd);
        return AUDIO_RESULT_FILE_ERROR;
    }
    mmap_file->file_size = st.st_size;
    
    // 映射标志
    int flags = MAP_PRIVATE;
    if (mmap_file->populate_pages) {
        flags |= MAP_POPULATE;
    }
    if (mmap_file->use_huge_pages) {
        flags |= MAP_HUGETLB;
    }
    
    // 执行映射
    mmap_file->mapped_addr = mmap(NULL, mmap_file->file_size,
                                 PROT_READ, flags, mmap_file->fd, 0);
    
    if (mmap_file->mapped_addr == MAP_FAILED) {
        close(mmap_file->fd);
        return AUDIO_RESULT_MEMORY_ERROR;
    }
    
    // 设置访问建议
    int advice = (mmap_file->access_pattern == AUDIO_ACCESS_SEQUENTIAL) ?
                 MADV_SEQUENTIAL : MADV_RANDOM;
    madvise(mmap_file->mapped_addr, mmap_file->file_size, advice);
    
    return AUDIO_RESULT_SUCCESS;
}
```

---

## CPU优化

### 1. 分支预测优化

```c
// 分支预测宏
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// 优化的音频处理循环
void audio_process_with_branch_optimization(const float* input, float* output,
                                           uint32_t frame_count, float threshold) {
    for (uint32_t i = 0; i < frame_count; i++) {
        float sample = input[i];
        
        // 大多数样本不会超过阈值
        if (UNLIKELY(fabsf(sample) > threshold)) {
            // 限幅处理
            output[i] = (sample > 0) ? threshold : -threshold;
        } else {
            output[i] = sample;
        }
    }
}

// 无分支的音频处理
void audio_process_branchless(const float* input, float* output,
                             uint32_t frame_count, float threshold) {
    for (uint32_t i = 0; i < frame_count; i++) {
        float sample = input[i];
        float abs_sample = fabsf(sample);
        
        // 无分支限幅
        float sign = copysignf(1.0f, sample);
        float limited = fminf(abs_sample, threshold);
        output[i] = sign * limited;
    }
}
```

### 2. 循环优化

```c
// 循环展开
void audio_gain_unrolled(const float* input, float* output,
                        uint32_t frame_count, float gain) {
    uint32_t unroll_count = frame_count & ~7;  // 8的倍数
    uint32_t i;
    
    // 8倍循环展开
    for (i = 0; i < unroll_count; i += 8) {
        output[i + 0] = input[i + 0] * gain;
        output[i + 1] = input[i + 1] * gain;
        output[i + 2] = input[i + 2] * gain;
        output[i + 3] = input[i + 3] * gain;
        output[i + 4] = input[i + 4] * gain;
        output[i + 5] = input[i + 5] * gain;
        output[i + 6] = input[i + 6] * gain;
        output[i + 7] = input[i + 7] * gain;
    }
    
    // 处理剩余样本
    for (; i < frame_count; i++) {
        output[i] = input[i] * gain;
    }
}

// 软件流水线
void audio_pipeline_software(const float* input, float* output,
                            uint32_t frame_count) {
    if (frame_count < 4) {
        // 简单处理
        for (uint32_t i = 0; i < frame_count; i++) {
            output[i] = process_sample(input[i]);
        }
        return;
    }
    
    // 预加载前几个样本
    float s0 = input[0];
    float s1 = input[1];
    float s2 = input[2];
    
    // 流水线处理
    for (uint32_t i = 0; i < frame_count - 3; i++) {
        float s3 = input[i + 3];  // 预加载
        
        output[i] = process_sample(s0);
        
        // 移动流水线
        s0 = s1;
        s1 = s2;
        s2 = s3;
    }
    
    // 处理剩余样本
    output[frame_count - 3] = process_sample(s0);
    output[frame_count - 2] = process_sample(s1);
    output[frame_count - 1] = process_sample(s2);
}
```

### 3. 函数内联优化

```c
// 强制内联的快速数学函数
static inline __attribute__((always_inline))
float fast_sin_approx(float x) {
    // 快速正弦近似
    const float B = 4.0f / M_PI;
    const float C = -4.0f / (M_PI * M_PI);
    
    float y = B * x + C * x * fabsf(x);
    
    // 提高精度的修正项
    const float P = 0.225f;
    y = P * (y * fabsf(y) - y) + y;
    
    return y;
}

static inline __attribute__((always_inline))
float fast_exp_approx(float x) {
    // 快速指数近似
    union { float f; int32_t i; } u;
    u.i = (int32_t)(12102203.0f * x + 1065353216.0f);
    return u.f;
}

// 内联的音频效果处理
static inline __attribute__((always_inline))
float apply_distortion(float sample, float drive, float mix) {
    float driven = sample * drive;
    float distorted = tanhf(driven);  // 软限幅失真
    return sample * (1.0f - mix) + distorted * mix;
}
```

---

## I/O优化

### 1. 异步I/O

```c
// 异步I/O上下文
typedef struct audio_aio_context {
    struct aiocb* aiocb_list;
    uint32_t request_count;
    uint32_t completed_count;
    
    // 回调函数
    void (*completion_callback)(struct aiocb* aiocb, int result);
    
    // 统计信息
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t read_operations;
    uint32_t write_operations;
} audio_aio_context_t;

// 异步读取音频文件
audio_result_t audio_read_async(audio_aio_context_t* ctx,
                               int fd, void* buffer, size_t size,
                               off_t offset) {
    struct aiocb* aiocb = &ctx->aiocb_list[ctx->request_count];
    
    memset(aiocb, 0, sizeof(struct aiocb));
    aiocb->aio_fildes = fd;
    aiocb->aio_buf = buffer;
    aiocb->aio_nbytes = size;
    aiocb->aio_offset = offset;
    aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;
    
    if (aio_read(aiocb) == -1) {
        return AUDIO_RESULT_IO_ERROR;
    }
    
    ctx->request_count++;
    return AUDIO_RESULT_SUCCESS;
}

// 批量I/O完成检查
audio_result_t audio_aio_check_completion(audio_aio_context_t* ctx) {
    struct aiocb** aiocb_list = (struct aiocb**)ctx->aiocb_list;
    
    // 检查所有未完成的请求
    for (uint32_t i = ctx->completed_count; i < ctx->request_count; i++) {
        int status = aio_error(&ctx->aiocb_list[i]);
        
        if (status == 0) {
            // 操作完成
            ssize_t bytes = aio_return(&ctx->aiocb_list[i]);
            if (bytes > 0) {
                ctx->bytes_read += bytes;
                ctx->read_operations++;
            }
            
            if (ctx->completion_callback) {
                ctx->completion_callback(&ctx->aiocb_list[i], bytes);
            }
            
            ctx->completed_count++;
        } else if (status != EINPROGRESS) {
            // 发生错误
            return AUDIO_RESULT_IO_ERROR;
        }
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 2. 直接I/O

```c
// 直接I/O配置
typedef struct audio_direct_io_config {
    bool enabled;               // 启用直接I/O
    size_t alignment;           // 对齐要求
    size_t block_size;          // 块大小
    uint32_t queue_depth;       // 队列深度
} audio_direct_io_config_t;

// 对齐的缓冲区分配
void* audio_alloc_aligned_buffer(size_t size, size_t alignment) {
    void* ptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

// 直接I/O读取
audio_result_t audio_read_direct(int fd, void* buffer, size_t size,
                                off_t offset, const audio_direct_io_config_t* config) {
    // 检查对齐
    if ((uintptr_t)buffer % config->alignment != 0) {
        return AUDIO_RESULT_ALIGNMENT_ERROR;
    }
    
    if (size % config->block_size != 0) {
        return AUDIO_RESULT_SIZE_ERROR;
    }
    
    // 设置直接I/O标志
    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_DIRECT) == -1) {
        return AUDIO_RESULT_IO_ERROR;
    }
    
    // 执行读取
    ssize_t bytes_read = pread(fd, buffer, size, offset);
    if (bytes_read != (ssize_t)size) {
        return AUDIO_RESULT_IO_ERROR;
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 3. 批量I/O

```c
// 批量I/O请求
typedef struct audio_batch_io_request {
    int fd;
    void* buffer;
    size_t size;
    off_t offset;
    bool is_write;
    
    // 完成状态
    bool completed;
    ssize_t result;
} audio_batch_io_request_t;

// 批量I/O处理
audio_result_t audio_process_batch_io(audio_batch_io_request_t* requests,
                                     uint32_t request_count) {
    // 使用io_uring (Linux) 或类似机制
    #ifdef __linux__
    struct io_uring ring;
    
    if (io_uring_queue_init(request_count, &ring, 0) < 0) {
        return AUDIO_RESULT_IO_ERROR;
    }
    
    // 提交所有请求
    for (uint32_t i = 0; i < request_count; i++) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            break;
        }
        
        if (requests[i].is_write) {
            io_uring_prep_write(sqe, requests[i].fd, requests[i].buffer,
                               requests[i].size, requests[i].offset);
        } else {
            io_uring_prep_read(sqe, requests[i].fd, requests[i].buffer,
                              requests[i].size, requests[i].offset);
        }
        
        io_uring_sqe_set_data(sqe, &requests[i]);
    }
    
    io_uring_submit(&ring);
    
    // 等待完成
    for (uint32_t i = 0; i < request_count; i++) {
        struct io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring, &cqe);
        
        audio_batch_io_request_t* req = 
            (audio_batch_io_request_t*)io_uring_cqe_get_data(cqe);
        req->result = cqe->res;
        req->completed = true;
        
        io_uring_cqe_seen(&ring, cqe);
    }
    
    io_uring_queue_exit(&ring);
    #endif
    
    return AUDIO_RESULT_SUCCESS;
}
```

---

## 实时性能

### 1. 实时线程配置

```c
// 实时性能配置
typedef struct audio_realtime_config {
    int thread_priority;        // 线程优先级 (1-99)
    int scheduling_policy;      // 调度策略
    size_t stack_size;          // 栈大小
    bool lock_memory;           // 锁定内存
    bool disable_interrupts;    // 禁用中断
    uint32_t cpu_affinity_mask; // CPU亲和性掩码
} audio_realtime_config_t;

// 配置实时性能
audio_result_t configure_realtime_performance(const audio_realtime_config_t* config) {
    // 设置线程优先级
    struct sched_param param;
    param.sched_priority = config->thread_priority;
    
    if (sched_setscheduler(0, config->scheduling_policy, &param) != 0) {
        return AUDIO_RESULT_THREAD_ERROR;
    }
    
    // 锁定内存
    if (config->lock_memory) {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            // 非致命错误
        }
    }
    
    // 设置CPU亲和性
    if (config->cpu_affinity_mask != 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        
        for (int i = 0; i < 32; i++) {
            if (config->cpu_affinity_mask & (1U << i)) {
                CPU_SET(i, &cpuset);
            }
        }
        
        sched_setaffinity(0, sizeof(cpuset), &cpuset);
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 2. 无锁数据结构

```c
// 无锁环形缓冲区
typedef struct audio_lockfree_ringbuffer {
    alignas(64) volatile uint32_t write_pos;
    alignas(64) volatile uint32_t read_pos;
    
    uint32_t capacity;
    uint32_t mask;              // capacity - 1 (必须是2的幂)
    
    alignas(64) char data[];
} audio_lockfree_ringbuffer_t;

// 无锁写入
bool audio_lockfree_write(audio_lockfree_ringbuffer_t* rb,
                         const void* data, uint32_t size) {
    uint32_t write_pos = atomic_load_explicit(&rb->write_pos, memory_order_relaxed);
    uint32_t read_pos = atomic_load_explicit(&rb->read_pos, memory_order_acquire);
    
    uint32_t available = rb->capacity - (write_pos - read_pos);
    if (available < size) {
        return false;  // 缓冲区满
    }
    
    // 拷贝数据
    uint32_t write_index = write_pos & rb->mask;
    if (write_index + size <= rb->capacity) {
        // 连续拷贝
        memcpy(&rb->data[write_index], data, size);
    } else {
        // 分段拷贝
        uint32_t first_part = rb->capacity - write_index;
        memcpy(&rb->data[write_index], data, first_part);
        memcpy(&rb->data[0], (char*)data + first_part, size - first_part);
    }
    
    // 更新写位置
    atomic_store_explicit(&rb->write_pos, write_pos + size, memory_order_release);
    return true;
}

// 无锁读取
bool audio_lockfree_read(audio_lockfree_ringbuffer_t* rb,
                        void* data, uint32_t size) {
    uint32_t read_pos = atomic_load_explicit(&rb->read_pos, memory_order_relaxed);
    uint32_t write_pos = atomic_load_explicit(&rb->write_pos, memory_order_acquire);
    
    uint32_t available = write_pos - read_pos;
    if (available < size) {
        return false;  // 数据不足
    }
    
    // 拷贝数据
    uint32_t read_index = read_pos & rb->mask;
    if (read_index + size <= rb->capacity) {
        // 连续拷贝
        memcpy(data, &rb->data[read_index], size);
    } else {
        // 分段拷贝
        uint32_t first_part = rb->capacity - read_index;
        memcpy(data, &rb->data[read_index], first_part);
        memcpy((char*)data + first_part, &rb->data[0], size - first_part);
    }
    
    // 更新读位置
    atomic_store_explicit(&rb->read_pos, read_pos + size, memory_order_release);
    return true;
}
```

### 3. 实时监控

```c
// 实时性能监控
typedef struct audio_realtime_monitor {
    // 延迟统计
    uint32_t min_latency_us;
    uint32_t max_latency_us;
    uint32_t avg_latency_us;
    uint64_t latency_sum;
    uint32_t latency_samples;
    
    // 抖动统计
    uint32_t jitter_us;
    uint32_t max_jitter_us;
    
    // 错误计数
    uint32_t underruns;
    uint32_t overruns;
    uint32_t missed_deadlines;
    
    // 时间戳
    uint64_t last_callback_time;
    uint64_t callback_interval_us;
} audio_realtime_monitor_t;

// 更新实时监控
void audio_update_realtime_monitor(audio_realtime_monitor_t* monitor,
                                  uint64_t callback_start_time,
                                  uint64_t callback_end_time) {
    uint64_t latency = callback_end_time - callback_start_time;
    
    // 更新延迟统计
    if (monitor->latency_samples == 0) {
        monitor->min_latency_us = latency;
        monitor->max_latency_us = latency;
    } else {
        if (latency < monitor->min_latency_us) {
            monitor->min_latency_us = latency;
        }
        if (latency > monitor->max_latency_us) {
            monitor->max_latency_us = latency;
        }
    }
    
    monitor->latency_sum += latency;
    monitor->latency_samples++;
    monitor->avg_latency_us = monitor->latency_sum / monitor->latency_samples;
    
    // 计算抖动
    if (monitor->last_callback_time != 0) {
        uint64_t interval = callback_start_time - monitor->last_callback_time;
        uint64_t expected_interval = monitor->callback_interval_us;
        
        uint64_t jitter = (interval > expected_interval) ?
                         (interval - expected_interval) :
                         (expected_interval - interval);
        
        monitor->jitter_us = jitter;
        if (jitter > monitor->max_jitter_us) {
            monitor->max_jitter_us = jitter;
        }
    }
    
    monitor->last_callback_time = callback_start_time;
}
```

---

## 平台特定优化

### 1. Linux优化

```c
// Linux特定优化
typedef struct audio_linux_optimization {
    // ALSA优化
    bool use_mmap_mode;         // 使用MMAP模式
    uint32_t period_size;       // 周期大小
    uint32_t buffer_periods;    // 缓冲区周期数
    
    // 系统优化
    bool disable_cpu_freq_scaling; // 禁用CPU频率缩放
    bool set_rlimits;           // 设置资源限制
    int nice_value;             // Nice值
    
    // 内核优化
    bool use_rt_kernel;         // 使用实时内核
    bool disable_irq_balance;   // 禁用IRQ平衡
} audio_linux_optimization_t;

// 应用Linux优化
audio_result_t apply_linux_optimizations(const audio_linux_optimization_t* opt) {
    // 设置资源限制
    if (opt->set_rlimits) {
        struct rlimit rlim;
        
        // 设置实时优先级限制
        rlim.rlim_cur = 99;
        rlim.rlim_max = 99;
        setrlimit(RLIMIT_RTPRIO, &rlim);
        
        // 设置内存锁定限制
        rlim.rlim_cur = RLIM_INFINITY;
        rlim.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_MEMLOCK, &rlim);
    }
    
    // 设置Nice值
    if (opt->nice_value != 0) {
        nice(opt->nice_value);
    }
    
    // 禁用CPU频率缩放
    if (opt->disable_cpu_freq_scaling) {
        system("echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor");
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 2. macOS优化

```c
// macOS特定优化
typedef struct audio_macos_optimization {
    // CoreAudio优化
    bool use_hog_mode;          // 独占模式
    uint32_t buffer_frame_size; // 缓冲区帧大小
    bool disable_ui_sounds;     // 禁用UI声音
    
    // 系统优化
    bool boost_thread_priority; // 提升线程优先级
    bool disable_app_nap;       // 禁用App Nap
} audio_macos_optimization_t;

// 应用macOS优化
audio_result_t apply_macos_optimizations(const audio_macos_optimization_t* opt) {
    // 设置线程优先级
    if (opt->boost_thread_priority) {
        thread_time_constraint_policy_data_t policy;
        policy.period = 2902;       // 约1ms @ 44.1kHz
        policy.computation = 1451;  // 50%的周期
        policy.constraint = 2902;
        policy.preemptible = TRUE;
        
        thread_policy_set(mach_thread_self(),
                         THREAD_TIME_CONSTRAINT_POLICY,
                         (thread_policy_t)&policy,
                         THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    }
    
    // 禁用App Nap
    if (opt->disable_app_nap) {
        // 通过NSProcessInfo禁用App Nap
        // 需要Objective-C代码
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 3. ESP32优化

```c
// ESP32特定优化
typedef struct audio_esp32_optimization {
    // I2S优化
    uint32_t dma_buffer_count;  // DMA缓冲区数量
    uint32_t dma_buffer_len;    // DMA缓冲区长度
    bool use_apll;              // 使用APLL时钟
    
    // 内存优化
    bool use_psram;             // 使用PSRAM
    bool enable_cache;          // 启用缓存
    
    // CPU优化
    uint32_t cpu_frequency_mhz; // CPU频率
    bool pin_to_core;           // 绑定到特定核心
    uint32_t core_id;           // 核心ID
} audio_esp32_optimization_t;

// 应用ESP32优化
audio_result_t apply_esp32_optimizations(const audio_esp32_optimization_t* opt) {
    // 设置CPU频率
    if (opt->cpu_frequency_mhz > 0) {
        esp_pm_config_esp32_t pm_config = {
            .max_freq_mhz = opt->cpu_frequency_mhz,
            .min_freq_mhz = opt->cpu_frequency_mhz,
            .light_sleep_enable = false
        };
        esp_pm_configure(&pm_config);
    }
    
    // 绑定任务到特定核心
    if (opt->pin_to_core) {
        TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
        vTaskCoreAffinitySet(current_task, 1 << opt->core_id);
    }
    
    // 配置I2S DMA
    if (opt->dma_buffer_count > 0) {
        // 配置I2S DMA参数
        // 具体实现依赖于ESP-IDF版本
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

---

## 性能监控

### 1. 实时性能指标

```c
// 性能指标收集器
typedef struct audio_perf_collector {
    // 基本指标
    uint64_t samples_processed;
    uint64_t bytes_processed;
    uint64_t processing_time_us;
    
    // 延迟指标
    uint32_t input_latency_us;
    uint32_t output_latency_us;
    uint32_t processing_latency_us;
    
    // 质量指标
    uint32_t underruns;
    uint32_t overruns;
    uint32_t glitches;
    
    // 资源使用
    float cpu_usage_percent;
    size_t memory_usage_bytes;
    uint32_t thread_count;
    
    // 时间戳
    uint64_t start_time;
    uint64_t last_update_time;
    
    // 采样配置
    uint32_t sample_interval_ms;
    bool enabled;
} audio_perf_collector_t;

// 更新性能指标
void audio_perf_update(audio_perf_collector_t* collector) {
    uint64_t current_time = audio_get_time_us();
    
    if (!collector->enabled) {
        return;
    }
    
    // 检查采样间隔
    if (current_time - collector->last_update_time < 
        collector->sample_interval_ms * 1000) {
        return;
    }
    
    // 更新CPU使用率
    collector->cpu_usage_percent = audio_get_cpu_usage();
    
    // 更新内存使用
    collector->memory_usage_bytes = audio_get_memory_usage();
    
    // 更新线程数
    collector->thread_count = audio_get_thread_count();
    
    collector->last_update_time = current_time;
}
```

### 2. 性能分析工具

```c
// 性能分析器
typedef struct audio_performance_analyzer {
    // 分析配置
    bool enable_profiling;
    bool enable_tracing;
    uint32_t trace_buffer_size;
    
    // 分析数据
    audio_perf_timer_t* function_timers;
    uint32_t timer_count;
    
    // 热点分析
    struct {
        char function_name[64];
        uint64_t total_time;
        uint32_t call_count;
        float percentage;
    } hotspots[AUDIO_MAX_HOTSPOTS];
    uint32_t hotspot_count;
    
    // 输出配置
    char output_file[256];
    bool real_time_output;
} audio_performance_analyzer_t;

// 生成性能报告
audio_result_t audio_generate_performance_report(
    const audio_performance_analyzer_t* analyzer,
    const char* output_file) {
    
    FILE* fp = fopen(output_file, "w");
    if (!fp) {
        return AUDIO_RESULT_FILE_ERROR;
    }
    
    fprintf(fp, "LinxOS Audio Performance Report\n");
    fprintf(fp, "================================\n\n");
    
    // 函数性能统计
    fprintf(fp, "Function Performance:\n");
    fprintf(fp, "%-32s %12s %12s %12s %8s\n",
            "Function", "Total(us)", "Calls", "Avg(us)", "Percent");
    fprintf(fp, "%-32s %12s %12s %12s %8s\n",
            "--------", "--------", "-----", "------", "-------");
    
    uint64_t total_time = 0;
    for (uint32_t i = 0; i < analyzer->timer_count; i++) {
        total_time += analyzer->function_timers[i].total_time;
    }
    
    for (uint32_t i = 0; i < analyzer->timer_count; i++) {
        const audio_perf_timer_t* timer = &analyzer->function_timers[i];
        float percentage = (float)timer->total_time / total_time * 100.0f;
        
        fprintf(fp, "%-32s %12llu %12u %12llu %7.2f%%\n",
                "function_name",  // 需要添加函数名存储
                timer->total_time,
                timer->call_count,
                timer->avg_time,
                percentage);
    }
    
    // 热点分析
    fprintf(fp, "\nHotspot Analysis:\n");
    for (uint32_t i = 0; i < analyzer->hotspot_count; i++) {
        fprintf(fp, "%d. %s (%.2f%%)\n",
                i + 1,
                analyzer->hotspots[i].function_name,
                analyzer->hotspots[i].percentage);
    }
    
    fclose(fp);
    return AUDIO_RESULT_SUCCESS;
}
```

### 3. 实时监控界面

```c
// 实时监控数据
typedef struct audio_realtime_monitor_data {
    // 当前状态
    float cpu_usage;
    float memory_usage_mb;
    uint32_t active_streams;
    uint32_t current_latency_us;
    
    // 历史数据
    float cpu_history[AUDIO_MONITOR_HISTORY_SIZE];
    float latency_history[AUDIO_MONITOR_HISTORY_SIZE];
    uint32_t history_index;
    
    // 警告状态
    bool cpu_warning;
    bool latency_warning;
    bool memory_warning;
    
    // 阈值
    float cpu_warning_threshold;
    uint32_t latency_warning_threshold;
    float memory_warning_threshold;
} audio_realtime_monitor_data_t;

// 更新监控数据
void audio_update_monitor_data(audio_realtime_monitor_data_t* data) {
    // 获取当前指标
    data->cpu_usage = audio_get_cpu_usage();
    data->memory_usage_mb = audio_get_memory_usage() / (1024.0f * 1024.0f);
    data->active_streams = audio_get_active_stream_count();
    data->current_latency_us = audio_get_current_latency();
    
    // 更新历史数据
    data->cpu_history[data->history_index] = data->cpu_usage;
    data->latency_history[data->history_index] = data->current_latency_us;
    data->history_index = (data->history_index + 1) % AUDIO_MONITOR_HISTORY_SIZE;
    
    // 检查警告条件
    data->cpu_warning = (data->cpu_usage > data->cpu_warning_threshold);
    data->latency_warning = (data->current_latency_us > data->latency_warning_threshold);
    data->memory_warning = (data->memory_usage_mb > data->memory_warning_threshold);
}
```

---

## 故障排除

### 1. 常见性能问题

```c
// 性能问题诊断
typedef enum {
    AUDIO_PERF_ISSUE_NONE = 0,
    AUDIO_PERF_ISSUE_HIGH_LATENCY,
    AUDIO_PERF_ISSUE_HIGH_CPU,
    AUDIO_PERF_ISSUE_MEMORY_LEAK,
    AUDIO_PERF_ISSUE_UNDERRUNS,
    AUDIO_PERF_ISSUE_THREAD_CONTENTION,
    AUDIO_PERF_ISSUE_IO_BOTTLENECK
} audio_perf_issue_t;

// 性能问题诊断器
typedef struct audio_perf_diagnostics {
    audio_perf_issue_t detected_issues[AUDIO_MAX_ISSUES];
    uint32_t issue_count;
    
    // 诊断数据
    float avg_cpu_usage;
    uint32_t avg_latency_us;
    size_t memory_growth_rate;
    uint32_t underrun_rate;
    uint32_t context_switch_rate;
    
    // 建议
    char recommendations[AUDIO_MAX_ISSUES][256];
} audio_perf_diagnostics_t;

// 执行性能诊断
audio_result_t audio_diagnose_performance(audio_perf_diagnostics_t* diagnostics) {
    diagnostics->issue_count = 0;
    
    // 检查高延迟
    if (diagnostics->avg_latency_us > 20000) {  // 20ms
        diagnostics->detected_issues[diagnostics->issue_count++] = 
            AUDIO_PERF_ISSUE_HIGH_LATENCY;
        strcpy(diagnostics->recommendations[diagnostics->issue_count - 1],
               "Reduce buffer size or optimize processing pipeline");
    }
    
    // 检查高CPU使用率
    if (diagnostics->avg_cpu_usage > 80.0f) {
        diagnostics->detected_issues[diagnostics->issue_count++] = 
            AUDIO_PERF_ISSUE_HIGH_CPU;
        strcpy(diagnostics->recommendations[diagnostics->issue_count - 1],
               "Enable SIMD optimizations or reduce processing complexity");
    }
    
    // 检查内存泄漏
    if (diagnostics->memory_growth_rate > 1024 * 1024) {  // 1MB/s
        diagnostics->detected_issues[diagnostics->issue_count++] = 
            AUDIO_PERF_ISSUE_MEMORY_LEAK;
        strcpy(diagnostics->recommendations[diagnostics->issue_count - 1],
               "Check for memory leaks in audio processing code");
    }
    
    // 检查缓冲区下溢
    if (diagnostics->underrun_rate > 10) {  // 每秒10次
        diagnostics->detected_issues[diagnostics->issue_count++] = 
            AUDIO_PERF_ISSUE_UNDERRUNS;
        strcpy(diagnostics->recommendations[diagnostics->issue_count - 1],
               "Increase buffer size or improve real-time scheduling");
    }
    
    return AUDIO_RESULT_SUCCESS;
}
```

### 2. 性能调试工具

```c
// 性能调试器
typedef struct audio_perf_debugger {
    bool enabled;
    
    // 断点
    struct {
        char function_name[64];
        uint32_t line_number;
        bool enabled;
        uint32_t hit_count;
    } breakpoints[AUDIO_MAX_BREAKPOINTS];
    uint32_t breakpoint_count;
    
    // 监视点
    struct {
        void* address;
        size_t size;
        bool enabled;
        uint32_t access_count;
    } watchpoints[AUDIO_MAX_WATCHPOINTS];
    uint32_t watchpoint_count;
    
    // 调用栈
    void* call_stack[AUDIO_MAX_STACK_DEPTH];
    uint32_t stack_depth;
} audio_perf_debugger_t;

// 性能断点
#define AUDIO_PERF_BREAKPOINT(debugger, func_name) \
    do { \
        if ((debugger)->enabled) { \
            audio_perf_hit_breakpoint(debugger, func_name, __LINE__); \
        } \
    } while(0)

// 内存访问监视
void audio_perf_watch_memory(audio_perf_debugger_t* debugger,
                            void* address, size_t size) {
    if (debugger->watchpoint_count < AUDIO_MAX_WATCHPOINTS) {
        debugger->watchpoints[debugger->watchpoint_count].address = address;
        debugger->watchpoints[debugger->watchpoint_count].size = size;
        debugger->watchpoints[debugger->watchpoint_count].enabled = true;
        debugger->watchpoints[debugger->watchpoint_count].access_count = 0;
        debugger->watchpoint_count++;
    }
}
```

---

## 最佳实践

### 1.