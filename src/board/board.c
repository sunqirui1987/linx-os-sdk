#include "board.h"
#include "../common/log/linx_log.h"
#include "../common/settings.h"
#include "../common/system_info.h"
#include "../display/display.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define TAG "Board"

// Forward declaration for external create_board function
extern Board* create_board(void);

// Extended board data structure
typedef struct {
    Settings* settings;
    DisplayInterface* display;
    bool power_save_mode;
    char* cached_system_info_json;
    char* cached_board_json;
    char* cached_device_status_json;
} BoardData;

// Default implementations for virtual functions

static const char* board_default_get_uuid(Board* self) {
    return self->uuid_;
}

static void* board_default_get_backlight(Board* self) {
    LINX_LOGW(TAG, "No backlight implementation available");
    return NULL;
}

static void* board_default_get_led(Board* self) {
    LINX_LOGW(TAG, "No LED implementation available");
    return NULL;
}

static bool board_default_get_temperature(Board* self, float* temperature) {
    LINX_LOGW(TAG, "No temperature sensor implementation available");
    return false;
}

static void* board_default_get_display(Board* self) {
    if (!self || !self->data) {
        LINX_LOGW(TAG, "No display implementation available");
        return NULL;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    if (!board_data->display) {
        LINX_LOGI(TAG, "Initializing display interface");
        board_data->display = display_interface_create();
        if (board_data->display) {
            display_interface_init(board_data->display);
        }
    }
    
    return board_data->display;
}

static void* board_default_get_camera(Board* self) {
    LINX_LOGW(TAG, "No camera implementation available");
    return NULL;
}

static bool board_default_get_battery_level(Board* self, int* level, bool* charging, bool* discharging) {
    LINX_LOGW(TAG, "No battery level implementation available");
    return false;
}

static const char* board_default_get_system_info_json(Board* self) {
    if (!self || !self->data) {
        return NULL;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    
    // Return cached version if available
    if (board_data->cached_system_info_json) {
        return board_data->cached_system_info_json;
    }
    
    // Generate comprehensive system info using system_info module
    char mac_str[18] = {0};
    char sha256_str[65] = {0};
    float temperature = 0.0f;
    bool has_temp = system_info_get_temperature(&temperature);
    
    system_info_get_mac_address(mac_str, sizeof(mac_str));
    system_info_get_app_elf_sha256(sha256_str, sizeof(sha256_str));
    
    // Allocate buffer for JSON
    board_data->cached_system_info_json = malloc(2048);
    if (!board_data->cached_system_info_json) {
        LINX_LOGE(TAG, "Failed to allocate memory for system info JSON");
        return NULL;
    }
    
    snprintf(board_data->cached_system_info_json, 2048,
        "{"
        "\"version\":2,"
        "\"uuid\":\"%s\","
        "\"board_type\":\"%s\","
        "\"chip_model\":\"%s\","
        "\"chip_revision\":%u,"
        "\"cpu_cores\":%u,"
        "\"cpu_freq_hz\":%u,"
        "\"flash_size\":%u,"
        "\"free_heap\":%u,"
        "\"min_free_heap\":%u,"
        "\"psram_size\":%u,"
        "\"free_psram\":%u,"
        "\"mac_address\":\"%s\","
        "\"app_name\":\"%s\","
        "\"app_version\":\"%s\","
        "\"compile_date\":\"%s\","
        "\"compile_time\":\"%s\","
        "\"idf_version\":\"%s\","
        "\"app_sha256\":\"%s\","
        "\"uptime_ms\":%llu,"
        "\"reset_reason\":\"%s\""
        "%s%.2f"
        "}",
        self->uuid_ ? self->uuid_ : "unknown",
        board_get_board_type(self) ? board_get_board_type(self) : "unknown",
        system_info_get_chip_model_name(),
        system_info_get_chip_revision(),
        system_info_get_cpu_cores(),
        system_info_get_cpu_freq_hz(),
        system_info_get_flash_size(),
        system_info_get_free_heap_size(),
        system_info_get_minimum_free_heap_size(),
        system_info_get_psram_size(),
        system_info_get_free_psram_size(),
        mac_str,
        system_info_get_app_name(),
        system_info_get_app_version(),
        system_info_get_app_compile_date(),
        system_info_get_app_compile_time(),
        system_info_get_idf_version(),
        sha256_str,
        system_info_get_uptime_ms(),
        system_info_get_reset_reason(),
        has_temp ? ",\"temperature\":" : "",
        has_temp ? temperature : 0.0f
    );
    
    return board_data->cached_system_info_json;
}

static void board_default_set_power_save_mode(Board* self, bool enabled) {
    if (!self || !self->data) {
        LINX_LOGW(TAG, "Invalid board or board data");
        return;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    board_data->power_save_mode = enabled;
    
    LINX_LOGI(TAG, "Power save mode %s", enabled ? "enabled" : "disabled");
    
    // If display is available, set its power save mode
    if (board_data->display) {
        display_interface_set_power_save_mode(board_data->display, enabled);
    }
}

static const char* board_default_get_board_json(Board* self) {
    if (!self || !self->data) {
        return NULL;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    
    // Return cached version if available
    if (board_data->cached_board_json) {
        return board_data->cached_board_json;
    }
    
    // Generate board JSON
    board_data->cached_board_json = malloc(1024);
    if (!board_data->cached_board_json) {
        LINX_LOGE(TAG, "Failed to allocate memory for board JSON");
        return NULL;
    }
    
    snprintf(board_data->cached_board_json, 1024,
        "{"
        "\"board_type\":\"%s\","
        "\"uuid\":\"%s\","
        "\"power_save_mode\":%s,"
        "\"has_display\":%s,"
        "\"has_camera\":%s,"
        "\"has_backlight\":%s,"
        "\"has_led\":%s"
        "}",
        board_get_board_type(self) ? board_get_board_type(self) : "unknown",
        self->uuid_ ? self->uuid_ : "unknown",
        board_data->power_save_mode ? "true" : "false",
        board_data->display ? "true" : "false",
        "false",  // Camera availability would need to be checked
        "false",  // Backlight availability would need to be checked
        "false"   // LED availability would need to be checked
    );
    
    return board_data->cached_board_json;
}

static const char* board_default_get_device_status_json(Board* self) {
    if (!self || !self->data) {
        return NULL;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    
    // Return cached version if available
    if (board_data->cached_device_status_json) {
        return board_data->cached_device_status_json;
    }
    
    // Generate device status JSON
    board_data->cached_device_status_json = malloc(1024);
    if (!board_data->cached_device_status_json) {
        LINX_LOGE(TAG, "Failed to allocate memory for device status JSON");
        return NULL;
    }
    
    float temperature = 0.0f;
    bool has_temp = system_info_get_temperature(&temperature);
    int battery_level = 0;
    bool charging = false, discharging = false;
    bool has_battery = board_get_battery_level(self, &battery_level, &charging, &discharging);
    
    snprintf(board_data->cached_device_status_json, 1024,
        "{"
        "\"uptime_ms\":%llu,"
        "\"free_heap\":%u,"
        "\"power_save_mode\":%s"
        "%s%.2f"
        "%s%d"
        "%s%s"
        "%s%s"
        "}",
        system_info_get_uptime_ms(),
        system_info_get_free_heap_size(),
        board_data->power_save_mode ? "true" : "false",
        has_temp ? ",\"temperature\":" : "",
        has_temp ? temperature : 0.0f,
        has_battery ? ",\"battery_level\":" : "",
        has_battery ? battery_level : 0,
        has_battery ? ",\"charging\":" : "",
        has_battery ? (charging ? "true" : "false") : "",
        has_battery ? ",\"discharging\":" : "",
        has_battery ? (discharging ? "true" : "false") : ""
    );
    
    return board_data->cached_device_status_json;
}

static void board_default_destroy(Board* self) {
    if (self) {
        if (self->data) {
            BoardData* board_data = (BoardData*)self->data;
            
            // Clean up settings
            if (board_data->settings) {
                settings_destroy(board_data->settings);
            }
            
            // Clean up display
            if (board_data->display) {
                display_interface_destroy(board_data->display);
            }
            
            // Clean up cached JSON strings
            if (board_data->cached_system_info_json) {
                free(board_data->cached_system_info_json);
            }
            if (board_data->cached_board_json) {
                free(board_data->cached_board_json);
            }
            if (board_data->cached_device_status_json) {
                free(board_data->cached_device_status_json);
            }
            
            free(self->data);
        }
        
        if (self->uuid_) {
            free(self->uuid_);
        }
        free(self);
    }
}

// Default vtable with default implementations
static const BoardVTable board_default_vtable = {
    .get_board_type = NULL,  // Pure virtual - must be implemented
    .get_audio_codec = NULL,  // Pure virtual - must be implemented
    .get_network = NULL,  // Pure virtual - must be implemented
    .start_network = NULL,  // Pure virtual - must be implemented
    .get_network_state_icon = NULL,  // Pure virtual - must be implemented
    .set_power_save_mode = board_default_set_power_save_mode,
    .get_board_json = board_default_get_board_json,
    .get_device_status_json = board_default_get_device_status_json,
    
    .get_uuid = board_default_get_uuid,
    .get_backlight = board_default_get_backlight,
    .get_led = board_default_get_led,
    .get_temperature = board_default_get_temperature,
    .get_display = board_default_get_display,
    .get_camera = board_default_get_camera,
    .get_battery_level = board_default_get_battery_level,
    .get_system_info_json = board_default_get_system_info_json,
    .destroy = board_default_destroy
};

// Protected method: Generate UUID
char* board_generate_uuid(void) {
    char* uuid = malloc(37);  // UUID string length + null terminator
    if (!uuid) {
        LINX_LOGE(TAG, "Failed to allocate memory for UUID");
        return NULL;
    }
    
    // Generate a simple UUID v4 (random)
    srand(time(NULL));
    snprintf(uuid, 37, 
        "%08x-%04x-4%03x-%04x-%012x",
        rand(), rand() & 0xFFFF, rand() & 0x0FFF,
        (rand() & 0x3FFF) | 0x8000, rand());
    
    LINX_LOGI(TAG, "Generated UUID: %s", uuid);
    return uuid;
}

// Board constructor
Board* board_create(void) {
    Board* self = malloc(sizeof(Board));
    if (!self) {
        LINX_LOGE(TAG, "Failed to allocate memory for Board");
        return NULL;
    }
    
    // Initialize BoardData
    BoardData* board_data = malloc(sizeof(BoardData));
    if (!board_data) {
        LINX_LOGE(TAG, "Failed to allocate memory for BoardData");
        free(self);
        return NULL;
    }
    
    // Initialize BoardData members
    board_data->settings = settings_create("board", true);  // Auto-save enabled
    board_data->display = NULL;  // Lazy initialization
    board_data->power_save_mode = false;
    board_data->cached_system_info_json = NULL;
    board_data->cached_board_json = NULL;
    board_data->cached_device_status_json = NULL;
    
    self->vtable = &board_default_vtable;
    self->uuid_ = board_generate_uuid();
    self->data = board_data;
    
    LINX_LOGI(TAG, "Board created with UUID: %s", self->uuid_);
    return self;
}

// Board destructor
void board_destroy(Board* self) {
    if (!self) {
        LINX_LOGW(TAG, "Attempting to destroy NULL board");
        return;
    }
    
    if (self->vtable && self->vtable->destroy) {
        self->vtable->destroy(self);
    } else {
        board_default_destroy(self);
    }
}

// Singleton pattern implementation
Board* board_get_instance(void) {
    static Board* instance = NULL;
    if (!instance) {
        instance = create_board();
        if (!instance) {
            LINX_LOGE(TAG, "Failed to create board instance");
        }
    }
    return instance;
}

// Public method implementations - these call the vtable functions

const char* board_get_board_type(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_board_type) {
        LINX_LOGE(TAG, "Invalid board or get_board_type function not implemented");
        return NULL;
    }
    return self->vtable->get_board_type(self);
}

const char* board_get_uuid(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_uuid) {
        LINX_LOGE(TAG, "Invalid board or get_uuid function");
        return NULL;
    }
    return self->vtable->get_uuid(self);
}

void* board_get_backlight(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_backlight) {
        LINX_LOGW(TAG, "Invalid board or get_backlight function");
        return NULL;
    }
    return self->vtable->get_backlight(self);
}

void* board_get_led(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_led) {
        LINX_LOGW(TAG, "Invalid board or get_led function");
        return NULL;
    }
    return self->vtable->get_led(self);
}

void* board_get_audio_codec(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_audio_codec) {
        LINX_LOGE(TAG, "Invalid board or get_audio_codec function not implemented");
        return NULL;
    }
    return self->vtable->get_audio_codec(self);
}

