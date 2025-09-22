/**
 * @file play_audio_test.c
 * @brief 简化版 Linx Player 音频播放测试程序
 * @author Linx Team
 * @date 2024
 * 
 * 本测试程序提供简洁的音频播放功能测试：
 * - 播放器基本功能测试
 * - 音频数据输入和播放
 * - 状态管理和错误处理
 * - Opus 文件播放支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>

#include "../linx_player.h"
#include "../../audio/audio_interface.h"
#include "../../audio/portaudio_mac.h"
#include "../../codecs/audio_codec.h"
#include "../../codecs/codec_stub.h"
#include "../../codecs/opus_codec.h"
#include "../../log/linx_log.h"

// 测试配置常量
#define TEST_SAMPLE_RATE    16000   // 与 linx_demo.c 一致
#define TEST_CHANNELS       1
#define TEST_FRAME_SIZE     320     // 20ms @ 16kHz，与 linx_demo.c 一致
#define TEST_BUFFER_SIZE    8192
#define TEST_TONE_FREQ      440.0   // A4音符频率
#define TEST_DATA_SIZE      512
#define TEST_DURATION_SEC   5

// Opus 文件播放相关常量
#define OPUS_FRAME_SIZE_MS  20      // 20ms 帧
#define OPUS_MAX_PACKET_SIZE 4000   // Opus 最大包大小
#define OPUS_MAX_FRAME_SIZE 5760    // 120ms @ 48kHz
#define READ_BUFFER_SIZE    8192    // 增大缓冲区以支持 Ogg 页面

// Ogg 页面相关常量
#define OGG_PAGE_HEADER_SIZE 27     // Ogg 页面头部固定大小
#define OGG_CAPTURE_PATTERN "OggS"  // Ogg 页面标识
#define OGG_MAX_SEGMENTS 255        // 最大段数
#define OGG_MAX_PAGE_SIZE 65307     // 最大页面大小

// Ogg 页面头部结构
typedef struct {
    uint8_t capture_pattern[4];     // "OggS"
    uint8_t version;                // 版本号
    uint8_t header_type;            // 头部类型
    uint64_t granule_position;      // 颗粒位置
    uint32_t serial_number;         // 流序列号
    uint32_t page_sequence;         // 页面序列号
    uint32_t checksum;              // CRC 校验和
    uint8_t page_segments;          // 段数
    uint8_t segment_table[OGG_MAX_SEGMENTS]; // 段表
} ogg_page_header_t;

// Opus 文件信息结构体
typedef struct {
    FILE* file;
    long file_size;
    long bytes_read;
    uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    size_t buffer_len;
    
    // Ogg 解析状态
    bool is_ogg_format;             // 是否为 Ogg 格式
    ogg_page_header_t current_page; // 当前页面头部
    size_t page_data_pos;           // 当前页面数据位置
    size_t page_data_len;           // 当前页面数据长度
    uint8_t* page_data;             // 页面数据缓冲区
    bool header_parsed;             // 是否已解析头部
} opus_file_reader_t;

// 全局变量
static volatile bool g_running = true;
static linx_player_t* g_player = NULL;

/**
 * 信号处理函数 - 优雅退出
 */
static void signal_handler(int sig) {
    printf("\n[INFO] 收到退出信号 %d，正在停止测试...\n", sig);
    g_running = false;
}

/**
 * 播放器状态变化回调函数
 */
static void on_player_state_changed(player_state_t old_state, player_state_t new_state, void* user_data) {
    const char* state_names[] = {"IDLE", "PLAYING", "PAUSED", "STOPPED", "ERROR"};
    printf("[STATE] %s -> %s\n", state_names[old_state], state_names[new_state]);
}

/**
 * 生成测试音频数据（正弦波）
 */
static void generate_sine_wave(uint8_t* buffer, size_t size, double frequency, double* phase) {
    for (size_t i = 0; i < size; i++) {
        // 生成正弦波样本
        double sample = sin(*phase) * 127.0;
        buffer[i] = (uint8_t)(sample + 128);
        
        // 更新相位
        *phase += 2.0 * M_PI * frequency / TEST_SAMPLE_RATE;
        if (*phase >= 2.0 * M_PI) {
            *phase -= 2.0 * M_PI;
        }
    }
}

