#ifndef LINX_AUDIO_TYPES_H
#define LINX_AUDIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file types.h
 * @brief LinxOS音频系统核心类型定义
 * @details 定义音频系统中使用的所有基础数据结构、枚举和常量
 */

// ============================================================================
// 基础常量定义
// ============================================================================

/** 最大音频通道数 */
#define LINX_AUDIO_MAX_CHANNELS         8

/** 最大采样率 */
#define LINX_AUDIO_MAX_SAMPLE_RATE      192000

/** 最小采样率 */
#define LINX_AUDIO_MIN_SAMPLE_RATE      8000

/** 默认缓冲区大小 */
#define LINX_AUDIO_DEFAULT_BUFFER_SIZE  1024

/** 最大缓冲区大小 */
#define LINX_AUDIO_MAX_BUFFER_SIZE      8192

/** 最小缓冲区大小 */
#define LINX_AUDIO_MIN_BUFFER_SIZE      64

/** 最大设备名称长度 */
#define LINX_AUDIO_MAX_DEVICE_NAME      256

/** 最大流名称长度 */
#define LINX_AUDIO_MAX_STREAM_NAME      128

/** 最大插件名称长度 */
#define LINX_AUDIO_MAX_PLUGIN_NAME      64

/** 最大事件订阅者数量 */
#define LINX_AUDIO_MAX_SUBSCRIBERS      32

// ============================================================================
// 错误码定义
// ============================================================================

/**
 * @brief 音频系统错误码
 */
typedef enum {
    LINX_AUDIO_SUCCESS = 0,                 /**< 成功 */
    LINX_AUDIO_ERROR_INVALID_PARAM = -1,    /**< 无效参数 */
    LINX_AUDIO_ERROR_INVALID_PARAMETER = -1, /**< 无效参数 (别名) */
    LINX_AUDIO_ERROR_OUT_OF_MEMORY = -2,    /**< 内存不足 */
    LINX_AUDIO_ERROR_NOT_INITIALIZED = -3,  /**< 未初始化 */
    LINX_AUDIO_ERROR_ALREADY_INITIALIZED = -4, /**< 已初始化 */
    LINX_AUDIO_ERROR_DEVICE_NOT_FOUND = -5, /**< 设备未找到 */
    LINX_AUDIO_ERROR_DEVICE_BUSY = -6,      /**< 设备忙 */
    LINX_AUDIO_ERROR_UNSUPPORTED_FORMAT = -7, /**< 不支持的格式 */
    LINX_AUDIO_ERROR_BUFFER_OVERFLOW = -8,  /**< 缓冲区溢出 */
    LINX_AUDIO_ERROR_BUFFER_UNDERFLOW = -9, /**< 缓冲区下溢 */
    LINX_AUDIO_ERROR_TIMEOUT = -10,         /**< 超时 */
    LINX_AUDIO_ERROR_IO_ERROR = -11,        /**< IO错误 */
    LINX_AUDIO_ERROR_CODEC_ERROR = -12,     /**< 编解码错误 */
    LINX_AUDIO_ERROR_PLUGIN_ERROR = -13,    /**< 插件错误 */
    LINX_AUDIO_ERROR_STREAM_ERROR = -14,    /**< 流错误 */
    LINX_AUDIO_ERROR_NOT_FOUND = -15,       /**< 未找到 */
    LINX_AUDIO_ERROR_INVALID_STATE = -16,   /**< 无效状态 */
    LINX_AUDIO_ERROR_MUTEX_INIT = -17,      /**< 互斥锁初始化失败 */
    LINX_AUDIO_ERROR_CONDITION_INIT = -18,  /**< 条件变量初始化失败 */
    LINX_AUDIO_ERROR_THREAD_CREATE = -19,   /**< 线程创建失败 */
    LINX_AUDIO_ERROR_RESOURCE_LIMIT = -20,  /**< 资源限制 */
    LINX_AUDIO_ERROR_NOT_SUPPORTED = -21,   /**< 不支持的操作 */
    LINX_AUDIO_ERROR_THREAD_ERROR = -22,    /**< 线程错误 */
    LINX_AUDIO_ERROR_UNKNOWN = -99          /**< 未知错误 */
} linx_audio_error_t;