bool board_get_temperature(Board* self, float* temperature) {
    if (!self || !self->vtable || !self->vtable->get_temperature) {
        LINX_LOGW(TAG, "Invalid board or get_temperature function");
        return false;
    }
    return self->vtable->get_temperature(self, temperature);
}

void* board_get_display(Board* self) {
    if (!self || !self->vtable) {
        LINX_LOGW(TAG, "Invalid board or vtable");
        return NULL;
    }
    
    if (self->vtable->get_display) {
        return self->vtable->get_display(self);
    } else {
        // Use default implementation
        return board_default_get_display(self);
    }
}

void* board_get_camera(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_camera) {
        LINX_LOGW(TAG, "Invalid board or get_camera function");
        return NULL;
    }
    return self->vtable->get_camera(self);
}

void* board_get_network(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_network) {
        LINX_LOGE(TAG, "Invalid board or get_network function not implemented");
        return NULL;
    }
    return self->vtable->get_network(self);
}

void board_start_network(Board* self) {
    if (!self || !self->vtable || !self->vtable->start_network) {
        LINX_LOGE(TAG, "Invalid board or start_network function not implemented");
        return;
    }
    self->vtable->start_network(self);
}

const char* board_get_network_state_icon(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_network_state_icon) {
        LINX_LOGE(TAG, "Invalid board or get_network_state_icon function not implemented");
        return NULL;
    }
    return self->vtable->get_network_state_icon(self);
}

