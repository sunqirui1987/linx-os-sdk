/**
 * @file lvgl_display.c
 * @brief LVGL显示模块实现文件
 * @details 实现基于LVGL的显示接口，提供状态显示、通知、表情、聊天消息等功能
 * @author LinX OS SDK Team
 * @version 1.0
 */

#include "lvgl_display.h"
#include "../log/linx_log.h"
#include "../../third/fonts/include/font_awesome.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define TAG "DISPLAY"              /**< 日志标签 */

/**
 * @brief 获取当前时间（微秒）
 * @details 使用单调时钟获取高精度时间戳，用于定时器计算
 * @return 当前时间的微秒数
 */
static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/**
 * @brief 定时器线程函数
 * @details 在独立线程中运行，定期检查所有定时器并触发到期的回调函数
 * @param arg 指向TimerManager结构体的指针
 * @return 线程退出时返回NULL
 */
static void* timer_thread_func(void* arg) {
    TimerManager* tm = (TimerManager*)arg;
    
    while (tm->running) {
        uint64_t current_time = get_time_us();
        
        pthread_mutex_lock(&tm->mutex);
        for (int i = 0; i < tm->count; i++) {
            simple_timer_t* timer = tm->timers[i];
            if (timer && timer->active) {
                uint64_t elapsed = current_time - timer->start_time_us;
                if (elapsed >= timer->timeout_us) {
                    // 执行定时器回调函数
                    if (timer->callback) {
                        timer->callback(timer->arg);
                    }
                    
                    // 处理周期性定时器或一次性定时器
                    if (timer->repeat) {
                        timer->start_time_us = current_time;  // 重置周期性定时器
                    } else {
                        timer->active = false;                // 停用一次性定时器
                    }
                }
            }
        }
        pthread_mutex_unlock(&tm->mutex);
        usleep(10000); // 休眠10毫秒，避免过度占用CPU
    }
    return NULL;
}

/**
 * @brief 定时器管理函数
 */

/**
 * @brief 初始化定时器管理器
 * @details 清零结构体，初始化互斥锁，启动定时器处理线程
 * @param tm 定时器管理器指针
 */
static void timer_manager_init(TimerManager* tm) {
    memset(tm, 0, sizeof(TimerManager));
    pthread_mutex_init(&tm->mutex, NULL);
    tm->running = true;
    pthread_create(&tm->thread, NULL, timer_thread_func, tm);
}

/**
 * @brief 清理定时器管理器
 * @details 停止定时器线程，等待线程结束，销毁互斥锁
 * @param tm 定时器管理器指针
 */
static void timer_manager_cleanup(TimerManager* tm) {
    if (tm->running) {
        tm->running = false;
        pthread_join(tm->thread, NULL);
        pthread_mutex_destroy(&tm->mutex);
    }
}

/**
 * @brief 向定时器管理器添加定时器
 * @details 线程安全地将定时器添加到管理器中，并设置开始时间
 * @param tm 定时器管理器指针
 * @param timer 要添加的定时器指针
 */
static void timer_manager_add(TimerManager* tm, simple_timer_t* timer) {
    pthread_mutex_lock(&tm->mutex);
    if (tm->count < MAX_TIMERS) {
        tm->timers[tm->count++] = timer;
        timer->start_time_us = get_time_us();
    }
    pthread_mutex_unlock(&tm->mutex);
}

/**
 * @brief 从定时器管理器移除定时器
 * @details 线程安全地从管理器中移除指定的定时器
 * @param tm 定时器管理器指针
 * @param timer 要移除的定时器指针
 */
static void timer_manager_remove(TimerManager* tm, simple_timer_t* timer) {
    pthread_mutex_lock(&tm->mutex);
    for (int i = 0; i < tm->count; i++) {
        if (tm->timers[i] == timer) {
            // 移除定时器并压缩数组
            for (int j = i; j < tm->count - 1; j++) {
                tm->timers[j] = tm->timers[j + 1];
            }
            tm->count--;
            break;
        }
    }
    pthread_mutex_unlock(&tm->mutex);
}

/**
 * @brief 简单定时器实现
 */

/**
 * @brief 创建简单定时器
 * @details 分配内存并初始化定时器结构体，设置回调函数和参数
 * @param callback 定时器到期时调用的回调函数
 * @param arg 传递给回调函数的用户数据
 * @return 成功返回定时器指针，失败返回NULL
 */
