/**
 * @file lvgl_display_demo.c
 * @brief LVGL Display Demo - 展示 lvgl_display 模块的各种功能
 * @details 这个演示程序展示了 lvgl_display 模块的所有主要功能，包括：
 *          - 状态显示和更新
 *          - 通知消息系统
 *          - 表情显示
 *          - 聊天消息
 *          - 状态栏管理
 *          - 省电模式
 *          - 截图功能
 *          - 线程安全的锁定机制
 * @author LinX OS SDK Team
 * @version 1.0
 */

#include "../lvgl_display.h"
#include "../../log/linx_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define TAG "DEMO"

// 全局变量
static LvglDisplay* g_display = NULL;
static volatile bool g_running = true;
static pthread_t g_demo_thread;

// 演示用的状态消息
static const char* demo_status_messages[] = {
    "系统启动中...",
    "正在连接网络...",
    "网络连接成功",
    "正在初始化设备...",
    "设备初始化完成",
    "系统就绪",
    "运行中...",
    "处理用户请求",
    "数据同步中...",
    "系统正常运行"
};

// 演示用的通知消息
static const char* demo_notifications[] = {
    "欢迎使用 LVGL Display Demo!",
    "新消息到达",
    "系统更新可用",
    "电池电量充足",
    "WiFi 连接成功",
    "蓝牙设备已连接",
    "文件下载完成",
    "备份已完成",
    "设置已保存",
    "演示即将结束"
};

// 演示用的表情
static const char* demo_emotions[] = {
    "😊", "😎", "🤔", "😴", "🚀", 
    "💡", "🎉", "❤️", "👍", "🔥"
};

// 演示用的聊天消息
typedef struct {
    const char* role;
    const char* content;
} ChatMessage;

static const ChatMessage demo_chat_messages[] = {
    {"user", "你好，请介绍一下这个系统"},
    {"assistant", "您好！这是 LinX OS SDK 的显示模块演示程序。"},
    {"user", "这个显示模块有什么功能？"},
    {"assistant", "显示模块支持状态显示、通知消息、表情、聊天界面等功能。"},
    {"user", "能展示一下截图功能吗？"},
    {"assistant", "当然可以！截图功能可以将当前屏幕保存为 JPEG 格式。"},
    {"system", "系统提示：演示程序运行正常"},
    {"user", "谢谢演示！"},
    {"assistant", "不客气！希望这个演示对您有帮助。"}
};

/**
 * @brief 信号处理函数
 * @details 处理 Ctrl+C 信号，优雅地退出程序
 * @param sig 信号编号
 */
static void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\n🛑 收到退出信号，正在优雅地关闭演示程序...\n");
        g_running = false;
    }
}

/**
 * @brief 模拟状态提供者回调函数
 * @details 这些回调函数模拟真实系统中的状态获取
 */

// 获取电池状态
static bool get_battery_level(int* level, bool* charging, bool* discharging, void* user_data) {
    (void)user_data;
    static int battery = 85;
    static int direction = -1;
    static bool is_charging = false;
    
    battery += direction * (rand() % 3);
    if (battery <= 60) {
        battery = 60;
        direction = 1;
        is_charging = true;
    } else if (battery >= 100) {
        battery = 100;
        direction = -1;
        is_charging = false;
    }
    
    *level = battery;
    *charging = is_charging;
    *discharging = !is_charging && battery < 95;
    return true;
}

// 获取网络图标
static const char* get_network_icon(void* user_data) {
    (void)user_data;
    static int counter = 0;
    const char* icons[] = {"wifi_off", "wifi_1", "wifi_2", "wifi_3", "wifi_full"};
    counter++;
    if ((counter % 20) == 0) {
        return icons[0]; // 偶尔断开连接
    }
    return icons[1 + (counter % 4)];
}

// 获取音量等级
static int get_volume_level(void* user_data) {
    (void)user_data;
    static int level = 50;
    static int direction = 1;
    
    level += direction * (rand() % 5);
    if (level <= 0) {
        level = 0;
        direction = 1;
    } else if (level >= 100) {
        level = 100;
        direction = -1;
    }
    
    return level;
}

// 检查设备是否空闲
static bool is_device_idle(void* user_data) {
    (void)user_data;
    static int counter = 0;
    counter++;
    return (counter % 4) == 0; // 模拟设备空闲状态
}