/**
 * @brief 音频操作结果类型
 * @details 与linx_audio_error_t相同，用于表示函数返回结果
 */
typedef linx_audio_error_t linx_audio_result_t;

// ============================================================================
// 音频格式定义
// ============================================================================

/**
 * @brief 音频采样格式
 */
typedef enum {
    LINX_AUDIO_FORMAT_UNKNOWN = 0,      /**< 未知格式 */
    LINX_AUDIO_FORMAT_U8,               /**< 8位无符号整数 */
    LINX_AUDIO_FORMAT_S16LE,            /**< 16位有符号整数，小端 */
    LINX_AUDIO_FORMAT_S16BE,            /**< 16位有符号整数，大端 */
    LINX_AUDIO_FORMAT_S24LE,            /**< 24位有符号整数，小端 */
    LINX_AUDIO_FORMAT_S24BE,            /**< 24位有符号整数，大端 */
    LINX_AUDIO_FORMAT_S32LE,            /**< 32位有符号整数，小端 */
    LINX_AUDIO_FORMAT_S32BE,            /**< 32位有符号整数，大端 */
    LINX_AUDIO_FORMAT_F32LE,            /**< 32位浮点数，小端 */
    LINX_AUDIO_FORMAT_F32BE,            /**< 32位浮点数，大端 */
    LINX_AUDIO_FORMAT_F64LE,            /**< 64位浮点数，小端 */
    LINX_AUDIO_FORMAT_F64BE             /**< 64位浮点数，大端 */
} linx_audio_format_t;

// 常用格式别名
#define LINX_AUDIO_FORMAT_INT16     LINX_AUDIO_FORMAT_S16LE
#define LINX_AUDIO_FORMAT_INT24     LINX_AUDIO_FORMAT_S24LE
#define LINX_AUDIO_FORMAT_INT32     LINX_AUDIO_FORMAT_S32LE
#define LINX_AUDIO_FORMAT_FLOAT32   LINX_AUDIO_FORMAT_F32LE
#define LINX_AUDIO_FORMAT_FLOAT64   LINX_AUDIO_FORMAT_F64LE

/**
 * @brief 音频参数结构体
 */
typedef struct {
    linx_audio_format_t format;         /**< 采样格式 */
    uint32_t sample_rate;               /**< 采样率 */
    uint16_t channels;                  /**< 通道数 */
    uint16_t bits_per_sample;           /**< 每样本位数 */
    uint32_t frame_size;                /**< 帧大小（字节） */
    uint32_t buffer_size;               /**< 缓冲区大小（帧数） */
} linx_audio_params_t;

// ============================================================================
// 设备相关定义
// ============================================================================

/**
 * @brief 音频设备类型
 */
typedef enum {
    LINX_AUDIO_DEVICE_TYPE_UNKNOWN = 0, /**< 未知设备 */
    LINX_AUDIO_DEVICE_TYPE_PLAYBACK,    /**< 播放设备 */
    LINX_AUDIO_DEVICE_TYPE_CAPTURE,     /**< 录音设备 */
    LINX_AUDIO_DEVICE_TYPE_DUPLEX       /**< 全双工设备 */
} linx_audio_device_type_t;

// 设备类型别名，用于兼容性
#define LINX_AUDIO_DEVICE_TYPE_OUTPUT   LINX_AUDIO_DEVICE_TYPE_PLAYBACK
#define LINX_AUDIO_DEVICE_TYPE_INPUT    LINX_AUDIO_DEVICE_TYPE_CAPTURE

/**
 * @brief 音频设备状态
 */