simple_timer_t* simple_timer_create(timer_callback_t callback, void* arg) {
    if (!callback) return NULL;
    
    simple_timer_t* timer = malloc(sizeof(simple_timer_t));
    if (!timer) {
        LINX_LOGE(TAG, "Failed to allocate timer");
        return NULL;
    }
    
    // 初始化定时器结构体
    *timer = (simple_timer_t){
        .callback = callback,
        .arg = arg,
        .timeout_us = 0,
        .start_time_us = 0,
        .active = false,
        .repeat = false
    };
    
    return timer;
}

/**
 * @brief 销毁简单定时器
 * @details 停止定时器并释放内存
 * @param timer 要销毁的定时器指针
 */
void simple_timer_destroy(simple_timer_t* timer) {
    if (timer) {
        simple_timer_stop(timer);
        free(timer);
    }
}

/**
 * @brief 启动一次性定时器
 * @details 设置定时器为一次性模式，指定超时时间后激活
 * @param timer 定时器指针
 * @param timeout_us 超时时间（微秒）
 * @return 成功返回true，失败返回false
 */
bool simple_timer_start_once(simple_timer_t* timer, uint64_t timeout_us) {
    if (!timer) return false;
    
    simple_timer_stop(timer);
    timer->timeout_us = timeout_us;
    timer->active = true;
    timer->repeat = false;
    
    // 注意：这需要访问显示器的定时器管理器
    // 在实际实现中，需要将定时器管理器作为参数传递
    return true;
}

/**
 * @brief 启动周期性定时器
 * @details 设置定时器为周期性模式，按指定周期重复触发
 * @param timer 定时器指针
 * @param period_us 周期时间（微秒）
 * @return 成功返回true，失败返回false
 */
bool simple_timer_start_periodic(simple_timer_t* timer, uint64_t period_us) {
    if (!timer) return false;
    
    simple_timer_stop(timer);
    timer->timeout_us = period_us;
    timer->active = true;
    timer->repeat = true;
    
    return true;
}

/**
 * @brief 停止定时器
 * @details 将定时器设置为非激活状态，停止触发回调
 * @param timer 要停止的定时器指针
 */
void simple_timer_stop(simple_timer_t* timer) {
    if (timer) {
        timer->active = false;
    }
}

/**
 * @brief 显示器锁保护实现
 * @details 提供类似RAII的锁管理机制，自动获取和释放锁
 */

/**
 * @brief 创建显示器锁保护对象
 * @details 尝试获取显示器锁，创建保护对象用于自动管理锁的生命周期
 * @param display 要锁定的显示器实例
 * @return 锁保护对象，包含锁定状态信息
 */
DisplayLockGuard display_lock_guard_create(LvglDisplay* display) {
    DisplayLockGuard guard = {.display = display, .locked = false};
    
    if (display && display->vtable && display->vtable->lock) {
        guard.locked = display->vtable->lock(display, 0);
    }
    
    return guard;
}

/**
 * @brief 销毁显示器锁保护对象
 * @details 如果锁已获取，则自动释放锁并重置状态
 * @param guard 要销毁的锁保护对象指针
 */
void display_lock_guard_destroy(DisplayLockGuard* guard) {
    if (guard && guard->locked && guard->display && 
        guard->display->vtable && guard->display->vtable->unlock) {
        guard->display->vtable->unlock(guard->display);
        guard->locked = false;
    }
}

/**
 * @brief 前向声明
 * @details 虚函数表实现函数的前向声明，用于在定义虚函数表时引用
 */
static void lvgl_display_set_status_impl(LvglDisplay* self, const char* status);
static void lvgl_display_show_notification_impl(LvglDisplay* self, const char* notification, int duration_ms);
static void lvgl_display_set_emotion_impl(LvglDisplay* self, const char* emotion);
static void lvgl_display_set_chat_message_impl(LvglDisplay* self, const char* role, const char* content);
static void lvgl_display_set_preview_image_impl(LvglDisplay* self, LvglImage* image);
static void lvgl_display_update_status_bar_impl(LvglDisplay* self, bool update_all);
static void lvgl_display_set_power_save_mode_impl(LvglDisplay* self, bool on);
static bool lvgl_display_snapshot_to_jpeg_impl(LvglDisplay* self, char* jpeg_data, size_t* data_size, int quality);
static bool lvgl_display_lock_impl(LvglDisplay* self, int timeout_ms);
static void lvgl_display_unlock_impl(LvglDisplay* self);
static void lvgl_display_destroy_impl(LvglDisplay* self);

