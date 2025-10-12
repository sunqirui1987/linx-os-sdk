/**
 * @file audio_driver.h
 * @brief LinxOS音频驱动接口
 * 
 * 音频驱动层提供硬件抽象接口，负责：
 * 1. 音频硬件的抽象和管理
 * 2. 设备的枚举和配置
 * 3. 音频数据的传输
 * 4. 硬件特性的查询
 * 5. 电源管理和热插拔支持
 */

#ifndef LINX_AUDIO_DRIVER_H
#define LINX_AUDIO_DRIVER_H

#include "../core/types.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct linx_audio_driver linx_audio_driver_t;
typedef struct linx_audio_device linx_audio_device_t;
typedef struct linx_audio_device_info linx_audio_device_info_t;

/**
 * @brief 音频驱动类型
 */
typedef enum {
    LINX_AUDIO_DRIVER_TYPE_UNKNOWN = 0,   ///< 未知驱动
    LINX_AUDIO_DRIVER_TYPE_DUMMY,         ///< 虚拟驱动
    LINX_AUDIO_DRIVER_TYPE_COREAUDIO,     ///< CoreAudio驱动
    LINX_AUDIO_DRIVER_TYPE_ALSA,          ///< ALSA驱动
    LINX_AUDIO_DRIVER_TYPE_ESP32          ///< ESP32驱动
} linx_audio_driver_type_t;

/**
 * @brief 音频设备类型
 */
typedef enum {
    LINX_AUDIO_DEVICE_TYPE_UNKNOWN = 0,    ///< 未知设备
    LINX_AUDIO_DEVICE_TYPE_BUILTIN_SPEAKER, ///< 内置扬声器
    LINX_AUDIO_DEVICE_TYPE_BUILTIN_MIC,    ///< 内置麦克风
    LINX_AUDIO_DEVICE_TYPE_HEADPHONES,     ///< 耳机
    LINX_AUDIO_DEVICE_TYPE_HEADSET,        ///< 耳麦
    LINX_AUDIO_DEVICE_TYPE_USB_AUDIO,      ///< USB音频设备
    LINX_AUDIO_DEVICE_TYPE_BLUETOOTH,      ///< 蓝牙音频设备
    LINX_AUDIO_DEVICE_TYPE_HDMI,           ///< HDMI音频
    LINX_AUDIO_DEVICE_TYPE_SPDIF,          ///< SPDIF数字音频
    LINX_AUDIO_DEVICE_TYPE_VIRTUAL         ///< 虚拟设备
} linx_audio_device_type_t;

/**
 * @brief 音频设备方向
 */
typedef enum {
    LINX_AUDIO_DEVICE_DIRECTION_INPUT = 0x01,   ///< 输入设备
    LINX_AUDIO_DEVICE_DIRECTION_OUTPUT = 0x02,  ///< 输出设备
    LINX_AUDIO_DEVICE_DIRECTION_DUPLEX = 0x03   ///< 双工设备
} linx_audio_device_direction_t;

/**
 * @brief 音频设备状态
 */
typedef enum {
    LINX_AUDIO_DEVICE_STATE_UNKNOWN = 0,   ///< 未知状态
    LINX_AUDIO_DEVICE_STATE_UNPLUGGED,     ///< 未插入
    LINX_AUDIO_DEVICE_STATE_PLUGGED,       ///< 已插入
    LINX_AUDIO_DEVICE_STATE_ACTIVE,        ///< 活跃
    LINX_AUDIO_DEVICE_STATE_DISABLED,      ///< 禁用
    LINX_AUDIO_DEVICE_STATE_ERROR          ///< 错误状态
} linx_audio_device_state_t;

/**
 * @brief 音频设备能力
 */
typedef struct {
    // 格式支持
    linx_audio_format_t* supported_formats;    ///< 支持的格式列表
    uint32_t format_count;                ///< 格式数量
    
    // 采样率支持
    uint32_t* supported_sample_rates;     ///< 支持的采样率列表
    uint32_t sample_rate_count;           ///< 采样率数量
    uint32_t min_sample_rate;             ///< 最小采样率
    uint32_t max_sample_rate;             ///< 最大采样率
    
    // 通道支持
    uint32_t min_channels;                ///< 最小通道数
    uint32_t max_channels;                ///< 最大通道数
    linx_audio_channel_layout_t* supported_layouts; ///< 支持的通道布局
    uint32_t layout_count;                ///< 布局数量
    
    // 缓冲区支持
    uint32_t min_buffer_size;             ///< 最小缓冲区大小
    uint32_t max_buffer_size;             ///< 最大缓冲区大小
    uint32_t preferred_buffer_size;       ///< 推荐缓冲区大小
    
    // 延迟特性
    uint32_t min_latency_frames;          ///< 最小延迟（帧数）
    uint32_t max_latency_frames;          ///< 最大延迟（帧数）
    
    // 功能特性
    bool supports_volume_control;         ///< 支持音量控制
    bool supports_mute;                   ///< 支持静音
    bool supports_monitoring;             ///< 支持监听
    bool supports_exclusive_mode;         ///< 支持独占模式
    bool supports_shared_mode;            ///< 支持共享模式
} linx_audio_device_capabilities_t;

