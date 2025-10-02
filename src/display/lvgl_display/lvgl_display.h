/**
 * @file lvgl_display.h
 * @brief LVGL显示模块头文件
 * @details 提供基于LVGL的显示接口实现，支持状态显示、通知、表情、聊天消息等功能
 * @author LinX OS SDK Team
 * @version 1.0
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "../display.h"
#include "lvgl_image.h"

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct LvglDisplay LvglDisplay;
typedef struct LvglDisplayVTable LvglDisplayVTable;

/**
 * @brief 定时器回调函数类型
 * @details 用于通知定时器的回调函数类型定义
 * @param arg 传递给回调函数的参数
 */
typedef void (*timer_callback_t)(void* arg);

/**
 * @brief 简单定时器结构体
 * @details 用于替代esp_timer的简单定时器实现，支持一次性和周期性定时器
 */
typedef struct {
    timer_callback_t callback;      /**< 定时器回调函数 */
    void* arg;                      /**< 传递给回调函数的参数 */
    uint64_t timeout_us;            /**< 超时时间（微秒） */
    uint64_t start_time_us;         /**< 开始时间（微秒） */
    bool active;                    /**< 定时器是否激活 */
    bool repeat;                    /**< 是否为周期性定时器 */
} simple_timer_t;

/**
 * @brief 定时器管理结构体
 * @details 管理多个定时器的运行，使用独立线程处理定时器事件
 */
#define MAX_TIMERS 16               /**< 最大定时器数量 */
typedef struct {
    simple_timer_t* timers[MAX_TIMERS]; /**< 定时器数组 */
    int count;                      /**< 当前定时器数量 */
    pthread_t thread;               /**< 定时器处理线程 */
    bool running;                   /**< 定时器管理器是否运行中 */
    pthread_mutex_t mutex;          /**< 线程同步互斥锁 */
} TimerManager;

/**
 * @brief 外部状态提供者的回调函数类型定义
 */

/**
 * @brief 电池电量回调函数类型
 * @param level 输出参数，电池电量百分比(0-100)
 * @param charging 输出参数，是否正在充电
 * @param discharging 输出参数，是否正在放电
 * @param user_data 用户自定义数据
 * @return 成功获取电池信息返回true，否则返回false
 */
typedef bool (*battery_level_callback_t)(int* level, bool* charging, bool* discharging, void* user_data);

/**
 * @brief 网络图标回调函数类型
 * @param user_data 用户自定义数据
 * @return 返回网络状态对应的图标字符串
 */
typedef const char* (*network_icon_callback_t)(void* user_data);

/**
 * @brief 音量等级回调函数类型
 * @param user_data 用户自定义数据
 * @return 返回当前音量等级(0-100)
 */
typedef int (*volume_level_callback_t)(void* user_data);

/**
 * @brief 设备空闲状态回调函数类型
 * @param user_data 用户自定义数据
 * @return 设备空闲返回true，否则返回false
 */
typedef bool (*device_idle_callback_t)(void* user_data);

/**
 * @brief 低电量声音提示回调函数类型
 * @param user_data 用户自定义数据
 */
typedef void (*low_battery_sound_callback_t)(void* user_data);

/**
 * @brief 状态提供者回调函数结构体
 * @details 包含所有外部状态获取的回调函数，用于获取电池、网络、音量等状态信息
 */
typedef struct {
    battery_level_callback_t get_battery_level;        /**< 获取电池电量的回调函数 */
    network_icon_callback_t get_network_icon;          /**< 获取网络图标的回调函数 */
    volume_level_callback_t get_volume_level;          /**< 获取音量等级的回调函数 */
    device_idle_callback_t is_device_idle;             /**< 检查设备是否空闲的回调函数 */
    low_battery_sound_callback_t play_low_battery_sound; /**< 播放低电量提示音的回调函数 */
    void* user_data;                                   /**< 用户自定义数据，传递给所有回调函数 */
} LvglDisplayCallbacks;

/**
 * @brief LVGL显示器结构体
 * @details 主要的显示器实现，继承自DisplayInterface，提供完整的显示功能
 */
struct LvglDisplay {
    // 基础显示接口（继承自DisplayInterface）
    DisplayInterface base;              /**< 基础显示接口，提供多态性支持 */
    