/**
 * @brief 虚函数表
 * @details 定义LvglDisplay类的虚函数表，实现多态机制
 */
static const LvglDisplayVTable lvgl_display_vtable = {
    .set_status = lvgl_display_set_status_impl,                    // 设置状态文本
    .show_notification = lvgl_display_show_notification_impl,      // 显示通知消息
    .set_emotion = lvgl_display_set_emotion_impl,                  // 设置表情
    .set_chat_message = lvgl_display_set_chat_message_impl,        // 设置聊天消息
    .set_preview_image = lvgl_display_set_preview_image_impl,      // 设置预览图片
    .update_status_bar = lvgl_display_update_status_bar_impl,      // 更新状态栏
    .set_power_save_mode = lvgl_display_set_power_save_mode_impl,  // 设置省电模式
    .snapshot_to_jpeg = lvgl_display_snapshot_to_jpeg_impl,        // 截图为JPEG
    .lock = lvgl_display_lock_impl,                                // 锁定显示器
    .unlock = lvgl_display_unlock_impl,                            // 解锁显示器
    .destroy = lvgl_display_destroy_impl                           // 销毁显示器
};

/**
 * @brief 通知定时器回调函数
 * @details 当通知显示时间到期时调用，隐藏通知标签并显示状态标签
 * @param arg 传入的LvglDisplay实例指针
 */