/**
 * @brief 音频设备信息
 */
struct linx_audio_device_info {
    // 基本信息
    uint32_t id;                          ///< 设备ID
    char name[128];                       ///< 设备名称
    char description[256];                ///< 设备描述
    char manufacturer[64];                ///< 制造商
    char driver_name[64];                 ///< 驱动名称
    
    // 设备属性
    linx_audio_device_type_t type;             ///< 设备类型
    linx_audio_device_direction_t direction;   ///< 设备方向
    linx_audio_device_state_t state;           ///< 设备状态
    
    // 硬件信息
    char hardware_id[128];                ///< 硬件ID
    char serial_number[64];               ///< 序列号
    uint32_t vendor_id;                   ///< 厂商ID
    uint32_t product_id;                  ///< 产品ID
    
    // 能力信息
    linx_audio_device_capabilities_t capabilities; ///< 设备能力
    
    // 默认格式
    linx_audio_format_info_t default_format;   ///< 默认格式
    
    // 标志
    bool is_default;                      ///< 是否为默认设备
    bool is_system;                       ///< 是否为系统设备
    bool is_removable;                    ///< 是否可移除
    bool is_wireless;                     ///< 是否为无线设备
};

/**
 * @brief 音频设备配置
 */
typedef struct {
    // 格式配置
    linx_audio_format_info_t format;           ///< 音频格式
    
    // 缓冲配置
    uint32_t buffer_size;                 ///< 缓冲区大小
    uint32_t buffer_count;                ///< 缓冲区数量
    uint32_t period_size;                 ///< 周期大小
    
    // 音量配置
    float volume;                         ///< 音量（0.0-1.0）
    bool muted;                           ///< 是否静音
    
    // 模式配置
    bool exclusive_mode;                  ///< 独占模式
    bool low_latency_mode;                ///< 低延迟模式
    
    // 回调函数
    linx_audio_data_callback_t data_callback;  ///< 数据回调
    linx_audio_event_callback_t event_callback; ///< 事件回调
    void* user_data;                      ///< 用户数据
} linx_audio_device_config_t;

/**
 * @brief 音频设备统计信息
 */
typedef struct {
    // 基本统计
    uint64_t total_frames_processed;      ///< 总处理帧数
    uint64_t total_bytes_processed;       ///< 总处理字节数
    uint32_t buffer_underruns;            ///< 缓冲区欠载次数
    uint32_t buffer_overruns;             ///< 缓冲区溢出次数
    
    // 时间统计
    uint64_t total_active_time_us;        ///< 总活跃时间（微秒）
    uint32_t average_latency_frames;      ///< 平均延迟（帧数）
    uint32_t peak_latency_frames;         ///< 峰值延迟（帧数）
    
    // 错误统计
    uint32_t hardware_errors;             ///< 硬件错误次数
    uint32_t timeout_errors;              ///< 超时错误次数
    uint32_t format_errors;               ///< 格式错误次数
} linx_audio_device_stats_t;

/**
 * @brief 音频设备结构
 */
struct linx_audio_device {
    // 基本信息
    linx_audio_device_info_t info;             ///< 设备信息
    linx_audio_device_config_t config;         ///< 设备配置
    linx_audio_device_stats_t stats;           ///< 统计信息
    
    // 驱动引用
    linx_audio_driver_t* driver;               ///< 驱动引用
    
    // 缓冲区管理
    linx_audio_buffer_t** buffers;             ///< 缓冲区数组
    uint32_t buffer_count;                ///< 缓冲区数量
    uint32_t current_buffer;              ///< 当前缓冲区索引
    
    // 同步对象
    pthread_mutex_t mutex;                ///< 互斥锁
    pthread_cond_t condition;             ///< 条件变量
    
    // 内部数据
    void* private_data;                   ///< 私有数据
};

/**
 * @brief 音频驱动虚函数表
 */
