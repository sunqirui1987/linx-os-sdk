#include "linx_log.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 全局日志上下文 */
static log_context_t g_log_ctx = {0};
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 日志级别字符串 */
const char* log_level_strings[LOG_LEVEL_MAX] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/* 日志级别颜色代码 (ANSI) */
const char* log_level_colors[LOG_LEVEL_MAX] = {
    "\033[36m",  /* DEBUG - 青色 */
    "\033[32m",  /* INFO  - 绿色 */
    "\033[33m",  /* WARN  - 黄色 */
    "\033[31m",  /* ERROR - 红色 */
    "\033[35m"   /* FATAL - 紫色 */
};

/* 颜色重置 */
#define COLOR_RESET "\033[0m"

/* 内部函数声明 */
static void log_format_timestamp(char *buffer, size_t size);

int log_init(const log_config_t *config)
{
    pthread_mutex_lock(&g_log_mutex);
    
    /* 如果已经初始化，先清理 */
    if (g_log_ctx.initialized) {
        log_cleanup();
    }
    
    /* 使用默认配置或用户配置 */
    if (config) {
        g_log_ctx.config = *config;
    } else {
        log_config_t default_config = LOG_DEFAULT_CONFIG;
        g_log_ctx.config = default_config;
    }
    
    g_log_ctx.initialized = true;
    pthread_mutex_unlock(&g_log_mutex);
    
    return 0;
}

void log_cleanup(void)
{
    pthread_mutex_lock(&g_log_mutex);
    
    g_log_ctx.initialized = false;
    
    pthread_mutex_unlock(&g_log_mutex);
}

void log_set_level(log_level_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level < LOG_LEVEL_MAX) {
        pthread_mutex_lock(&g_log_mutex);
        g_log_ctx.config.level = level;
        pthread_mutex_unlock(&g_log_mutex);
    }
}

log_level_t log_get_level(void)
{
    pthread_mutex_lock(&g_log_mutex);
    log_level_t level = g_log_ctx.config.level;
    pthread_mutex_unlock(&g_log_mutex);
    return level;
}

bool log_is_level_enabled(log_level_t level)
{
    return (g_log_ctx.initialized && level >= g_log_ctx.config.level);
}

void log_write(log_level_t level, const char *file, int line, 
               const char *func, const char *format, ...)
{
    if (!g_log_ctx.initialized || level < g_log_ctx.config.level) {
        return;
    }
    
    pthread_mutex_lock(&g_log_mutex);
    
    char timestamp[64] = {0};
    char message[1024] = {0};
    char log_line[1536] = {0};
    
    /* 格式化时间戳 */
    if (g_log_ctx.config.enable_timestamp) {
        log_format_timestamp(timestamp, sizeof(timestamp));
    }
    
    /* 格式化用户消息 */
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    /* 构建完整的日志行 */
    const char *basename = strrchr(file, '/');
    basename = basename ? basename + 1 : file;
    
    if (g_log_ctx.config.enable_timestamp) {
        snprintf(log_line, sizeof(log_line), "[%s] [%s] %s:%d %s() - %s\n",
                timestamp, log_level_strings[level], basename, line, func, message);
    } else {
        snprintf(log_line, sizeof(log_line), "[%s] %s:%d %s() - %s\n",
                log_level_strings[level], basename, line, func, message);
    }
    
    /* 输出到控制台 */
    if (g_log_ctx.config.enable_color) {
        fprintf(stderr, "%s%s%s", log_level_colors[level], log_line, COLOR_RESET);
    } else {
        fprintf(stderr, "%s", log_line);
    }
    fflush(stderr);
    
    pthread_mutex_unlock(&g_log_mutex);
}

void log_flush(void)
{
    pthread_mutex_lock(&g_log_mutex);
    
    fflush(stderr);
    fflush(stdout);
    
    pthread_mutex_unlock(&g_log_mutex);
}

/* 内部函数实现 */

static void log_format_timestamp(char *buffer, size_t size)
{
    time_t now;
    struct tm *tm_info;
    
    time(&now);
    tm_info = localtime(&now);
    
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}