/**
 * 检查是否为 Ogg 格式文件
 */
static bool is_ogg_file(FILE* file) {
    uint8_t header[4];
    long pos = ftell(file);
    
    if (fread(header, 1, 4, file) != 4) {
        fseek(file, pos, SEEK_SET);
        return false;
    }
    
    fseek(file, pos, SEEK_SET);
    return memcmp(header, OGG_CAPTURE_PATTERN, 4) == 0;
}

/**
 * 读取 Ogg 页面头部
 */
static int read_ogg_page_header(opus_file_reader_t* reader, ogg_page_header_t* header) {
    uint8_t header_data[OGG_PAGE_HEADER_SIZE];
    
    if (fread(header_data, 1, OGG_PAGE_HEADER_SIZE, reader->file) != OGG_PAGE_HEADER_SIZE) {
        return -1;
    }
    
    // 解析头部
    memcpy(header->capture_pattern, header_data, 4);
    header->version = header_data[4];
    header->header_type = header_data[5];
    
    // 小端序读取 64 位颗粒位置
    header->granule_position = 0;
    for (int i = 0; i < 8; i++) {
        header->granule_position |= ((uint64_t)header_data[6 + i]) << (i * 8);
    }
    
    // 小端序读取 32 位值
    header->serial_number = 0;
    for (int i = 0; i < 4; i++) {
        header->serial_number |= ((uint32_t)header_data[14 + i]) << (i * 8);
    }
    
    header->page_sequence = 0;
    for (int i = 0; i < 4; i++) {
        header->page_sequence |= ((uint32_t)header_data[18 + i]) << (i * 8);
    }
    
    header->checksum = 0;
    for (int i = 0; i < 4; i++) {
        header->checksum |= ((uint32_t)header_data[22 + i]) << (i * 8);
    }
    
    header->page_segments = header_data[26];
    
    // 读取段表
    if (header->page_segments > 0) {
        if (fread(header->segment_table, 1, header->page_segments, reader->file) != header->page_segments) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * 读取 Ogg 页面数据
 */
static int read_ogg_page_data(opus_file_reader_t* reader) {
    ogg_page_header_t* header = &reader->current_page;
    
    // 计算页面数据总大小
    size_t total_size = 0;
    for (int i = 0; i < header->page_segments; i++) {
        total_size += header->segment_table[i];
    }
    
    if (total_size == 0) {
        reader->page_data_len = 0;
        return 0;
    }
    
    // 确保页面数据缓冲区足够大
    if (!reader->page_data || total_size > OGG_MAX_PAGE_SIZE) {
        if (reader->page_data) {
            free(reader->page_data);
        }
        reader->page_data = malloc(OGG_MAX_PAGE_SIZE);
        if (!reader->page_data) {
            return -1;
        }
    }
    
    // 读取页面数据
    if (fread(reader->page_data, 1, total_size, reader->file) != total_size) {
        return -1;
    }
    
    reader->page_data_len = total_size;
    reader->page_data_pos = 0;
    reader->bytes_read += OGG_PAGE_HEADER_SIZE + header->page_segments + total_size;
    
    return 0;
}

/**
 * 从当前 Ogg 页面提取下一个 Opus 包
 */
static int extract_opus_packet_from_page(opus_file_reader_t* reader, uint8_t* packet, size_t max_size) {
    if (reader->page_data_pos >= reader->page_data_len) {
        return 0; // 当前页面数据已读完
    }
    
    ogg_page_header_t* header = &reader->current_page;
    
    // 找到当前位置对应的段
    size_t current_segment = 0;
    size_t pos = 0;
    
    for (int i = 0; i < header->page_segments; i++) {
        if (pos == reader->page_data_pos) {
            current_segment = i;
            break;
        }
        pos += header->segment_table[i];
        if (pos > reader->page_data_pos) {
            // 位置在段中间，说明有问题
            return 0;
        }
    }
    
    if (current_segment >= header->page_segments) {
        return 0;
    }
    
    // 计算完整包的大小（可能跨越多个段）
    size_t packet_size = 0;
    size_t segments_used = 0;
    
    // Opus 包可能由多个 255 字节的段组成，最后一个段小于 255 字节表示包结束
    for (int i = current_segment; i < header->page_segments; i++) {
        packet_size += header->segment_table[i];
        segments_used++;
        
        // 如果段大小小于 255，说明包结束
        if (header->segment_table[i] < 255) {
            break;
        }
    }
    
    // 检查包大小是否合理
    if (packet_size == 0 || packet_size > max_size || packet_size > OPUS_MAX_PACKET_SIZE) {
        // 如果包太大，只取一个段的数据
        packet_size = header->segment_table[current_segment];
        if (packet_size > max_size) {
            packet_size = max_size;
        }
        segments_used = 1;
    }
    
    // 复制包数据
    memcpy(packet, reader->page_data + reader->page_data_pos, packet_size);
    
    // 更新位置
    reader->page_data_pos += packet_size;
    
    return (int)packet_size;
}

/**
 * 初始化 Opus 文件读取器
 */
static opus_file_reader_t* opus_file_reader_create(const char* filename) {
    if (!filename) {
        printf("[ERROR] 文件名为空\n");
        return NULL;
    }
    
    opus_file_reader_t* reader = malloc(sizeof(opus_file_reader_t));
    if (!reader) {
        printf("[ERROR] 内存分配失败\n");
        return NULL;
    }
    
    memset(reader, 0, sizeof(opus_file_reader_t));
    
    // 打开文件
    reader->file = fopen(filename, "rb");
    if (!reader->file) {
        printf("[ERROR] 无法打开文件: %s\n", filename);
        free(reader);
        return NULL;
    }
    
    // 获取文件大小
    fseek(reader->file, 0, SEEK_END);
    reader->file_size = ftell(reader->file);
    fseek(reader->file, 0, SEEK_SET);
    
    // 检查文件格式
    reader->is_ogg_format = is_ogg_file(reader->file);
    
    // 分配读取缓冲区
    reader->buffer_size = READ_BUFFER_SIZE;
    reader->buffer = malloc(reader->buffer_size);
    if (!reader->buffer) {
        printf("[ERROR] 缓冲区内存分配失败\n");
        fclose(reader->file);
        free(reader);
        return NULL;
    }
    
    // 如果是 Ogg 格式，跳过头部页面（OpusHead 和 OpusTags）
    if (reader->is_ogg_format) {
        printf("[INFO] 检测到 Ogg Opus 格式文件\n");
        
        // 读取并跳过 OpusHead 页面
        if (read_ogg_page_header(reader, &reader->current_page) == 0) {
            if (read_ogg_page_data(reader) == 0) {
                printf("[INFO] 跳过 OpusHead 页面 (%zu 字节)\n", reader->page_data_len);
            }
        }
        
        // 读取并跳过 OpusTags 页面
        if (read_ogg_page_header(reader, &reader->current_page) == 0) {
            if (read_ogg_page_data(reader) == 0) {
                printf("[INFO] 跳过 OpusTags 页面 (%zu 字节)\n", reader->page_data_len);
            }
        }
        
        reader->header_parsed = true;
        printf("[INFO] Ogg Opus 头部解析完成，准备读取音频数据\n");
    } else {
        printf("[INFO] 检测到原始 Opus 格式文件\n");
    }
    
    printf("[INFO] Opus 文件打开成功: %s (大小: %ld 字节, 格式: %s)\n", 
           filename, reader->file_size, reader->is_ogg_format ? "Ogg Opus" : "Raw Opus");
    return reader;
}

/**
 * 销毁 Opus 文件读取器
 */
static void opus_file_reader_destroy(opus_file_reader_t* reader) {
    if (!reader) return;
    
    if (reader->file) {
        fclose(reader->file);
    }
    if (reader->buffer) {
        free(reader->buffer);
    }
    if (reader->page_data) {
        free(reader->page_data);
    }
    free(reader);
}

/**
 * 从 Opus 文件读取下一个数据包
 * 支持 Ogg Opus 和原始 Opus 格式
 */
static int opus_file_reader_read_packet(opus_file_reader_t* reader, uint8_t* packet, size_t max_size) {
    if (!reader || !packet || !reader->file) {
        return -1;
    }
    
    if (reader->is_ogg_format) {
        // Ogg Opus 格式处理
        while (true) {
            // 尝试从当前页面提取包
            int packet_size = extract_opus_packet_from_page(reader, packet, max_size);
            if (packet_size > 0) {
                return packet_size;
            }
            
            // 当前页面已读完，读取下一个页面
            if (read_ogg_page_header(reader, &reader->current_page) != 0) {
                return 0; // 文件结束或错误
            }
            
            // 检查是否为有效的 Ogg 页面
            if (memcmp(reader->current_page.capture_pattern, OGG_CAPTURE_PATTERN, 4) != 0) {
                printf("[WARN] 无效的 Ogg 页面标识\n");
                return -1;
            }
            
            // 读取页面数据
            if (read_ogg_page_data(reader) != 0) {
                printf("[WARN] 读取 Ogg 页面数据失败\n");
                return -1;
            }
            
            // 如果页面没有数据，继续读取下一个页面
            if (reader->page_data_len == 0) {
                continue;
            }
        }
    } else {
        // 原始 Opus 格式处理（简化实现）
        if (reader->buffer_pos >= reader->buffer_len) {
            reader->buffer_len = fread(reader->buffer, 1, reader->buffer_size, reader->file);
            reader->buffer_pos = 0;
            reader->bytes_read += reader->buffer_len;
            
            if (reader->buffer_len == 0) {
                return 0; // 文件结束
            }
        }
        
        // 对于原始格式，假设每次读取固定大小的数据作为一个 Opus 包
        // 这是一个简化实现，实际应用中需要更复杂的包边界检测
        size_t packet_size = (reader->buffer_len - reader->buffer_pos);
        if (packet_size > max_size) {
            packet_size = max_size;
        }
        if (packet_size > OPUS_MAX_PACKET_SIZE) {
            packet_size = OPUS_MAX_PACKET_SIZE;
        }
        
        memcpy(packet, reader->buffer + reader->buffer_pos, packet_size);
        reader->buffer_pos += packet_size;
        
        return (int)packet_size;
    }
}

/**
 * 创建和初始化播放器（使用 Opus 编解码器）
 */
static linx_player_t* create_opus_player(void) {
    // 创建音频接口
    AudioInterface* audio_interface = portaudio_mac_create();
    if (!audio_interface) {
        printf("[ERROR] 创建音频接口失败\n");
        return NULL;
    }
    
    // 创建 Opus 编解码器
    audio_codec_t* codec = opus_codec_create();
    if (!codec) {
        printf("[ERROR] 创建 Opus 编解码器失败\n");
        audio_interface->vtable->destroy(audio_interface);
        return NULL;
    }
    
    // 创建播放器
    linx_player_t* player = linx_player_create(audio_interface, codec);
    if (!player) {
        printf("[ERROR] 创建播放器失败\n");
        codec->vtable->destroy(codec);
        audio_interface->vtable->destroy(audio_interface);
        return NULL;
    }
    
    // 配置播放器
    player_audio_config_t config = {
        .sample_rate = TEST_SAMPLE_RATE,
        .channels = TEST_CHANNELS,
        .frame_size = TEST_FRAME_SIZE,
        .buffer_size = TEST_BUFFER_SIZE
    };
    
    player_error_t error = linx_player_init(player, &config);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 播放器初始化失败: %s\n", linx_player_error_string(error));
        linx_player_destroy(player);
        return NULL;
    }
    
    // 设置状态回调
    linx_player_set_event_callback(player, on_player_state_changed, NULL);
    
    printf("[INFO] Opus 播放器创建并初始化成功\n");
    return player;
}

/**
 * 创建和初始化播放器（使用 stub 编解码器，用于正弦波测试）
 */
static linx_player_t* create_player(void) {
    // 创建音频接口
    AudioInterface* audio_interface = portaudio_mac_create();
    if (!audio_interface) {
        printf("[ERROR] 创建音频接口失败\n");
        return NULL;
    }
    
    // 创建编解码器
    audio_codec_t* codec = codec_stub_create();
    if (!codec) {
        printf("[ERROR] 创建编解码器失败\n");
        audio_interface->vtable->destroy(audio_interface);
        return NULL;
    }
    
    // 创建播放器
    linx_player_t* player = linx_player_create(audio_interface, codec);
    if (!player) {
        printf("[ERROR] 创建播放器失败\n");
        codec->vtable->destroy(codec);
        audio_interface->vtable->destroy(audio_interface);
        return NULL;
    }
    
    // 配置播放器
    player_audio_config_t config = {
        .sample_rate = TEST_SAMPLE_RATE,
        .channels = TEST_CHANNELS,
        .frame_size = TEST_FRAME_SIZE,
        .buffer_size = TEST_BUFFER_SIZE
    };
    
    player_error_t error = linx_player_init(player, &config);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 播放器初始化失败: %s\n", linx_player_error_string(error));
        linx_player_destroy(player);
        return NULL;
    }
    
    // 设置状态回调
    linx_player_set_event_callback(player, on_player_state_changed, NULL);
    
    printf("[INFO] 播放器创建并初始化成功\n");
    return player;
}