static void notification_timer_callback(void* arg) {
    LvglDisplay* display = (LvglDisplay*)arg;
    if (!display) return;
    
    // 使用锁保护确保线程安全
    DisplayLockGuard guard = display_lock_guard_create(display);
    
    // 隐藏通知标签
    if (display->notification_label) {
        lv_obj_add_flag(display->notification_label, LV_OBJ_FLAG_HIDDEN);
    }
    // 显示状态标签
    if (display->status_label) {
        lv_obj_remove_flag(display->status_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 创建LVGL显示器实例
 * @details 分配内存并初始化LvglDisplay结构体，设置虚函数表和各种组件
 * @return 成功返回显示器实例指针，失败返回NULL
 */
LvglDisplay* lvgl_display_create(void) {
    LvglDisplay* display = malloc(sizeof(LvglDisplay));
    if (!display) {
        LINX_LOGE(TAG, "Failed to allocate LvglDisplay");
        return NULL;
    }
    
    // 清零内存并设置虚函数表
    memset(display, 0, sizeof(LvglDisplay));
    display->vtable = &lvgl_display_vtable;
    
    // 初始化基础显示接口
    display_interface_init(&display->base);
    
    // 创建通知定时器
    display->notification_timer = simple_timer_create(notification_timer_callback, display);
    if (!display->notification_timer) {
        LINX_LOGE(TAG, "Failed to create notification timer");
        free(display);
        return NULL;
    }
    
    // 初始化定时器系统
    timer_manager_init(&display->timer_manager);
    
    // 初始化状态变量
    display->muted = false;
    display->last_status_update_time = time(NULL);
    display->status_update_counter = 0;
    
    // 初始化回调函数结构体
    memset(&display->callbacks, 0, sizeof(LvglDisplayCallbacks));
    
    return display;
}

/**
 * @brief 销毁LVGL显示器实例
 * @details 调用虚函数表中的销毁函数，清理资源并释放内存
 * @param display 要销毁的显示器实例指针
 */
void lvgl_display_destroy(LvglDisplay* display) {
    if (!display) return;
    
    if (display->vtable && display->vtable->destroy) {
        display->vtable->destroy(display);
    }
}

/**
 * @brief 初始化显示器
 * @details 检查显示器实例是否有效，执行必要的初始化操作
 * @param display 要初始化的显示器实例
 * @return 成功返回true，失败返回false
 */
bool lvgl_display_init(LvglDisplay* display) {
    return display != NULL;
}

/**
 * @brief 设置回调函数
 * @details 将用户提供的回调函数结构体复制到显示器实例中
 * @param display 显示器实例指针
 * @param callbacks 包含各种回调函数的结构体指针
 */
void lvgl_display_set_callbacks(LvglDisplay* display, const LvglDisplayCallbacks* callbacks) {
    if (display && callbacks) {
        display->callbacks = *callbacks;
    }
}

/**
 * @brief 公共API实现
 * @details 以下函数为显示器的公共接口，通过虚函数表调用具体实现
 */

/**
 * @brief 设置状态文本
 * @details 在状态栏显示指定的状态信息
 * @param display 显示器实例指针
 * @param status 要显示的状态文本
 */
void lvgl_display_set_status(LvglDisplay* display, const char* status) {
    if (display && display->vtable && display->vtable->set_status) {
        display->vtable->set_status(display, status);
    }
}

/**
 * @brief 显示通知消息
 * @details 在屏幕上显示临时通知消息，指定时间后自动消失
 * @param display 显示器实例指针
 * @param notification 通知消息文本
 * @param duration_ms 显示持续时间（毫秒）
 */
void lvgl_display_show_notification(LvglDisplay* display, const char* notification, int duration_ms) {
    if (display && display->vtable && display->vtable->show_notification) {
        display->vtable->show_notification(display, notification, duration_ms);
    }
}

/**
 * @brief 设置表情
 * @details 在屏幕上显示指定的表情图标
 * @param display 显示器实例指针
 * @param emotion 表情标识符
 */
void lvgl_display_set_emotion(LvglDisplay* display, const char* emotion) {
    if (display && display->vtable && display->vtable->set_emotion) {
        display->vtable->set_emotion(display, emotion);
    }
}

/**
 * @brief 设置聊天消息
 * @details 在聊天界面显示消息，区分角色和内容
 * @param display 显示器实例指针
 * @param role 消息发送者角色
 * @param content 消息内容
 */
void lvgl_display_set_chat_message(LvglDisplay* display, const char* role, const char* content) {
    if (display && display->vtable && display->vtable->set_chat_message) {
        display->vtable->set_chat_message(display, role, content);
    }
}

/**
 * @brief 设置预览图片
 * @details 在屏幕上显示预览图片
 * @param display 显示器实例指针
 * @param image 要显示的图片对象
 */
void lvgl_display_set_preview_image(LvglDisplay* display, LvglImage* image) {
    if (display && display->vtable && display->vtable->set_preview_image) {
        display->vtable->set_preview_image(display, image);
    }
}

/**
 * @brief 更新状态栏
 * @details 刷新状态栏显示的信息，如电池、网络、音量等
 * @param display 显示器实例指针
 * @param update_all 是否更新所有状态栏元素
 */
void lvgl_display_update_status_bar(LvglDisplay* display, bool update_all) {
    if (display && display->vtable && display->vtable->update_status_bar) {
        display->vtable->update_status_bar(display, update_all);
    }
}

/**
 * @brief 设置省电模式
 * @details 启用或禁用省电模式，调整屏幕亮度和刷新率
 * @param display 显示器实例指针
 * @param on true启用省电模式，false禁用
 */
void lvgl_display_set_power_save_mode(LvglDisplay* display, bool on) {
    if (display && display->vtable && display->vtable->set_power_save_mode) {
        display->vtable->set_power_save_mode(display, on);
    }
}

/**
 * @brief 截图为JPEG格式
 * @details 将当前屏幕内容保存为JPEG格式的图片数据
 * @param display 显示器实例指针
 * @param jpeg_data 用于存储JPEG数据的缓冲区
 * @param data_size 输入时为缓冲区大小，输出时为实际数据大小
 * @param quality JPEG压缩质量（1-100）
 * @return 成功返回true，失败返回false
 */
bool lvgl_display_snapshot_to_jpeg(LvglDisplay* display, char* jpeg_data, size_t* data_size, int quality) {
    if (display && display->vtable && display->vtable->snapshot_to_jpeg) {
        return display->vtable->snapshot_to_jpeg(display, jpeg_data, data_size, quality);
    }
    return false;
}

/**
 * @brief 锁定显示器
 * @details 获取显示器的互斥锁，确保线程安全访问
 * @param display 显示器实例指针
 * @param timeout_ms 超时时间（毫秒），0表示立即返回
 * @return 成功获取锁返回true，失败返回false
 */
bool lvgl_display_lock(LvglDisplay* display, int timeout_ms) {
    if (display && display->vtable && display->vtable->lock) {
        return display->vtable->lock(display, timeout_ms);
    }
    return false;
}

/**
 * @brief 解锁显示器
 * @details 释放显示器的互斥锁，允许其他线程访问
 * @param display 显示器实例指针
 */
void lvgl_display_unlock(LvglDisplay* display) {
    if (display && display->vtable && display->vtable->unlock) {
        display->vtable->unlock(display);
    }
}

/**
 * @brief 虚函数实现
 * @details 以下为虚函数表中各函数的具体实现
 */

/**
 * @brief 设置状态文本的实现
 * @details 更新状态标签的文本内容，记录更新时间和计数器
 * @param self 显示器实例指针
 * @param status 要显示的状态文本
 */
static void lvgl_display_set_status_impl(LvglDisplay* self, const char* status) {
    if (!self || !status) return;
    
    // 获取显示器锁保护
    DisplayLockGuard guard = display_lock_guard_create(self);
    
    // 更新状态标签文本并显示
    if (self->status_label) {
        lv_label_set_text(self->status_label, status);
        lv_obj_remove_flag(self->status_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏通知标签
    if (self->notification_label) {
        lv_obj_add_flag(self->notification_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 记录状态更新时间
    self->last_status_update_time = time(NULL);
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 显示通知消息的实现
 * @details 在屏幕上显示临时通知，启动定时器在指定时间后自动隐藏
 * @param self 显示器实例指针
 * @param notification 通知消息文本
 * @param duration_ms 显示持续时间（毫秒）
 */
static void lvgl_display_show_notification_impl(LvglDisplay* self, const char* notification, int duration_ms) {
    if (!self || !notification) return;
    
    // 获取显示器锁保护
    DisplayLockGuard guard = display_lock_guard_create(self);
    
    // 更新通知标签文本并显示
    if (self->notification_label) {
        lv_label_set_text(self->notification_label, notification);
        lv_obj_remove_flag(self->notification_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 隐藏状态标签
    if (self->status_label) {
        lv_obj_add_flag(self->status_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 启动通知定时器，到期后自动隐藏通知
    if (self->notification_timer) {
        simple_timer_stop(self->notification_timer);
        timer_manager_add(&self->timer_manager, self->notification_timer);
        simple_timer_start_once(self->notification_timer, duration_ms * 1000);
    }
    
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 设置表情的实现
 * @details 在表情标签中显示指定的表情，添加表情符号前缀
 * @param self 显示器实例指针
 * @param emotion 表情标识符字符串
 */
static void lvgl_display_set_emotion_impl(LvglDisplay* self, const char* emotion) {
    if (!self || !emotion) {
        LINX_LOGW(TAG, "Invalid parameters for set_emotion");
        return;
    }
    
    LINX_LOGI(TAG, "Setting emotion: %s", emotion);
    
    // 获取显示器锁保护
    DisplayLockGuard guard = display_lock_guard_create(self);
    
    // 更新表情标签，添加表情符号前缀
    if (self->emotion_label) {
        char emotion_text[64];
        snprintf(emotion_text, sizeof(emotion_text), "😊 %s", emotion);
        lv_label_set_text(self->emotion_label, emotion_text);
        lv_obj_remove_flag(self->emotion_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 设置聊天消息的实现
 * @details 在聊天消息标签中显示角色和内容，格式为"角色: 内容"
 * @param self 显示器实例指针
 * @param role 消息发送者角色
 * @param content 消息内容
 */
static void lvgl_display_set_chat_message_impl(LvglDisplay* self, const char* role, const char* content) {
    if (!self || !role || !content) {
        LINX_LOGW(TAG, "Invalid parameters for set_chat_message");
        return;
    }
    
    LINX_LOGI(TAG, "Setting chat message - Role: %s, Content: %s", role, content);
    
    // 获取显示器锁保护
    DisplayLockGuard guard = display_lock_guard_create(self);
    
    // 格式化并显示聊天消息
    if (self->chat_message_label) {
        char message_text[256];
        snprintf(message_text, sizeof(message_text), "%s: %s", role, content);
        lv_label_set_text(self->chat_message_label, message_text);
        lv_obj_remove_flag(self->chat_message_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 设置预览图片的实现
 * @details 占位符实现，用于设置预览图片显示
 * @param self 显示器实例指针
 * @param image 要显示的图片对象
 */
static void lvgl_display_set_preview_image_impl(LvglDisplay* self, LvglImage* image) {
    // 占位符实现
    (void)self;
    (void)image;
}

/**
 * @brief 更新电池图标显示
 * @details 根据电池电量和充电状态更新电池图标，并处理低电量弹窗
 * @param self 显示器实例指针
 */
// Helper function to update battery icon
static void update_battery_icon(LvglDisplay* self) {
    // 检查回调函数和电池标签是否有效
    if (!self->callbacks.get_battery_level || !self->battery_label) return;
    
    int battery_level;      // 电池电量百分比
    bool charging, discharging;  // 充电状态和放电状态
    
    // 获取电池状态信息
    if (!self->callbacks.get_battery_level(&battery_level, &charging, &discharging, self->callbacks.user_data)) {
        return;
    }
    
    // 根据电池状态选择合适的图标
    const char* icon = charging ? FONT_AWESOME_BATTERY_BOLT :  // 充电中显示闪电图标
        (battery_level < 20) ? FONT_AWESOME_BATTERY_EMPTY :    // 电量低于20%显示空电池
        (battery_level < 40) ? FONT_AWESOME_BATTERY_QUARTER :  // 电量20-40%显示1/4电池
        (battery_level < 60) ? FONT_AWESOME_BATTERY_HALF :     // 电量40-60%显示1/2电池
        (battery_level < 80) ? FONT_AWESOME_BATTERY_THREE_QUARTERS : // 电量60-80%显示3/4电池
        FONT_AWESOME_BATTERY_FULL;                             // 电量80%以上显示满电池
    
    // 只有图标发生变化时才更新显示
    if (self->battery_icon != icon) {
        self->battery_icon = icon;
        lv_label_set_text(self->battery_label, icon);
    }
    
    // 处理低电量弹窗显示
    if (self->low_battery_popup) {
        bool show_popup = (battery_level < 20 && discharging);  // 电量低于20%且正在放电时显示弹窗
        bool is_hidden = lv_obj_has_flag(self->low_battery_popup, LV_OBJ_FLAG_HIDDEN);
        
        // 需要显示弹窗且当前隐藏时，显示弹窗并播放提示音
        if (show_popup && is_hidden) {
            lv_obj_remove_flag(self->low_battery_popup, LV_OBJ_FLAG_HIDDEN);
            if (self->callbacks.play_low_battery_sound) {
                self->callbacks.play_low_battery_sound(self->callbacks.user_data);
            }
        } 
        // 不需要显示弹窗且当前显示时，隐藏弹窗
        else if (!show_popup && !is_hidden) {
            lv_obj_add_flag(self->low_battery_popup, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * @brief 更新状态栏的实现
 * @details 更新状态栏中的各种状态信息，包括静音图标、网络图标、电池图标等
 * @param self 显示器实例指针
 * @param update_all 是否强制更新所有状态信息
 */
static void lvgl_display_update_status_bar_impl(LvglDisplay* self, bool update_all) {
    if (!self) return;
    
    // 获取显示器锁，确保线程安全
    DisplayLockGuard guard = display_lock_guard_create(self);
    
    // 更新静音图标
    if (self->callbacks.get_volume_level && self->mute_label) {
        int volume = self->callbacks.get_volume_level(self->callbacks.user_data);
        bool should_mute = (volume == 0);  // 音量为0时显示静音图标
        
        // 只有静音状态发生变化时才更新显示
        if (should_mute != self->muted) {
            self->muted = should_mute;
            lv_label_set_text(self->mute_label, should_mute ? FONT_AWESOME_VOLUME_XMARK : "");
        }
    }

    // 当设备空闲时更新时间显示
    if (self->callbacks.is_device_idle && self->callbacks.is_device_idle(self->callbacks.user_data)) {
        time_t now = time(NULL);
        // 每10秒更新一次时间显示
        if (now - self->last_status_update_time > 10) {
            struct tm* tm_info = localtime(&now);
            // 确保时间有效（年份大于等于2025）
            if (tm_info->tm_year >= 2025 - 1900) {
                char time_str[16];
                strftime(time_str, sizeof(time_str), "%H:%M  ", tm_info);  // 格式化为"时:分  "
                lvgl_display_set_status_impl(self, time_str);
            }
        }
    }
    
    // 更新电池图标
    update_battery_icon(self);
    
    // 每10次更新或强制更新时更新网络图标
    if (update_all || (self->status_update_counter++ % 10 == 0)) {
        if (self->callbacks.get_network_icon && self->network_label) {
            const char* icon = self->callbacks.get_network_icon(self->callbacks.user_data);
            // 只有图标发生变化时才更新显示
            if (icon && self->network_icon != icon) {
                self->network_icon = icon;
                lv_label_set_text(self->network_label, icon);
            }
        }
    }
    
    // 释放显示器锁
    display_lock_guard_destroy(&guard);
}

/**
 * @brief 设置省电模式的实现
 * @details 启用或禁用省电模式，在省电模式下显示睡眠表情
 * @param self 显示器实例指针
 * @param on 是否启用省电模式
 */
static void lvgl_display_set_power_save_mode_impl(LvglDisplay* self, bool on) {
    if (!self) {
        LINX_LOGW(TAG, "Invalid display for set_power_save_mode");
        return;
    }
    
    LINX_LOGI(TAG, "Setting power save mode: %s", on ? "ON" : "OFF");
    
    // 启用省电模式时显示睡眠表情
    if (on) {
        if (self->vtable && self->vtable->set_emotion) {
            self->vtable->set_emotion(self, "sleepy");  // 设置为睡眠表情
        }
        if (self->vtable && self->vtable->set_chat_message) {
            self->vtable->set_chat_message(self, "system", "Entering power save mode...");  // 显示进入省电模式消息
        }
    } else {
        // 禁用省电模式时恢复正常状态
        if (self->vtable && self->vtable->set_emotion) {
            self->vtable->set_emotion(self, "neutral");  // 设置为中性表情
        }
        if (self->vtable && self->vtable->set_chat_message) {
            self->vtable->set_chat_message(self, "system", "Power save mode disabled");  // 显示省电模式已禁用消息
        }
    }
}

/**
 * @brief 截图为JPEG格式的实现
 * @details 占位符实现，用于将显示内容截图并保存为JPEG格式
 * @param self 显示器实例指针
 * @param jpeg_data 用于存储JPEG数据的缓冲区
 * @param data_size 输入时为缓冲区大小，输出时为实际数据大小
 * @param quality JPEG压缩质量（1-100）
 * @return 成功返回true，失败返回false
 */
static bool lvgl_display_snapshot_to_jpeg_impl(LvglDisplay* self, char* jpeg_data, size_t* data_size, int quality) {
    (void)self;       // 未使用的参数
    (void)jpeg_data;  // 未使用的参数
    (void)data_size;  // 未使用的参数
    (void)quality;    // 未使用的参数
    
    LINX_LOGE(TAG, "Snapshot to JPEG not implemented");
    return false;  // 功能未实现
}

/**
 * @brief 锁定显示器的实现
 * @details 占位符实现，用于锁定显示器访问
 * @param self 显示器实例指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功返回true，失败返回false
 */
static bool lvgl_display_lock_impl(LvglDisplay* self, int timeout_ms) {
    (void)self;        // 未使用的参数
    (void)timeout_ms;  // 未使用的参数
    return true;       // 占位符实现，总是返回成功
}

/**
 * @brief 解锁显示器的实现
 * @details 占位符实现，用于解锁显示器访问
 * @param self 显示器实例指针
 */
static void lvgl_display_unlock_impl(LvglDisplay* self) {
    (void)self;  // 未使用的参数
    // 占位符实现
}

/**
 * @brief 销毁显示器的实现
 * @details 清理显示器相关的所有资源，包括定时器、LVGL对象等
 * @param self 显示器实例指针
 */
static void lvgl_display_destroy_impl(LvglDisplay* self) {
    if (!self) return;
    
    // 停止定时器系统
    timer_manager_cleanup(&self->timer_manager);
    
    // 销毁通知定时器
    if (self->notification_timer) {
        simple_timer_destroy(self->notification_timer);
        self->notification_timer = NULL;
    }
    
    // 清理LVGL对象
    if (self->network_label) lv_obj_del(self->network_label);          // 删除网络图标标签
    if (self->notification_label) lv_obj_del(self->notification_label); // 删除通知标签
    if (self->status_label) lv_obj_del(self->status_label);            // 删除状态标签
    if (self->mute_label) lv_obj_del(self->mute_label);                // 删除静音图标标签
    if (self->battery_label) lv_obj_del(self->battery_label);          // 删除电池图标标签
    if (self->low_battery_popup) lv_obj_del(self->low_battery_popup);  // 删除低电量弹窗
    if (self->low_battery_label) lv_obj_del(self->low_battery_label);  // 删除低电量标签
    if (self->emotion_label) lv_obj_del(self->emotion_label);          // 删除表情标签
    if (self->chat_message_label) lv_obj_del(self->chat_message_label); // 删除聊天消息标签
    
    free(self);  // 释放显示器实例内存
}