typedef struct {
    // 生命周期管理
    linx_audio_result_t (*initialize)(linx_audio_driver_t* driver);
    linx_audio_result_t (*deinitialize)(linx_audio_driver_t* driver);
    linx_audio_result_t (*start)(linx_audio_driver_t* driver);
    linx_audio_result_t (*stop)(linx_audio_driver_t* driver);
    
    // 设备管理
    linx_audio_result_t (*enumerate_devices)(linx_audio_driver_t* driver,
                                       linx_audio_device_info_t** devices,
                                       uint32_t* count);
    linx_audio_result_t (*get_device_info)(linx_audio_driver_t* driver,
                                     uint32_t device_id,
                                     linx_audio_device_info_t* info);
    linx_audio_result_t (*open_device)(linx_audio_driver_t* driver,
                                 uint32_t device_id,
                                 const linx_audio_device_config_t* config,
                                 linx_audio_device_t** device);
    linx_audio_result_t (*close_device)(linx_audio_driver_t* driver,
                                  linx_audio_device_t* device);
    
    // 设备控制
    linx_audio_result_t (*start_device)(linx_audio_driver_t* driver,
                                  linx_audio_device_t* device);
    linx_audio_result_t (*stop_device)(linx_audio_driver_t* driver,
                                 linx_audio_device_t* device);
    linx_audio_result_t (*pause_device)(linx_audio_driver_t* driver,
                                  linx_audio_device_t* device);
    linx_audio_result_t (*resume_device)(linx_audio_driver_t* driver,
                                   linx_audio_device_t* device);
    
    // 数据传输
    linx_audio_result_t (*read_data)(linx_audio_driver_t* driver,
                               linx_audio_device_t* device,
                               linx_audio_buffer_t* buffer);
    linx_audio_result_t (*write_data)(linx_audio_driver_t* driver,
                                linx_audio_device_t* device,
                                const linx_audio_buffer_t* buffer);
    
    // 配置管理
    linx_audio_result_t (*set_device_config)(linx_audio_driver_t* driver,
                                       linx_audio_device_t* device,
                                       const linx_audio_device_config_t* config);
    linx_audio_result_t (*get_device_config)(linx_audio_driver_t* driver,
                                       linx_audio_device_t* device,
                                       linx_audio_device_config_t* config);
    
    // 音量控制
    linx_audio_result_t (*set_volume)(linx_audio_driver_t* driver,
                                linx_audio_device_t* device,
                                float volume);
    linx_audio_result_t (*get_volume)(linx_audio_driver_t* driver,
                                linx_audio_device_t* device,
                                float* volume);
    linx_audio_result_t (*set_mute)(linx_audio_driver_t* driver,
                              linx_audio_device_t* device,
                              bool muted);
    linx_audio_result_t (*get_mute)(linx_audio_driver_t* driver,
                              linx_audio_device_t* device,
                              bool* muted);
    
    // 状态查询
    linx_audio_result_t (*get_device_state)(linx_audio_driver_t* driver,
                                      linx_audio_device_t* device,
                                      linx_audio_device_state_t* state);
    linx_audio_result_t (*get_device_stats)(linx_audio_driver_t* driver,
                                      linx_audio_device_t* device,
                                      linx_audio_device_stats_t* stats);
    linx_audio_result_t (*reset_device_stats)(linx_audio_driver_t* driver,
                                        linx_audio_device_t* device);
    
    // 延迟查询
    linx_audio_result_t (*get_latency)(linx_audio_driver_t* driver,
                                 linx_audio_device_t* device,
                                 uint32_t* latency_frames);
    
    // 事件处理
    linx_audio_result_t (*set_event_callback)(linx_audio_driver_t* driver,
                                        linx_audio_event_callback_t callback,
                                        void* user_data);
    
    // 电源管理
    linx_audio_result_t (*suspend)(linx_audio_driver_t* driver);
    linx_audio_result_t (*resume)(linx_audio_driver_t* driver);
    
    // 清理
    void (*destroy)(linx_audio_driver_t* driver);
} linx_audio_driver_vtable_t;

/**
 * @brief 音频驱动配置
 */
typedef struct {
    // 基本配置
    char name[64];                        ///< 驱动名称
    char version[32];                     ///< 驱动版本
    
    // 性能配置
    uint32_t max_devices;                 ///< 最大设备数量
    uint32_t polling_interval_ms;         ///< 轮询间隔（毫秒）
    bool enable_hot_plug;                 ///< 启用热插拔
    
    // 调试配置
    bool enable_debug_logging;            ///< 启用调试日志
    bool enable_performance_monitoring;   ///< 启用性能监控
} linx_audio_driver_config_t;

/**
 * @brief 音频驱动统计信息
 */
typedef struct {
    // 基本统计
    uint32_t total_devices;               ///< 总设备数量
    uint32_t active_devices;              ///< 活跃设备数量
    uint32_t hot_plug_events;             ///< 热插拔事件次数
    
    // 性能统计
    uint64_t total_data_transferred;      ///< 总传输数据量
    uint32_t average_latency_us;          ///< 平均延迟（微秒）
    uint32_t peak_latency_us;             ///< 峰值延迟（微秒）
    
    // 错误统计
    uint32_t hardware_errors;             ///< 硬件错误次数
    uint32_t timeout_errors;              ///< 超时错误次数
    uint32_t initialization_failures;     ///< 初始化失败次数
} linx_audio_driver_stats_t;