/**
 * 测试播放器基本功能
 */
static bool test_basic_playback(linx_player_t* player) {
    printf("\n=== 开始基本播放测试 ===\n");
    
    // 启动播放器
    player_error_t error = linx_player_start(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 启动播放器失败: %s\n", linx_player_error_string(error));
        return false;
    }
    
    printf("[INFO] 播放器启动成功，开始生成音频数据...\n");
    
    // 生成并输入音频数据
    uint8_t audio_data[TEST_DATA_SIZE];
    double phase = 0.0;
    int data_count = 0;
    
    while (g_running && data_count < (TEST_DURATION_SEC * 10)) {  // 约5秒的数据
        // 生成测试音频数据
        generate_sine_wave(audio_data, sizeof(audio_data), TEST_TONE_FREQ, &phase);
        
        // 输入到播放器
        error = linx_player_feed_data(player, audio_data, sizeof(audio_data));
        if (error != PLAYER_SUCCESS) {
            printf("[WARN] 输入音频数据失败: %s\n", linx_player_error_string(error));
        }
        
        // 显示缓冲区状态
        float buffer_usage = linx_player_get_buffer_usage(player);
        printf("\r[INFO] 缓冲区使用率: %.1f%% | 数据包: %d", 
               buffer_usage * 100.0f, ++data_count);
        fflush(stdout);
        
        usleep(100000);  // 100ms间隔
    }
    
    printf("\n[INFO] 音频数据输入完成\n");
    
    // 等待播放完成
    printf("[INFO] 等待播放完成...\n");
    sleep(2);
    
    // 获取播放统计
    size_t total_bytes, total_frames;
    linx_player_get_stats(player, &total_bytes, &total_frames);
    printf("[STATS] 播放统计 - 字节数: %zu, 帧数: %zu\n", total_bytes, total_frames);
    
    return true;
}

