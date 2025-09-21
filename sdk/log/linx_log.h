#ifndef LINX_LOG_H
#define LINX_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别定义 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_MAX
} log_level_t;

/* 日志配置结构体 */
typedef struct {
    log_level_t level;              /* 最低日志级别 */
    bool enable_timestamp;          /* 是否启用时间戳 */
    bool enable_thread_id;          /* 是否启用线程ID */
    bool enable_color;              /* 是否启用颜色输出 */
} log_config_t;

/* 日志上下文结构体 */
typedef struct {
    log_config_t config;
    bool initialized;
} log_context_t;

/* 默认配置 */
#define LOG_DEFAULT_CONFIG { \
    .level = LOG_LEVEL_INFO, \
    .enable_timestamp = true, \
    .enable_thread_id = false, \
    .enable_color = true \
}

/* 日志级别字符串 */
extern const char* log_level_strings[LOG_LEVEL_MAX];

/* 日志级别颜色代码 */
extern const char* log_level_colors[LOG_LEVEL_MAX];

/* 核心函数 */

/**
 * 初始化日志模块
 * @param config 日志配置，如果为NULL则使用默认配置
 * @return 0成功，-1失败
 */
int log_init(const log_config_t *config);

/**
 * 清理日志模块
 */
void log_cleanup(void);

/**
 * 设置日志级别
 * @param level 日志级别
 */
void log_set_level(log_level_t level);

/**
 * 获取当前日志级别
 * @return 当前日志级别
 */
log_level_t log_get_level(void);

/**
 * 写入日志
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_write(log_level_t level, const char *file, int line, 
               const char *func, const char *format, ...);

/**
 * 刷新日志缓冲区
 */
void log_flush(void);

/**
 * 检查日志级别是否启用
 * @param level 日志级别
 * @return true启用，false禁用
 */
bool log_is_level_enabled(log_level_t level);

/* 便捷宏定义 */
#define LOG_DEBUG(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_DEBUG)) { \
            log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_INFO)) { \
            log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_WARN)) { \
            log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_ERROR)) { \
            log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define LOG_FATAL(fmt, ...) \
    do { \
        if (log_is_level_enabled(LOG_LEVEL_FATAL)) { \
            log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* LINX_LOG_H */