bool board_get_battery_level(Board* self, int* level, bool* charging, bool* discharging) {
    if (!self || !self->vtable || !self->vtable->get_battery_level) {
        LINX_LOGW(TAG, "Invalid board or get_battery_level function");
        return false;
    }
    return self->vtable->get_battery_level(self, level, charging, discharging);
}

const char* board_get_system_info_json(Board* self) {
    if (!self || !self->vtable) {
        LINX_LOGW(TAG, "Invalid board or vtable");
        return NULL;
    }
    
    if (self->vtable->get_system_info_json) {
        return self->vtable->get_system_info_json(self);
    } else {
        // Use default implementation
        return board_default_get_system_info_json(self);
    }
}

void board_set_power_save_mode(Board* self, bool enabled) {
    if (!self || !self->vtable) {
        LINX_LOGW(TAG, "Invalid board or vtable");
        return;
    }
    
    if (self->vtable->set_power_save_mode) {
        self->vtable->set_power_save_mode(self, enabled);
    } else {
        // Use default implementation
        board_default_set_power_save_mode(self, enabled);
    }
}

const char* board_get_board_json(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_board_json) {
        LINX_LOGE(TAG, "Invalid board or get_board_json function not implemented");
        return NULL;
    }
    return self->vtable->get_board_json(self);
}