// 播放低电量提示音
static void play_low_battery_sound(void* user_data) {
    (void)user_data;
    printf("🔋 播放低电量提示音\n");
}

/**
 * @brief 演示线程函数
 * @details 在独立线程中运行各种演示功能
 * @param arg 线程参数（未使用）
 * @return NULL
 */
static void* demo_thread_func(void* arg) {
    (void)arg; // 避免未使用参数警告
    
    printf("🎬 演示线程启动\n");
    
    int step = 0;
    int substep = 0;
    
    while (g_running) {
        if (!g_display) {
            usleep(100000); // 100ms
            continue;
        }
        
        // 锁定显示器进行操作
        if (!lvgl_display_lock(g_display, 1000)) {
            printf("⚠️  无法锁定显示器，跳过此次更新\n");
            usleep(500000); // 500ms
            continue;
        }
        
        switch (step % 6) {
            case 0: {
                // 演示状态消息
                int msg_idx = substep % (sizeof(demo_status_messages) / sizeof(demo_status_messages[0]));
                printf("📝 设置状态消息: %s\n", demo_status_messages[msg_idx]);
                lvgl_display_set_status(g_display, demo_status_messages[msg_idx]);
                break;
            }
            
            case 1: {
                // 演示通知消息
                int msg_idx = substep % (sizeof(demo_notifications) / sizeof(demo_notifications[0]));
                printf("📢 显示通知: %s\n", demo_notifications[msg_idx]);
                lvgl_display_show_notification(g_display, demo_notifications[msg_idx], 3000);
                break;
            }
            
            case 2: {
                // 演示表情显示
                int emotion_idx = substep % (sizeof(demo_emotions) / sizeof(demo_emotions[0]));
                printf("😊 显示表情: %s\n", demo_emotions[emotion_idx]);
                lvgl_display_set_emotion(g_display, demo_emotions[emotion_idx]);
                break;
            }
            
            case 3: {
                // 演示聊天消息
                int msg_idx = substep % (sizeof(demo_chat_messages) / sizeof(demo_chat_messages[0]));
                const ChatMessage* msg = &demo_chat_messages[msg_idx];
                printf("💬 聊天消息 [%s]: %s\n", msg->role, msg->content);
                lvgl_display_set_chat_message(g_display, msg->role, msg->content);
                break;
            }
            
            case 4: {
                // 演示状态栏更新
                printf("🔄 更新状态栏\n");
                lvgl_display_update_status_bar(g_display, substep % 2 == 0);
                break;
            }
            
            case 5: {
                // 演示省电模式切换
                bool power_save = (substep % 4) < 2;
                printf("🔋 %s省电模式\n", power_save ? "开启" : "关闭");
                lvgl_display_set_power_save_mode(g_display, power_save);
                break;
            }
        }
        
        // 解锁显示器
        lvgl_display_unlock(g_display);
        
        substep++;
        if (substep >= 10) {
            substep = 0;
            step++;
        }
        
        // 每2秒切换一次演示内容
        usleep(2000000); // 2秒
    }
    
    printf("🎬 演示线程结束\n");
    return NULL;
}

/**
 * @brief 演示截图功能
 * @details 测试显示器的截图功能
 */
static void demo_screenshot(void) {
    if (!g_display) {
        printf("❌ 显示器未初始化\n");
        return;
    }
    
    printf("📸 开始截图演示...\n");
    
    // 分配截图缓冲区 (假设最大 100KB)
    size_t buffer_size = 100 * 1024;
    char* jpeg_buffer = malloc(buffer_size);
    if (!jpeg_buffer) {
        printf("❌ 无法分配截图缓冲区\n");
        return;
    }
    
    // 锁定显示器
    if (!lvgl_display_lock(g_display, 5000)) {
        printf("❌ 无法锁定显示器进行截图\n");
        free(jpeg_buffer);
        return;
    }
    
    // 执行截图
    size_t actual_size = buffer_size;
    bool success = lvgl_display_snapshot_to_jpeg(g_display, jpeg_buffer, &actual_size, 85);
    
    // 解锁显示器
    lvgl_display_unlock(g_display);
    
    if (success) {
        printf("✅ 截图成功！大小: %zu 字节\n", actual_size);
        
        // 保存到文件
        FILE* file = fopen("screenshot.jpg", "wb");
        if (file) {
            fwrite(jpeg_buffer, 1, actual_size, file);
            fclose(file);
            printf("💾 截图已保存为 screenshot.jpg\n");
        } else {
            printf("⚠️  无法保存截图文件\n");
        }
    } else {
        printf("❌ 截图失败\n");
    }
    
    free(jpeg_buffer);
}