/**
 * @brief 音频驱动结构
 */
struct linx_audio_driver {
    // 基本信息
    uint32_t id;                          ///< 驱动ID
    char name[64];                        ///< 驱动名称
    char version[32];                     ///< 驱动版本
    
    // 虚函数表
    const linx_audio_driver_vtable_t* vtable;  ///< 虚函数表
    
    // 配置和统计
    linx_audio_driver_config_t config;         ///< 驱动配置
    linx_audio_driver_stats_t stats;           ///< 统计信息
    
    // 设备管理
    linx_audio_device_t** devices;             ///< 设备数组
    uint32_t device_count;                ///< 设备数量
    uint32_t max_devices;                 ///< 最大设备数量
    
    // 同步对象
    pthread_mutex_t mutex;                ///< 互斥锁
    pthread_cond_t condition;             ///< 条件变量
    
    // 监控线程
    pthread_t monitor_thread;             ///< 监控线程
    bool monitor_running;                 ///< 监控运行标志
    
    // 内部数据
    void* private_data;                   ///< 私有数据
};

// =============================================================================
// 音频驱动API
// =============================================================================

/**
 * @brief 创建音频驱动
 * @param type 驱动类型
 * @return 音频驱动实例，失败返回NULL
 */
linx_audio_driver_t* linx_audio_driver_create(linx_audio_driver_type_t type);

/**
 * @brief 销毁音频驱动
 * @param driver 音频驱动
 */
void linx_audio_driver_destroy(linx_audio_driver_t* driver);

/**
 * @brief 初始化音频驱动
 * @param driver 音频驱动
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_initialize(linx_audio_driver_t* driver);

/**
 * @brief 反初始化音频驱动
 * @param driver 音频驱动
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_deinitialize(linx_audio_driver_t* driver);

/**
 * @brief 启动音频驱动
 * @param driver 音频驱动
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_start(linx_audio_driver_t* driver);

/**
 * @brief 停止音频驱动
 * @param driver 音频驱动
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_stop(linx_audio_driver_t* driver);

// =============================================================================
// 设备管理API
// =============================================================================

/**
 * @brief 枚举音频设备
 * @param driver 音频驱动
 * @param devices 输出设备信息数组
 * @param count 输出设备数量
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_enumerate_devices(linx_audio_driver_t* driver,
                                             linx_audio_device_info_t** devices,
                                             uint32_t* count);

/**
 * @brief 获取设备信息
 * @param driver 音频驱动
 * @param device_id 设备ID
 * @param info 输出设备信息
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_get_device_info(linx_audio_driver_t* driver,
                                           uint32_t device_id,
                                           linx_audio_device_info_t* info);

/**
 * @brief 打开音频设备
 * @param driver 音频驱动
 * @param device_id 设备ID
 * @param config 设备配置
 * @param device 输出设备实例
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_open_device(linx_audio_driver_t* driver,
                                       uint32_t device_id,
                                       const linx_audio_device_config_t* config,
                                       linx_audio_device_t** device);

/**
 * @brief 关闭音频设备
 * @param driver 音频驱动
 * @param device 音频设备
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_close_device(linx_audio_driver_t* driver,
                                        linx_audio_device_t* device);

// =============================================================================
// 实用工具函数
// =============================================================================

/**
 * @brief 设备类型转字符串
 * @param type 设备类型
 * @return 类型字符串
 */
const char* linx_audio_device_type_to_string(linx_audio_device_type_t type);

/**
 * @brief 设备状态转字符串
 * @param state 设备状态
 * @return 状态字符串
 */
const char* linx_audio_device_state_to_string(linx_audio_device_state_t state);

/**
 * @brief 设备方向转字符串
 * @param direction 设备方向
 * @return 方向字符串
 */
const char* linx_audio_device_direction_to_string(linx_audio_device_direction_t direction);

/**
 * @brief 获取默认驱动配置
 * @param config 输出配置
 * @return 操作结果
 */
linx_audio_result_t linx_audio_driver_get_default_config(linx_audio_driver_config_t* config);

/**
 * @brief 获取默认设备配置
 * @param config 输出配置
 * @return 操作结果
 */
linx_audio_result_t linx_audio_device_get_default_config(linx_audio_device_config_t* config);

/**
 * @brief 检查格式是否支持
 * @param capabilities 设备能力
 * @param format 音频格式
 * @return true表示支持，false表示不支持
 */
bool linx_audio_device_supports_format(const linx_audio_device_capabilities_t* capabilities,
                                 const linx_audio_format_info_t* format);

#ifdef __cplusplus
}
#endif

#endif // LINX_AUDIO_DRIVER_H