const char* board_get_device_status_json(Board* self) {
    if (!self || !self->vtable || !self->vtable->get_device_status_json) {
        LINX_LOGE(TAG, "Invalid board or get_device_status_json function not implemented");
        return NULL;
    }
    return self->vtable->get_device_status_json(self);
}

// Additional utility functions for board management

Settings* board_get_settings(Board* self) {
    if (!self || !self->data) {
        LINX_LOGE(TAG, "Invalid board or board data");
        return NULL;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    return board_data->settings;
}

bool board_get_power_save_mode(Board* self) {
    if (!self || !self->data) {
        LINX_LOGW(TAG, "Invalid board or board data");
        return false;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    return board_data->power_save_mode;
}

void board_invalidate_cache(Board* self) {
    if (!self || !self->data) {
        return;
    }
    
    BoardData* board_data = (BoardData*)self->data;
    
    // Free cached JSON strings to force regeneration
    if (board_data->cached_system_info_json) {
        free(board_data->cached_system_info_json);
        board_data->cached_system_info_json = NULL;
    }
    if (board_data->cached_board_json) {
        free(board_data->cached_board_json);
        board_data->cached_board_json = NULL;
    }
    if (board_data->cached_device_status_json) {
        free(board_data->cached_device_status_json);
        board_data->cached_device_status_json = NULL;
    }
    
    LINX_LOGI(TAG, "Board cache invalidated");
}

bool board_save_settings(Board* self) {
    Settings* settings = board_get_settings(self);
    if (!settings) {
        return false;
    }
    
    return settings_save(settings);
}

bool board_reset_settings(Board* self) {
    Settings* settings = board_get_settings(self);
    if (!settings) {
        return false;
    }
    
    bool result = settings_clear(settings);
    if (result) {
        LINX_LOGI(TAG, "Board settings reset to defaults");
    }
    
    return result;
}