typedef enum {
    LINX_AUDIO_DEVICE_STATE_UNKNOWN = 0, /**< 未知状态 */
    LINX_AUDIO_DEVICE_STATE_IDLE,        /**< 空闲 */
    LINX_AUDIO_DEVICE_STATE_RUNNING,     /**< 运行中 */
    LINX_AUDIO_DEVICE_STATE_PAUSED,      /**< 暂停 */
    LINX_AUDIO_DEVICE_STATE_ERROR        /**< 错误状态 */
} linx_audio_device_state_t;

/**
 * @brief 音频设备信息
 */
typedef struct {
    uint32_t device_id;                 /**< 设备ID */
    char name[LINX_AUDIO_MAX_DEVICE_NAME]; /**< 设备名称 */
    linx_audio_device_type_t type;      /**< 设备类型 */
    linx_audio_device_state_t state;    /**< 设备状态 */
    linx_audio_params_t default_params; /**< 默认音频参数 */
    linx_audio_format_t format;         /**< 支持的音频格式 */
    uint32_t min_sample_rate;           /**< 最小采样率 */
    uint32_t max_sample_rate;           /**< 最大采样率 */
    bool is_default;                    /**< 是否为默认设备 */
    void* driver_data;                  /**< 驱动私有数据 */
} linx_audio_device_info_t;

// ============================================================================
// 流相关定义
// ============================================================================

/**
 * @brief 音频流类型
 */
typedef enum {
    LINX_AUDIO_STREAM_TYPE_UNKNOWN = 0, /**< 未知流 */
    LINX_AUDIO_STREAM_TYPE_PLAYBACK,    /**< 播放流 */
    LINX_AUDIO_STREAM_TYPE_CAPTURE,     /**< 录音流 */
    LINX_AUDIO_STREAM_TYPE_DUPLEX,      /**< 全双工流 */
    LINX_AUDIO_STREAM_TYPE_LOOPBACK,    /**< 回环流 */
    LINX_AUDIO_STREAM_TYPE_VIRTUAL      /**< 虚拟流 */
} linx_audio_stream_type_t;

/**
 * @brief 音频流状态
 */
typedef enum {
    LINX_AUDIO_STREAM_STATE_UNKNOWN = 0, /**< 未知状态 */
    LINX_AUDIO_STREAM_STATE_CREATED,     /**< 已创建 */
    LINX_AUDIO_STREAM_STATE_INITIALIZED, /**< 已初始化 */
    LINX_AUDIO_STREAM_STATE_PREPARED,    /**< 已准备 */
    LINX_AUDIO_STREAM_STATE_RUNNING,     /**< 运行中 */
    LINX_AUDIO_STREAM_STATE_PAUSED,      /**< 暂停 */
    LINX_AUDIO_STREAM_STATE_STOPPED,     /**< 停止 */
    LINX_AUDIO_STREAM_STATE_ERROR        /**< 错误状态 */
} linx_audio_stream_state_t;

/**
 * @brief 音频流优先级
 */
typedef enum {
    LINX_AUDIO_STREAM_PRIORITY_LOW = 0,     /**< 低优先级 */
    LINX_AUDIO_STREAM_PRIORITY_NORMAL,      /**< 普通优先级 */
    LINX_AUDIO_STREAM_PRIORITY_HIGH,        /**< 高优先级 */
    LINX_AUDIO_STREAM_PRIORITY_REALTIME     /**< 实时优先级 */
} linx_audio_stream_priority_t;

// ============================================================================
// 事件系统定义
// ============================================================================

/**
 * @brief 音频事件类型
 */
