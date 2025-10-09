#ifndef ESP32_APPLICATION_H
#define ESP32_APPLICATION_H

#include "../../src/board/board.h"
#include "../../src/audio/audio_service.h"
#include "../../src/common/settings.h"
#include "../../src/linxsdk/linx_sdk.h"
#include "../../src/linxsdk/ota/ota.h"
#include "../../src/linxsdk/protocols/protocol.h"
#include "../../src/linxsdk/mcp/mcp_server.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device states
typedef enum {
    DEVICE_STATE_UNKNOWN = 0,
    DEVICE_STATE_STARTING,
    DEVICE_STATE_WIFI_CONFIGURING,
    DEVICE_STATE_IDLE,
    DEVICE_STATE_CONNECTING,
    DEVICE_STATE_LISTENING,
    DEVICE_STATE_SPEAKING,
    DEVICE_STATE_UPGRADING,
    DEVICE_STATE_ACTIVATING,
    DEVICE_STATE_AUDIO_TESTING,
    DEVICE_STATE_FATAL_ERROR,
    DEVICE_STATE_INVALID
} DeviceState;

// Listening modes
typedef enum {
    LISTENING_MODE_AUTO_STOP = 0,
    LISTENING_MODE_MANUAL_STOP,
    LISTENING_MODE_CONTINUOUS
} ListeningMode;

// AEC modes
typedef enum {
    AEC_MODE_OFF = 0,
    AEC_MODE_DEVICE_SIDE,
    AEC_MODE_SERVER_SIDE
} AecMode;

// Abort reasons
typedef enum {
    ABORT_REASON_USER = 0,
    ABORT_REASON_WAKE_WORD,
    ABORT_REASON_ERROR,
    ABORT_REASON_TIMEOUT
} AbortReason;

// Application events
#define APP_EVENT_SCHEDULE              (1 << 0)
#define APP_EVENT_SEND_AUDIO            (1 << 1)
#define APP_EVENT_WAKE_WORD_DETECTED    (1 << 2)
#define APP_EVENT_VAD_CHANGE            (1 << 3)
#define APP_EVENT_ERROR                 (1 << 4)
#define APP_EVENT_CHECK_VERSION_DONE    (1 << 5)
#define APP_EVENT_CLOCK_TICK            (1 << 6)

// Task callback function type
typedef void (*TaskCallback)(void* user_data);

// Task structure
typedef struct {
    TaskCallback callback;
    void* user_data;
} AppTask;

// Application structure
typedef struct ESP32Application {
    // Core components
    Board* board;
    AudioService* audio_service;
    Protocol* protocol;
    OTA* ota;
    MCPServer* mcp_server;
    
    // State management
    DeviceState device_state;
    ListeningMode listening_mode;
    AecMode aec_mode;
    
    // Event handling
    void* event_group;
    void* clock_timer;
    
    // Task management
    AppTask* task_queue;
    size_t task_queue_size;
    size_t task_queue_capacity;
    void* task_mutex;
    
    // Status flags
    bool has_server_time;
    bool aborted;
    bool voice_detected;
    int clock_ticks;
    
    // Error handling
    char* last_error_message;
    
    // Task handles
    void* check_version_task;
    void* main_event_loop_task;
    
} ESP32Application;

// Application lifecycle
ESP32Application* esp32_app_create(void);
void esp32_app_destroy(ESP32Application* app);
bool esp32_app_start(ESP32Application* app);
void esp32_app_main_event_loop(ESP32Application* app);

// State management
DeviceState esp32_app_get_device_state(ESP32Application* app);
void esp32_app_set_device_state(ESP32Application* app, DeviceState state);
bool esp32_app_is_voice_detected(ESP32Application* app);

// Task scheduling
void esp32_app_schedule(ESP32Application* app, TaskCallback callback, void* user_data);

// User interaction
void esp32_app_alert(ESP32Application* app, const char* status, const char* message, 
                     const char* emotion, const char* sound);
void esp32_app_dismiss_alert(ESP32Application* app);
void esp32_app_abort_speaking(ESP32Application* app, AbortReason reason);
void esp32_app_toggle_chat_state(ESP32Application* app);
void esp32_app_start_listening(ESP32Application* app);
void esp32_app_stop_listening(ESP32Application* app);

// System operations
void esp32_app_reboot(ESP32Application* app);
void esp32_app_wake_word_invoke(ESP32Application* app, const char* wake_word);
bool esp32_app_upgrade_firmware(ESP32Application* app, const char* url);
bool esp32_app_can_enter_sleep_mode(ESP32Application* app);

// Communication
void esp32_app_send_mcp_message(ESP32Application* app, const char* payload);

// Audio settings
void esp32_app_set_aec_mode(ESP32Application* app, AecMode mode);
AecMode esp32_app_get_aec_mode(ESP32Application* app);
void esp32_app_play_sound(ESP32Application* app, const char* sound);

// Version and asset management
void esp32_app_check_new_version(ESP32Application* app);
void esp32_app_check_assets_version(ESP32Application* app);

// Activation and configuration
void esp32_app_show_activation_code(ESP32Application* app, const char* code, const char* message);
void esp32_app_set_listening_mode(ESP32Application* app, ListeningMode mode);

// Event handlers
void esp32_app_on_wake_word_detected(ESP32Application* app);
void esp32_app_on_vad_change(ESP32Application* app, bool voice_detected);
void esp32_app_on_audio_data(ESP32Application* app, const uint8_t* data, size_t length);
void esp32_app_on_protocol_message(ESP32Application* app, const char* message);
void esp32_app_on_error(ESP32Application* app, const char* error_message);

// Singleton pattern support
ESP32Application* esp32_app_get_instance(void);

#ifdef __cplusplus
}
#endif

#endif // ESP32_APPLICATION_H