#include "application.h"
#include "../../board/esp32/esp32_board.h"
#include "../../src/common/linx_log.h"
#include "../../src/common/cjson/cJSON.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include <esp_log.h>

#define TAG "ESP32App"

// Global application instance
static ESP32Application* g_app_instance = NULL;

// State strings for logging
static const char* const STATE_STRINGS[] = {
    "unknown",
    "starting", 
    "wifi_configuring",
    "idle",
    "connecting",
    "listening",
    "speaking",
    "upgrading",
    "activating",
    "audio_testing",
    "fatal_error",
    "invalid_state"
};

// Forward declarations
static void main_event_loop_task(void* arg);
static void check_version_task(void* arg);
static void clock_timer_callback(void* arg);
static void expand_task_queue(ESP32Application* app);

ESP32Application* esp32_app_create(void) {
    if (g_app_instance != NULL) {
        LINX_LOG_WARN(TAG, "Application instance already exists");
        return g_app_instance;
    }
    
    ESP32Application* app = (ESP32Application*)malloc(sizeof(ESP32Application));
    if (!app) {
        LINX_LOG_ERROR(TAG, "Failed to allocate memory for application");
        return NULL;
    }
    
    memset(app, 0, sizeof(ESP32Application));
    
    // Initialize core components
    app->board = esp32_board_create();
    if (!app->board) {
        LINX_LOG_ERROR(TAG, "Failed to create board");
        free(app);
        return NULL;
    }
    
    // Initialize event group
    app->event_group = xEventGroupCreate();
    if (!app->event_group) {
        LINX_LOG_ERROR(TAG, "Failed to create event group");
        esp32_board_destroy(app->board);
        free(app);
        return NULL;
    }
    
    // Initialize task mutex
    app->task_mutex = xSemaphoreCreateMutex();
    if (!app->task_mutex) {
        LINX_LOG_ERROR(TAG, "Failed to create task mutex");
        vEventGroupDelete(app->event_group);
        esp32_board_destroy(app->board);
        free(app);
        return NULL;
    }
    
    // Initialize task queue
    app->task_queue_capacity = 10;
    app->task_queue = (AppTask*)malloc(sizeof(AppTask) * app->task_queue_capacity);
    if (!app->task_queue) {
        LINX_LOG_ERROR(TAG, "Failed to allocate task queue");
        vSemaphoreDelete(app->task_mutex);
        vEventGroupDelete(app->event_group);
        esp32_board_destroy(app->board);
        free(app);
        return NULL;
    }
    
    // Initialize state
    app->device_state = DEVICE_STATE_STARTING;
    app->listening_mode = LISTENING_MODE_AUTO_STOP;
    app->aec_mode = AEC_MODE_OFF;
    app->has_server_time = false;
    app->aborted = false;
    app->voice_detected = false;
    app->clock_ticks = 0;
    
    // Create clock timer
    esp_timer_create_args_t timer_args = {
        .callback = clock_timer_callback,
        .arg = app,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "app_clock",
        .skip_unhandled_events = true
    };
    
    if (esp_timer_create(&timer_args, (esp_timer_handle_t*)&app->clock_timer) != ESP_OK) {
        LINX_LOG_ERROR(TAG, "Failed to create clock timer");
        free(app->task_queue);
        vSemaphoreDelete(app->task_mutex);
        vEventGroupDelete(app->event_group);
        esp32_board_destroy(app->board);
        free(app);
        return NULL;
    }
    
    g_app_instance = app;
    LINX_LOG_INFO(TAG, "ESP32 application created successfully");
    
    return app;
}

void esp32_app_destroy(ESP32Application* app) {
    if (!app) return;
    
    // Stop timer
    if (app->clock_timer) {
        esp_timer_stop((esp_timer_handle_t)app->clock_timer);
        esp_timer_delete((esp_timer_handle_t)app->clock_timer);
    }
    
    // Clean up components
    if (app->audio_service) {
        // audio_service_destroy(app->audio_service);
    }
    
    if (app->protocol) {
        // protocol_destroy(app->protocol);
    }
    
    if (app->ota) {
        // ota_destroy(app->ota);
    }
    
    if (app->mcp_server) {
        // mcp_server_destroy(app->mcp_server);
    }
    
    if (app->board) {
        esp32_board_destroy(app->board);
    }
    
    // Clean up synchronization objects
    if (app->task_mutex) {
        vSemaphoreDelete(app->task_mutex);
    }
    
    if (app->event_group) {
        vEventGroupDelete(app->event_group);
    }
    
    // Clean up memory
    if (app->task_queue) {
        free(app->task_queue);
    }
    
    if (app->last_error_message) {
        free(app->last_error_message);
    }
    
    free(app);
    
    if (g_app_instance == app) {
        g_app_instance = NULL;
    }
    
    LINX_LOG_INFO(TAG, "ESP32 application destroyed");
}