    // LVGL对象
    lv_display_t* display;              /**< LVGL显示器对象 */
    lv_obj_t* network_label;            /**< 网络状态标签 */
    lv_obj_t* status_label;             /**< 状态文本标签 */
    lv_obj_t* notification_label;       /**< 通知消息标签 */
    lv_obj_t* mute_label;               /**< 静音状态标签 */
    lv_obj_t* battery_label;            /**< 电池状态标签 */
    lv_obj_t* low_battery_popup;        /**< 低电量弹窗 */
    lv_obj_t* low_battery_label;        /**< 低电量提示标签 */
    lv_obj_t* emotion_label;            /**< 表情显示标签 */
    lv_obj_t* chat_message_label;       /**< 聊天消息标签 */
    
    // 状态跟踪
    const char* battery_icon;           /**< 当前电池图标字符串 */
    const char* network_icon;           /**< 当前网络图标字符串 */
    bool muted;                         /**< 是否处于静音状态 */
    
    // 时间管理
    time_t last_status_update_time;     /**< 上次状态更新时间 */
    simple_timer_t* notification_timer; /**< 通知显示定时器 */
    TimerManager timer_manager;         /**< 定时器管理器 */
    
    // 状态更新计数器（用于网络图标更新）
    int status_update_counter;          /**< 状态更新计数器，控制网络图标更新频率 */
    
    // 外部状态提供者回调函数
    LvglDisplayCallbacks callbacks;     /**< 外部状态获取回调函数集合 */
    
    // 虚函数表
    const LvglDisplayVTable* vtable;    /**< 虚函数表，实现多态性 */
};

/**
 * @brief 虚函数表结构体
 * @details 用于实现多态性的虚函数表，包含所有可重写的函数指针
 */
struct LvglDisplayVTable {
    // 基础显示接口函数
    void (*set_status)(LvglDisplay* self, const char* status);                      /**< 设置状态文本 */
    void (*show_notification)(LvglDisplay* self, const char* notification, int duration_ms); /**< 显示通知消息 */
    void (*set_emotion)(LvglDisplay* self, const char* emotion);                    /**< 设置表情显示 */
    void (*set_chat_message)(LvglDisplay* self, const char* role, const char* content); /**< 设置聊天消息 */
    void (*set_preview_image)(LvglDisplay* self, LvglImage* image);                 /**< 设置预览图像 */
    void (*update_status_bar)(LvglDisplay* self, bool update_all);                  /**< 更新状态栏 */
    void (*set_power_save_mode)(LvglDisplay* self, bool on);                        /**< 设置省电模式 */
    bool (*snapshot_to_jpeg)(LvglDisplay* self, char* jpeg_data, size_t* data_size, int quality); /**< 截图并转换为JPEG */
    
    // 锁定/解锁函数（由派生类实现）
    bool (*lock)(LvglDisplay* self, int timeout_ms);                               /**< 锁定显示器 */
    void (*unlock)(LvglDisplay* self);                                             /**< 解锁显示器 */
    
    // 析构函数
    void (*destroy)(LvglDisplay* self);                                            /**< 销毁显示器对象 */
};

/**
 * @brief 函数声明
 * @details LVGL显示模块的公共API函数声明
 */

/**
 * @brief 创建新的LVGL显示器实例
 * @return 成功返回LvglDisplay指针，失败返回NULL
 */
LvglDisplay* lvgl_display_create(void);

/**
 * @brief 销毁LVGL显示器并释放资源
 * @param display 要销毁的显示器实例
 */
void lvgl_display_destroy(LvglDisplay* display);

/**
 * @brief 初始化LVGL显示器
 * @param display 要初始化的显示器实例
 * @return 初始化成功返回true，失败返回false
 */
bool lvgl_display_init(LvglDisplay* display);

/**
 * @brief 设置状态提供者回调函数
 * @param display 显示器实例
 * @param callbacks 回调函数结构体指针
 */
void lvgl_display_set_callbacks(LvglDisplay* display, const LvglDisplayCallbacks* callbacks);

/**
 * @brief 设置状态文本
 * @param display 显示器实例
 * @param status 要显示的状态文本
 */