typedef enum {
    LINX_AUDIO_EVENT_UNKNOWN = 0,       /**< 未知事件 */
    LINX_AUDIO_EVENT_SYSTEM_INIT,       /**< 系统初始化 */
    LINX_AUDIO_EVENT_SYSTEM_SHUTDOWN,   /**< 系统关闭 */
    LINX_AUDIO_EVENT_DEVICE_ADDED,      /**< 设备添加 */
    LINX_AUDIO_EVENT_DEVICE_REMOVED,    /**< 设备移除 */
    LINX_AUDIO_EVENT_DEVICE_STATE_CHANGED, /**< 设备状态变化 */
    LINX_AUDIO_EVENT_STREAM_CREATED,    /**< 流创建 */
    LINX_AUDIO_EVENT_STREAM_DESTROYED,  /**< 流销毁 */
    LINX_AUDIO_EVENT_STREAM_STATE_CHANGED, /**< 流状态变化 */
    LINX_AUDIO_EVENT_BUFFER_OVERFLOW,   /**< 缓冲区溢出 */
    LINX_AUDIO_EVENT_BUFFER_UNDERFLOW,  /**< 缓冲区下溢 */
    LINX_AUDIO_EVENT_ERROR,             /**< 错误事件 */
    LINX_AUDIO_EVENT_TYPE_ALL = 0xFFFFFFFF  /**< 订阅所有事件 */
} linx_audio_event_type_t;

/**
 * @brief 音频事件数据
 */
typedef struct {
    linx_audio_event_type_t type;       /**< 事件类型 */
    uint64_t timestamp;                 /**< 时间戳 */
    uint32_t source_id;                 /**< 事件源ID */
    void* data;                         /**< 事件数据 */
    size_t data_size;                   /**< 数据大小 */
} linx_audio_event_t;

/**
 * @brief 事件回调函数类型
 */
typedef void (*linx_audio_event_callback_t)(const linx_audio_event_t* event, void* user_data);

// ============================================================================
// 插件系统定义
// ============================================================================

/**
 * @brief 插件类型
 */
typedef enum {
    LINX_AUDIO_PLUGIN_TYPE_UNKNOWN = 0, /**< 未知插件 */
    LINX_AUDIO_PLUGIN_TYPE_CODEC,       /**< 编解码器 */
    LINX_AUDIO_PLUGIN_TYPE_EFFECT,      /**< 音效处理 */
    LINX_AUDIO_PLUGIN_TYPE_FILTER,      /**< 滤波器 */
    LINX_AUDIO_PLUGIN_TYPE_DRIVER       /**< 驱动程序 */
} linx_audio_plugin_type_t;

/**
 * @brief 插件状态
 */
typedef enum {
    LINX_AUDIO_PLUGIN_STATE_UNKNOWN = 0, /**< 未知状态 */
    LINX_AUDIO_PLUGIN_STATE_LOADED,      /**< 已加载 */
    LINX_AUDIO_PLUGIN_STATE_INITIALIZED, /**< 已初始化 */
    LINX_AUDIO_PLUGIN_STATE_ACTIVE,      /**< 活跃状态 */
    LINX_AUDIO_PLUGIN_STATE_ERROR        /**< 错误状态 */
} linx_audio_plugin_state_t;

/**
 * @brief 插件信息
 */
typedef struct {
    char name[LINX_AUDIO_MAX_PLUGIN_NAME]; /**< 插件名称 */
    char version[32];                     /**< 版本号 */
    char author[64];                      /**< 作者 */
    char description[256];                /**< 描述 */
    linx_audio_plugin_type_t type;        /**< 插件类型 */
    linx_audio_plugin_state_t state;      /**< 插件状态 */
    void* handle;                         /**< 插件句柄 */
    void* interface;                      /**< 插件接口 */
} linx_audio_plugin_info_t;

// ============================================================================
// 缓冲区定义
// ============================================================================

/**
 * @brief 音频缓冲区
 */
typedef struct {
    void* data;                         /**< 数据指针 */
    size_t size;                        /**< 缓冲区大小（字节） */
    size_t used;                        /**< 已使用大小（字节） */
    uint32_t frames;                    /**< 帧数 */
    linx_audio_params_t params;         /**< 音频参数 */
    uint64_t timestamp;                 /**< 时间戳 */
    bool is_readonly;                   /**< 是否只读 */
} linx_audio_buffer_t;

// ============================================================================
// 线程优先级定义
// ============================================================================

/**
 * @brief 音频线程优先级
 */