/**
 * 测试播放器状态控制
 */
static bool test_state_control(linx_player_t* player) {
    printf("\n=== 开始状态控制测试 ===\n");
    
    player_error_t error;
    
    // 测试暂停
    printf("[TEST] 暂停播放器...\n");
    error = linx_player_pause(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 暂停失败: %s\n", linx_player_error_string(error));
        return false;
    }
    sleep(1);
    
    // 测试恢复
    printf("[TEST] 恢复播放器...\n");
    error = linx_player_resume(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 恢复失败: %s\n", linx_player_error_string(error));
        return false;
    }
    sleep(1);
    
    // 测试停止
    printf("[TEST] 停止播放器...\n");
    error = linx_player_stop(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 停止失败: %s\n", linx_player_error_string(error));
        return false;
    }
    
    printf("[INFO] 状态控制测试完成\n");
    return true;
}

/**
 * 测试缓冲区管理
 */
static bool test_buffer_management(linx_player_t* player) {
    printf("\n=== 开始缓冲区管理测试 ===\n");
    
    // 检查初始状态
    printf("[INFO] 初始缓冲区状态:\n");
    printf("  - 为空: %s\n", linx_player_is_buffer_empty(player) ? "是" : "否");
    printf("  - 已满: %s\n", linx_player_is_buffer_full(player) ? "是" : "否");
    printf("  - 使用率: %.1f%%\n", linx_player_get_buffer_usage(player) * 100.0f);
    
    // 填充一些数据
    uint8_t test_data[256];
    memset(test_data, 0x55, sizeof(test_data));
    
    printf("[TEST] 填充测试数据...\n");
    for (int i = 0; i < 5; i++) {
        player_error_t error = linx_player_feed_data(player, test_data, sizeof(test_data));
        if (error != PLAYER_SUCCESS) {
            printf("[WARN] 第%d次填充失败: %s\n", i+1, linx_player_error_string(error));
        }
        printf("  填充 %d: 使用率 %.1f%%\n", i+1, linx_player_get_buffer_usage(player) * 100.0f);
    }
    
    // 清空缓冲区
    printf("[TEST] 清空缓冲区...\n");
    player_error_t error = linx_player_clear_buffer(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 清空缓冲区失败: %s\n", linx_player_error_string(error));
        return false;
    }
    
    printf("[INFO] 清空后使用率: %.1f%%\n", linx_player_get_buffer_usage(player) * 100.0f);
    printf("[INFO] 缓冲区管理测试完成\n");
    
    return true;
}