/**
 * @brief 设置状态提供者回调
 * @details 配置显示器的状态回调函数
 */
static void setup_status_callbacks(void) {
    LvglDisplayCallbacks callbacks = {
        .get_battery_level = get_battery_level,
        .get_network_icon = get_network_icon,
        .get_volume_level = get_volume_level,
        .is_device_idle = is_device_idle,
        .play_low_battery_sound = play_low_battery_sound,
        .user_data = NULL
    };
    
    lvgl_display_set_callbacks(g_display, &callbacks);
    printf("✅ 状态回调函数已设置\n");
}

/**
 * @brief 打印使用说明
 */
static void print_usage(void) {
    printf("\n");
    printf("🚀 LVGL Display Demo 使用说明\n");
    printf("==============================\n");
    printf("这个演示程序将自动展示以下功能：\n");
    printf("📝 状态消息显示\n");
    printf("📢 通知消息系统\n");
    printf("😊 表情符号显示\n");
    printf("💬 聊天消息界面\n");
    printf("🔄 状态栏更新\n");
    printf("🔋 省电模式切换\n");
    printf("📸 截图功能\n");
    printf("\n");
    printf("💡 提示：\n");
    printf("- 程序会自动循环演示各种功能\n");
    printf("- 使用 Ctrl+C 可以优雅地退出程序\n");
    printf("- 截图将保存为 screenshot.jpg 文件\n");
    printf("\n");
}

/**
 * @brief 主函数
 * @details 程序入口点，初始化显示器并运行演示
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char* argv[]) {
    (void)argc; // 避免未使用参数警告
    (void)argv;
    
    printf("🚀 启动 LVGL Display Demo\n");
    printf("==============================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    
    // 初始化随机数种子
    srand(time(NULL));
    
    // 打印使用说明
    print_usage();
    
    // 创建显示器实例
    printf("🔧 创建 LVGL 显示器实例...\n");
    g_display = lvgl_display_create();
    if (!g_display) {
        printf("❌ 创建显示器失败\n");
        return EXIT_FAILURE;
    }
    
    // 初始化显示器
    printf("🔧 初始化显示器...\n");
    if (!lvgl_display_init(g_display)) {
        printf("❌ 初始化显示器失败\n");
        lvgl_display_destroy(g_display);
        return EXIT_FAILURE;
    }
    
    printf("✅ LVGL 显示器初始化成功\n");
    
    // 设置状态回调
    setup_status_callbacks();
    
    // 启动演示线程
    printf("🎬 启动演示线程...\n");
    if (pthread_create(&g_demo_thread, NULL, demo_thread_func, NULL) != 0) {
        printf("❌ 创建演示线程失败\n");
        lvgl_display_destroy(g_display);
        return EXIT_FAILURE;
    }
    
    printf("✅ 演示程序启动成功\n");
    printf("💡 使用 Ctrl+C 退出程序\n");
    printf("📱 正在运行演示...\n\n");
    
    // 主循环
    int screenshot_counter = 0;
    while (g_running) {
        // 每30秒进行一次截图演示
        if (screenshot_counter >= 15) { // 15 * 2秒 = 30秒
            demo_screenshot();
            screenshot_counter = 0;
        }
        
        screenshot_counter++;
        sleep(2); // 每2秒检查一次
    }
    
    // 清理资源
    printf("\n🧹 正在清理资源...\n");
    
    // 等待演示线程结束
    if (pthread_join(g_demo_thread, NULL) != 0) {
        printf("⚠️  等待演示线程结束时出错\n");
    }
    
    // 销毁显示器
    if (g_display) {
        lvgl_display_destroy(g_display);
        g_display = NULL;
    }
    
    printf("✅ 资源清理完成\n");
    printf("👋 LVGL Display Demo 已退出\n");
    
    return EXIT_SUCCESS;
}