typedef enum {
    LINX_AUDIO_THREAD_PRIORITY_LOW = 0,      /**< 低优先级 */
    LINX_AUDIO_THREAD_PRIORITY_NORMAL,       /**< 正常优先级 */
    LINX_AUDIO_THREAD_PRIORITY_HIGH,         /**< 高优先级 */
    LINX_AUDIO_THREAD_PRIORITY_REALTIME      /**< 实时优先级 */
} linx_audio_thread_priority_t;

// ============================================================================
// 音频格式信息定义
// ============================================================================

/**
 * @brief 通道布局类型
 */
typedef enum {
    LINX_AUDIO_CHANNEL_LAYOUT_UNKNOWN = 0,   /**< 未知布局 */
    LINX_AUDIO_CHANNEL_LAYOUT_MONO,          /**< 单声道 */
    LINX_AUDIO_CHANNEL_LAYOUT_STEREO,        /**< 立体声 */
    LINX_AUDIO_CHANNEL_LAYOUT_2_1,           /**< 2.1声道 */
    LINX_AUDIO_CHANNEL_LAYOUT_5_1,           /**< 5.1声道 */
    LINX_AUDIO_CHANNEL_LAYOUT_7_1            /**< 7.1声道 */
} linx_audio_channel_layout_t;

/**
 * @brief 音频格式信息结构体
 */
typedef struct {
    linx_audio_format_t format;         /**< 采样格式 */
    uint32_t sample_rate;               /**< 采样率 */
    uint16_t channels;                  /**< 通道数 */
    uint16_t bits_per_sample;           /**< 每样本位数 */
    uint32_t frame_size;                /**< 帧大小（字节） */
    linx_audio_channel_layout_t channel_layout; /**< 通道布局 */
} linx_audio_format_info_t;

/**
 * @brief 音频流配置结构体
 */
typedef struct {
    char name[LINX_AUDIO_MAX_STREAM_NAME]; /**< 流名称 */
    linx_audio_stream_type_t type;        /**< 流类型 */
    linx_audio_format_info_t format;      /**< 音频格式信息 */
    uint32_t device_id;                   /**< 关联的设备ID */
    bool auto_start;                      /**< 是否自动启动 */
    uint32_t latency_ms;                  /**< 延迟要求（毫秒） */
    uint32_t buffer_size;                 /**< 缓冲区大小（帧数） */
    uint32_t buffer_count;                /**< 缓冲区数量 */
    linx_audio_stream_priority_t priority; /**< 流优先级 */
} linx_audio_stream_config_t;





// ============================================================================
// 音频管理器状态定义
// ============================================================================

/**
 * @brief 音频管理器状态
 */
typedef enum {
    LINX_AUDIO_MANAGER_STATE_UNINITIALIZED = 0,
    LINX_AUDIO_MANAGER_STATE_INITIALIZING,
    LINX_AUDIO_MANAGER_STATE_INITIALIZED,
    LINX_AUDIO_MANAGER_STATE_DEINITIALIZING,
    LINX_AUDIO_MANAGER_STATE_STARTING,
    LINX_AUDIO_MANAGER_STATE_RUNNING,
    LINX_AUDIO_MANAGER_STATE_STOPPING,
    LINX_AUDIO_MANAGER_STATE_STOPPED,
    LINX_AUDIO_MANAGER_STATE_ERROR
} linx_audio_manager_state_t;

/**
 * @brief 音频驱动状态
 */
typedef enum {
    LINX_AUDIO_DRIVER_STATE_UNINITIALIZED = 0,  /**< 未初始化 */
    LINX_AUDIO_DRIVER_STATE_INITIALIZED,        /**< 已初始化 */
    LINX_AUDIO_DRIVER_STATE_RUNNING,            /**< 运行中 */
    LINX_AUDIO_DRIVER_STATE_STOPPED,            /**< 已停止 */
    LINX_AUDIO_DRIVER_STATE_ERROR               /**< 错误状态 */
} linx_audio_driver_state_t;

// ============================================================================
// 前向声明
// ============================================================================