/**
 * 测试 Opus 文件播放
 */
static bool test_opus_file_playback(const char* opus_file_path) {
    printf("\n=== 开始 Opus 文件播放测试 ===\n");
    
    if (!opus_file_path) {
        printf("[ERROR] Opus 文件路径为空\n");
        return false;
    }
    
    // 创建 Opus 播放器
    linx_player_t* player = create_opus_player();
    if (!player) {
        printf("[ERROR] 创建 Opus 播放器失败\n");
        return false;
    }
    
    // 创建文件读取器
    opus_file_reader_t* reader = opus_file_reader_create(opus_file_path);
    if (!reader) {
        printf("[ERROR] 创建文件读取器失败\n");
        linx_player_destroy(player);
        return false;
    }
    
    // 启动播放器
    player_error_t error = linx_player_start(player);
    if (error != PLAYER_SUCCESS) {
        printf("[ERROR] 启动播放器失败: %s\n", linx_player_error_string(error));
        opus_file_reader_destroy(reader);
        linx_player_destroy(player);
        return false;
    }
    
    printf("[INFO] 开始播放 Opus 文件: %s\n", opus_file_path);
    
    // 读取并播放 Opus 数据
    uint8_t opus_packet[OPUS_MAX_PACKET_SIZE];
    int packets_processed = 0;
    int total_bytes = 0;
    
    while (g_running) {
        // 从文件读取 Opus 数据包
        int packet_size = opus_file_reader_read_packet(reader, opus_packet, sizeof(opus_packet));
        if (packet_size <= 0) {
            printf("\n[INFO] 文件读取完成或出错，停止播放\n");
            break;
        }
        
        // 将 Opus 数据包输入到播放器
        error = linx_player_feed_data(player, opus_packet, packet_size);
        if (error != PLAYER_SUCCESS) {
            printf("[WARN] 输入 Opus 数据失败: %s\n", linx_player_error_string(error));
        }
        
        packets_processed++;
        total_bytes += packet_size;
        
        // 显示播放进度
        float progress = (float)reader->bytes_read / reader->file_size * 100.0f;
        float buffer_usage = linx_player_get_buffer_usage(player);
        printf("\r[PLAY] 进度: %.1f%% | 缓冲区: %.1f%% | 包数: %d | 字节: %d", 
               progress, buffer_usage * 100.0f, packets_processed, total_bytes);
        fflush(stdout);
        
        // 控制播放速度
        usleep(20000);  // 20ms 间隔，模拟实时播放
        
        // 如果文件播放完成
        if (reader->bytes_read >= reader->file_size) {
            printf("\n[INFO] 文件播放完成\n");
            break;
        }
    }
    
    printf("\n[INFO] 等待播放缓冲区清空...\n");
    sleep(2);
    
    // 获取播放统计
    size_t total_play_bytes, total_frames;
    linx_player_get_stats(player, &total_play_bytes, &total_frames);
    printf("[STATS] 播放统计 - 处理包数: %d, 总字节: %d, 播放字节: %zu, 帧数: %zu\n", 
           packets_processed, total_bytes, total_play_bytes, total_frames);
    
    // 停止播放器
    linx_player_stop(player);
    
    // 清理资源
    opus_file_reader_destroy(reader);
    linx_player_destroy(player);
    
    printf("[INFO] Opus 文件播放测试完成\n");
    return true;
}