bool esp32_app_start(ESP32Application* app) {
    if (!app) {
        LINX_LOG_ERROR(TAG, "Invalid application instance");
        return false;
    }
    
    LINX_LOG_INFO(TAG, "Starting ESP32 application...");
    
    // Initialize board
    if (!esp32_board_init_wifi(app->board)) {
        LINX_LOG_ERROR(TAG, "Failed to initialize WiFi");
        return false;
    }
    
    if (!esp32_board_init_display(app->board)) {
        LINX_LOG_ERROR(TAG, "Failed to initialize display");
        return false;
    }
    
    if (!esp32_board_init_audio(app->board)) {
        LINX_LOG_ERROR(TAG, "Failed to initialize audio");
        return false;
    }
    
    if (!esp32_board_init_led(app->board)) {
        LINX_LOG_ERROR(TAG, "Failed to initialize LED");
        return false;
    }
    
    // Start clock timer
    if (esp_timer_start_periodic((esp_timer_handle_t)app->clock_timer, 1000000) != ESP_OK) {
        LINX_LOG_ERROR(TAG, "Failed to start clock timer");
        return false;
    }
    
    // Create main event loop task
    if (xTaskCreate(main_event_loop_task, "app_main", 8192, app, 5, 
                   (TaskHandle_t*)&app->main_event_loop_task) != pdPASS) {
        LINX_LOG_ERROR(TAG, "Failed to create main event loop task");
        return false;
    }
    
    // Create version check task
    if (xTaskCreate(check_version_task, "check_version", 4096, app, 3,
                   (TaskHandle_t*)&app->check_version_task) != pdPASS) {
        LINX_LOG_ERROR(TAG, "Failed to create version check task");
        return false;
    }
    
    esp32_app_set_device_state(app, DEVICE_STATE_IDLE);
    
    LINX_LOG_INFO(TAG, "ESP32 application started successfully");
    return true;
}