void lvgl_display_set_status(LvglDisplay* display, const char* status);

/**
 * @brief 显示带超时的通知消息
 * @param display 显示器实例
 * @param notification 通知消息内容
 * @param duration_ms 显示持续时间（毫秒）
 */
void lvgl_display_show_notification(LvglDisplay* display, const char* notification, int duration_ms);

/**
 * @brief 设置表情显示
 * @param display 显示器实例
 * @param emotion 表情标识符
 */
void lvgl_display_set_emotion(LvglDisplay* display, const char* emotion);

/**
 * @brief 设置聊天消息显示
 * @param display 显示器实例
 * @param role 消息角色（用户、助手等）
 * @param content 消息内容
 */
void lvgl_display_set_chat_message(LvglDisplay* display, const char* role, const char* content);

/**
 * @brief 设置预览图像
 * @param display 显示器实例
 * @param image 要显示的图像对象
 */
void lvgl_display_set_preview_image(LvglDisplay* display, LvglImage* image);

/**
 * @brief 更新状态栏元素
 * @param display 显示器实例
 * @param update_all 是否更新所有元素（true）或仅更新必要元素（false）
 */
void lvgl_display_update_status_bar(LvglDisplay* display, bool update_all);

/**
 * @brief 设置省电模式
 * @param display 显示器实例
 * @param on 是否开启省电模式
 */
void lvgl_display_set_power_save_mode(LvglDisplay* display, bool on);

/**
 * @brief 截取屏幕并转换为JPEG格式
 * @param display 显示器实例
 * @param jpeg_data 输出JPEG数据缓冲区
 * @param data_size 输入/输出参数，缓冲区大小/实际数据大小
 * @param quality JPEG压缩质量（1-100）
 * @return 成功返回true，失败返回false
 */
bool lvgl_display_snapshot_to_jpeg(LvglDisplay* display, char* jpeg_data, size_t* data_size, int quality);

/**
 * @brief 锁定显示器以进行线程安全操作
 * @param display 显示器实例
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功锁定返回true，超时或失败返回false
 */
bool lvgl_display_lock(LvglDisplay* display, int timeout_ms);

/**
 * @brief 解锁显示器
 * @param display 显示器实例
 */
void lvgl_display_unlock(LvglDisplay* display);

/**
 * @brief 简单定时器函数（替代esp_timer）
 */

/**
 * @brief 创建简单定时器
 * @param callback 定时器回调函数
 * @param arg 传递给回调函数的参数
 * @return 成功返回定时器指针，失败返回NULL
 */
simple_timer_t* simple_timer_create(timer_callback_t callback, void* arg);

/**
 * @brief 销毁简单定时器
 * @param timer 要销毁的定时器
 */
void simple_timer_destroy(simple_timer_t* timer);

/**
 * @brief 启动一次性定时器
 * @param timer 定时器实例
 * @param timeout_us 超时时间（微秒）
 * @return 成功启动返回true，失败返回false
 */
bool simple_timer_start_once(simple_timer_t* timer, uint64_t timeout_us);

/**
 * @brief 启动周期性定时器
 * @param timer 定时器实例
 * @param period_us 周期时间（微秒）
 * @return 成功启动返回true，失败返回false
 */
bool simple_timer_start_periodic(simple_timer_t* timer, uint64_t period_us);

/**
 * @brief 停止定时器
 * @param timer 要停止的定时器
 */
void simple_timer_stop(simple_timer_t* timer);

/**
 * @brief 显示器锁保护结构体
 * @details 在C语言中实现类似RAII的行为，自动管理锁的获取和释放
 */
typedef struct {
    LvglDisplay* display;               /**< 关联的显示器实例 */
    bool locked;                        /**< 是否已锁定 */
} DisplayLockGuard;

/**
 * @brief 创建显示器锁保护对象
 * @param display 要锁定的显示器实例
 * @return 锁保护对象
 */
DisplayLockGuard display_lock_guard_create(LvglDisplay* display);

/**
 * @brief 销毁显示器锁保护对象并自动解锁
 * @param guard 要销毁的锁保护对象
 */
void display_lock_guard_destroy(DisplayLockGuard* guard);

#ifdef __cplusplus
}
#endif

#endif // LVGL_DISPLAY_H