/**
 * 打印使用帮助
 */
static void print_usage(const char* program_name) {
    printf("Linx Player 音频播放测试程序\n");
    printf("用法: %s [选项] [Opus文件路径]\n\n", program_name);
    printf("选项:\n");
    printf("  -h, --help      显示此帮助信息\n");
    printf("  -b, --basic     运行基本播放测试（正弦波）\n");
    printf("  -s, --state     运行状态控制测试\n");
    printf("  -m, --buffer    运行缓冲区管理测试\n");
    printf("  -o, --opus      播放 Opus 文件（需要指定文件路径）\n");
    printf("  -a, --all       运行所有测试 (默认)\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s --opus /path/to/audio.opus    # 播放 Opus 文件\n", program_name);
    printf("  %s --basic                       # 运行正弦波播放测试\n", program_name);
    printf("  %s --all                         # 运行所有测试\n", program_name);
    printf("\n");
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_INFO;  // 默认INFO级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return 0;
    }
    

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("🎵 Linx Player 音频播放测试程序\n");
    printf("================================================\n");
    
    // 解析命令行参数
    bool run_basic = false;
    bool run_state = false;
    bool run_buffer = false;
    bool run_opus = false;
    bool run_all = false;
    const char* opus_file_path = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--basic") == 0) {
            run_basic = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--state") == 0) {
            run_state = true;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--buffer") == 0) {
            run_buffer = true;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--opus") == 0) {
            run_opus = true;
            // 下一个参数应该是文件路径
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                opus_file_path = argv[++i];
            } else {
                printf("[ERROR] --opus 选项需要指定 Opus 文件路径\n");
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            run_all = true;
        } else if (argv[i][0] != '-') {
            // 如果不是选项，可能是 Opus 文件路径
            if (!opus_file_path) {
                opus_file_path = argv[i];
                run_opus = true;
            }
        } else {
            printf("[ERROR] 未知选项: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 如果指定了 Opus 文件但没有 --opus 选项，自动启用 Opus 播放
    if (opus_file_path && !run_opus) {
        run_opus = true;
    }
    
    // 默认运行所有测试（除非指定了 Opus 文件）
    if (!run_basic && !run_state && !run_buffer && !run_opus && !run_all) {
        run_all = true;
    }
    
    bool all_tests_passed = true;
    
    // 如果要运行 Opus 文件播放测试
    if (run_opus) {
        if (!test_opus_file_playback(opus_file_path)) {
            all_tests_passed = false;
        }
    } else {
        // 创建播放器（用于其他测试）
        g_player = create_player();
        if (!g_player) {
            printf("[ERROR] 播放器创建失败，测试终止\n");
            return 1;
        }
        
        // 运行测试
        if (run_all || run_basic) {
            if (!test_basic_playback(g_player)) {
                all_tests_passed = false;
            }
        }
        
        if (run_all || run_state) {
            if (!test_state_control(g_player)) {
                all_tests_passed = false;
            }
        }
        
        if (run_all || run_buffer) {
            if (!test_buffer_management(g_player)) {
                all_tests_passed = false;
            }
        }
        
        // 清理资源
        if (g_player) {
            linx_player_destroy(g_player);
            g_player = NULL;
        }
    }
    
    // 输出测试结果
    printf("\n================================================\n");
    if (all_tests_passed) {
        printf("✅ 所有测试通过！\n");
    } else {
        printf("❌ 部分测试失败！\n");
    }
    printf("🎵 Linx Player 测试程序结束\n");
    
    return all_tests_passed ? 0 : 1;
}