void esp32_app_main_event_loop(ESP32Application* app) {
    if (!app) return;
    
    EventBits_t bits;
    const TickType_t timeout = pdMS_TO_TICKS(1000);
    
    while (true) {
        bits = xEventGroupWaitBits(app->event_group,
                                  APP_EVENT_SCHEDULE | APP_EVENT_SEND_AUDIO | 
                                  APP_EVENT_WAKE_WORD_DETECTED | APP_EVENT_VAD_CHANGE |
                                  APP_EVENT_ERROR | APP_EVENT_CHECK_VERSION_DONE |
                                  APP_EVENT_CLOCK_TICK,
                                  pdTRUE, pdFALSE, timeout);
        
        if (bits & APP_EVENT_SCHEDULE) {
            // Process scheduled tasks
            if (xSemaphoreTake(app->task_mutex, portMAX_DELAY) == pdTRUE) {
                for (size_t i = 0; i < app->task_queue_size; i++) {
                    if (app->task_queue[i].callback) {
                        app->task_queue[i].callback(app->task_queue[i].user_data);
                    }
                }
                app->task_queue_size = 0;
                xSemaphoreGive(app->task_mutex);
            }
        }
        
        if (bits & APP_EVENT_WAKE_WORD_DETECTED) {
            esp32_app_on_wake_word_detected(app);
        }
        
        if (bits & APP_EVENT_VAD_CHANGE) {
            // Handle voice activity detection change
            LINX_LOG_DEBUG(TAG, "VAD change detected");
        }
        
        if (bits & APP_EVENT_ERROR) {
            LINX_LOG_ERROR(TAG, "Application error occurred");
        }
        
        if (bits & APP_EVENT_CHECK_VERSION_DONE) {
            LINX_LOG_INFO(TAG, "Version check completed");
        }
        
        if (bits & APP_EVENT_CLOCK_TICK) {
            app->clock_ticks++;
            // Handle periodic tasks
        }
        
        // Handle timeout (no events)
        if (bits == 0) {
            // Periodic maintenance tasks
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

DeviceState esp32_app_get_device_state(ESP32Application* app) {
    return app ? app->device_state : DEVICE_STATE_INVALID;
}

void esp32_app_set_device_state(ESP32Application* app, DeviceState state) {
    if (!app || state >= DEVICE_STATE_INVALID) return;
    
    if (app->device_state != state) {
        LINX_LOG_INFO(TAG, "Device state changed: %s -> %s", 
                     STATE_STRINGS[app->device_state], STATE_STRINGS[state]);
        app->device_state = state;
        
        // Update display based on state
        Display* display = board_get_display(app->board);
        if (display) {
            switch (state) {
                case DEVICE_STATE_STARTING:
                    display_set_status(display, "Starting...");
                    break;
                case DEVICE_STATE_WIFI_CONFIGURING:
                    display_set_status(display, "WiFi Config");
                    break;
                case DEVICE_STATE_IDLE:
                    display_set_status(display, "Ready");
                    break;
                case DEVICE_STATE_CONNECTING:
                    display_set_status(display, "Connecting...");
                    break;
                case DEVICE_STATE_LISTENING:
                    display_set_status(display, "Listening...");
                    break;
                case DEVICE_STATE_SPEAKING:
                    display_set_status(display, "Speaking...");
                    break;
                case DEVICE_STATE_UPGRADING:
                    display_set_status(display, "Upgrading...");
                    break;
                case DEVICE_STATE_ACTIVATING:
                    display_set_status(display, "Activating...");
                    break;
                case DEVICE_STATE_AUDIO_TESTING:
                    display_set_status(display, "Audio Test");
                    break;
                case DEVICE_STATE_FATAL_ERROR:
                    display_set_status(display, "Error!");
                    break;
                default:
                    break;
            }
        }
    }
}

bool esp32_app_is_voice_detected(ESP32Application* app) {
    return app ? app->voice_detected : false;
}

void esp32_app_schedule(ESP32Application* app, TaskCallback callback, void* user_data) {
    if (!app || !callback) return;
    
    if (xSemaphoreTake(app->task_mutex, portMAX_DELAY) == pdTRUE) {
        if (app->task_queue_size >= app->task_queue_capacity) {
            expand_task_queue(app);
        }
        
        app->task_queue[app->task_queue_size].callback = callback;
        app->task_queue[app->task_queue_size].user_data = user_data;
        app->task_queue_size++;
        
        xSemaphoreGive(app->task_mutex);
        xEventGroupSetBits(app->event_group, APP_EVENT_SCHEDULE);
    }
}

void esp32_app_alert(ESP32Application* app, const char* status, const char* message,
                     const char* emotion, const char* sound) {
    if (!app) return;
    
    LINX_LOG_WARN(TAG, "Alert [%s] %s: %s", emotion ? emotion : "none", 
                 status ? status : "unknown", message ? message : "");
    
    Display* display = board_get_display(app->board);
    if (display) {
        if (status) display_set_status(display, status);
        if (emotion) display_set_emotion(display, emotion);
        if (message) display_set_chat_message(display, "system", message);
    }
    
    if (sound && app->audio_service) {
        // audio_service_play_sound(app->audio_service, sound);
    }
}

void esp32_app_dismiss_alert(ESP32Application* app) {
    if (!app) return;
    
    if (app->device_state == DEVICE_STATE_IDLE) {
        Display* display = board_get_display(app->board);
        if (display) {
            display_set_status(display, "Ready");
            display_set_emotion(display, "neutral");
            display_set_chat_message(display, "system", "");
        }
    }
}

void esp32_app_on_wake_word_detected(ESP32Application* app) {
    if (!app) return;
    
    LINX_LOG_INFO(TAG, "Wake word detected");
    
    if (app->device_state == DEVICE_STATE_IDLE) {
        esp32_app_start_listening(app);
    }
}

void esp32_app_start_listening(ESP32Application* app) {
    if (!app) return;
    
    LINX_LOG_INFO(TAG, "Starting to listen");
    esp32_app_set_device_state(app, DEVICE_STATE_LISTENING);
    
    // Start audio recording
    if (app->audio_service) {
        // audio_service_start_recording(app->audio_service);
    }
}

void esp32_app_stop_listening(ESP32Application* app) {
    if (!app) return;
    
    LINX_LOG_INFO(TAG, "Stopping listening");
    
    // Stop audio recording
    if (app->audio_service) {
        // audio_service_stop_recording(app->audio_service);
    }
    
    esp32_app_set_device_state(app, DEVICE_STATE_IDLE);
}

ESP32Application* esp32_app_get_instance(void) {
    return g_app_instance;
}

// Private functions
static void main_event_loop_task(void* arg) {
    ESP32Application* app = (ESP32Application*)arg;
    esp32_app_main_event_loop(app);
}

static void check_version_task(void* arg) {
    ESP32Application* app = (ESP32Application*)arg;
    
    // Wait for network to be ready
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    esp32_app_check_new_version(app);
    
    // Task completed, delete itself
    vTaskDelete(NULL);
}

static void clock_timer_callback(void* arg) {
    ESP32Application* app = (ESP32Application*)arg;
    if (app && app->event_group) {
        xEventGroupSetBits(app->event_group, APP_EVENT_CLOCK_TICK);
    }
}

static void expand_task_queue(ESP32Application* app) {
    size_t new_capacity = app->task_queue_capacity * 2;
    AppTask* new_queue = (AppTask*)realloc(app->task_queue, sizeof(AppTask) * new_capacity);
    
    if (new_queue) {
        app->task_queue = new_queue;
        app->task_queue_capacity = new_capacity;
        LINX_LOG_DEBUG(TAG, "Task queue expanded to %zu", new_capacity);
    } else {
        LINX_LOG_ERROR(TAG, "Failed to expand task queue");
    }
}

// Stub implementations for missing functions
void esp32_app_abort_speaking(ESP32Application* app, AbortReason reason) {
    if (!app) return;
    LINX_LOG_INFO(TAG, "Abort speaking, reason: %d", reason);
    esp32_app_set_device_state(app, DEVICE_STATE_IDLE);
}

void esp32_app_toggle_chat_state(ESP32Application* app) {
    if (!app) return;
    
    switch (app->device_state) {
        case DEVICE_STATE_IDLE:
            esp32_app_start_listening(app);
            break;
        case DEVICE_STATE_LISTENING:
            esp32_app_stop_listening(app);
            break;
        case DEVICE_STATE_SPEAKING:
            esp32_app_abort_speaking(app, ABORT_REASON_USER);
            break;
        default:
            break;
    }
}

void esp32_app_reboot(ESP32Application* app) {
    LINX_LOG_INFO(TAG, "Rebooting system...");
    esp_restart();
}

void esp32_app_wake_word_invoke(ESP32Application* app, const char* wake_word) {
    if (!app || !wake_word) return;
    LINX_LOG_INFO(TAG, "Wake word invoked: %s", wake_word);
    esp32_app_on_wake_word_detected(app);
}

bool esp32_app_upgrade_firmware(ESP32Application* app, const char* url) {
    if (!app || !url) return false;
    LINX_LOG_INFO(TAG, "Upgrading firmware from: %s", url);
    // TODO: Implement OTA upgrade
    return false;
}

bool esp32_app_can_enter_sleep_mode(ESP32Application* app) {
    if (!app) return false;
    return app->device_state == DEVICE_STATE_IDLE;
}

void esp32_app_send_mcp_message(ESP32Application* app, const char* payload) {
    if (!app || !payload) return;
    LINX_LOG_DEBUG(TAG, "Sending MCP message: %s", payload);
    // TODO: Implement MCP message sending
}

void esp32_app_set_aec_mode(ESP32Application* app, AecMode mode) {
    if (!app) return;
    app->aec_mode = mode;
    LINX_LOG_INFO(TAG, "AEC mode set to: %d", mode);
}

AecMode esp32_app_get_aec_mode(ESP32Application* app) {
    return app ? app->aec_mode : AEC_MODE_OFF;
}

void esp32_app_play_sound(ESP32Application* app, const char* sound) {
    if (!app || !sound) return;
    LINX_LOG_DEBUG(TAG, "Playing sound: %s", sound);
    // TODO: Implement sound playing
}

void esp32_app_check_new_version(ESP32Application* app) {
    if (!app) return;
    LINX_LOG_INFO(TAG, "Checking for new version...");
    // TODO: Implement version checking
    xEventGroupSetBits(app->event_group, APP_EVENT_CHECK_VERSION_DONE);
}

void esp32_app_check_assets_version(ESP32Application* app) {
    if (!app) return;
    LINX_LOG_INFO(TAG, "Checking assets version...");
    // TODO: Implement assets version checking
}

void esp32_app_show_activation_code(ESP32Application* app, const char* code, const char* message) {
    if (!app || !code) return;
    LINX_LOG_INFO(TAG, "Showing activation code: %s", code);
    esp32_app_alert(app, "Activation", message ? message : code, "link", NULL);
}

void esp32_app_set_listening_mode(ESP32Application* app, ListeningMode mode) {
    if (!app) return;
    app->listening_mode = mode;
    LINX_LOG_INFO(TAG, "Listening mode set to: %d", mode);
}

void esp32_app_on_vad_change(ESP32Application* app, bool voice_detected) {
    if (!app) return;
    app->voice_detected = voice_detected;
    LINX_LOG_DEBUG(TAG, "Voice detected: %s", voice_detected ? "true" : "false");
}

void esp32_app_on_audio_data(ESP32Application* app, const uint8_t* data, size_t length) {
    if (!app || !data) return;
    // TODO: Process audio data
}

void esp32_app_on_protocol_message(ESP32Application* app, const char* message) {
    if (!app || !message) return;
    LINX_LOG_DEBUG(TAG, "Protocol message: %s", message);
    // TODO: Process protocol message
}

void esp32_app_on_error(ESP32Application* app, const char* error_message) {
    if (!app || !error_message) return;
    
    LINX_LOG_ERROR(TAG, "Application error: %s", error_message);
    
    // Store error message
    if (app->last_error_message) {
        free(app->last_error_message);
    }
    app->last_error_message = strdup(error_message);
    
    esp32_app_set_device_state(app, DEVICE_STATE_FATAL_ERROR);
    xEventGroupSetBits(app->event_group, APP_EVENT_ERROR);
}