typedef struct linx_audio_manager linx_audio_manager_t;

typedef struct linx_audio_stream linx_audio_stream_t;
typedef struct linx_audio_device linx_audio_device_t;
typedef struct linx_audio_plugin linx_audio_plugin_t;
typedef struct linx_event_bus linx_event_bus_t;
typedef struct linx_stream_manager linx_stream_manager_t;
typedef struct linx_plugin_manager linx_plugin_manager_t;

// ============================================================================
// 回调函数类型定义
// ============================================================================

/**
 * @brief 音频数据回调函数类型
 * @param stream 音频流
 * @param buffer 音频缓冲区
 * @param user_data 用户数据
 * @return 处理的帧数，负数表示错误
 */
typedef int (*linx_audio_data_callback_t)(
    linx_audio_stream_t* stream,
    linx_audio_buffer_t* buffer,
    void* user_data
);

/**
 * @brief 音频状态回调函数类型
 * @param stream 音频流
 * @param old_state 旧状态
 * @param new_state 新状态
 * @param user_data 用户数据
 */
typedef void (*linx_audio_state_callback_t)(
    linx_audio_stream_t* stream,
    linx_audio_stream_state_t old_state,
    linx_audio_stream_state_t new_state,
    void* user_data
);

/**
 * @brief 音频错误回调函数类型
 * @param stream 音频流
 * @param error 错误码
 * @param message 错误消息
 * @param user_data 用户数据
 */
typedef void (*linx_audio_error_callback_t)(
    linx_audio_stream_t* stream,
    linx_audio_error_t error,
    const char* message,
    void* user_data
);

/**
 * @brief 音频输入回调函数类型
 */
typedef void (*linx_audio_input_callback_t)(
    void* buffer,
    uint32_t frames,
    void* user_data
);

/**
 * @brief 音频输出回调函数类型
 */
typedef void (*linx_audio_output_callback_t)(
    void* buffer,
    uint32_t frames,
    void* user_data
);



// ============================================================================
// 实用宏定义
// ============================================================================

/** 获取音频格式的字节数 */
#define LINX_AUDIO_FORMAT_BYTES(format) \
    ((format) == LINX_AUDIO_FORMAT_U8 ? 1 : \
     (format) == LINX_AUDIO_FORMAT_S16LE || (format) == LINX_AUDIO_FORMAT_S16BE ? 2 : \
     (format) == LINX_AUDIO_FORMAT_S24LE || (format) == LINX_AUDIO_FORMAT_S24BE ? 3 : \
     (format) == LINX_AUDIO_FORMAT_S32LE || (format) == LINX_AUDIO_FORMAT_S32BE || \
     (format) == LINX_AUDIO_FORMAT_F32LE || (format) == LINX_AUDIO_FORMAT_F32BE ? 4 : \
     (format) == LINX_AUDIO_FORMAT_F64LE || (format) == LINX_AUDIO_FORMAT_F64BE ? 8 : 0)

/** 计算帧大小 */
#define LINX_AUDIO_FRAME_SIZE(format, channels) \
    (LINX_AUDIO_FORMAT_BYTES(format) * (channels))

/** 计算缓冲区大小（字节） */
#define LINX_AUDIO_BUFFER_BYTES(format, channels, frames) \
    (LINX_AUDIO_FRAME_SIZE(format, channels) * (frames))

/** 检查音频参数是否有效 */
#define LINX_AUDIO_PARAMS_VALID(params) \
    ((params) && \
     (params)->format > LINX_AUDIO_FORMAT_UNKNOWN && \
     (params)->sample_rate >= LINX_AUDIO_MIN_SAMPLE_RATE && \
     (params)->sample_rate <= LINX_AUDIO_MAX_SAMPLE_RATE && \
     (params)->channels > 0 && \
     (params)->channels <= LINX_AUDIO_MAX_CHANNELS)

#ifdef __cplusplus
}
#endif

#endif /* LINX_AUDIO_TYPES_H */