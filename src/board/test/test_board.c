#include "../board.h"
#include "../../common/log/linx_log.h"
#include "../../common/settings.h"
#include "../../common/system_info.h"
#include "../../display/display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "BoardTest"

// Simple board implementation for testing
static const char* test_board_get_board_type(Board* self) {
    return "TestBoard";
}

static void* test_board_get_audio_codec(Board* self) {
    return NULL; // No audio codec for test
}

static void* test_board_get_network(Board* self) {
    return NULL; // No network for test
}

static void test_board_start_network(Board* self) {
    LINX_LOGI(TAG, "Test network started");
}

static const char* test_board_get_network_state_icon(Board* self) {
    return "📶"; // Test network icon
}

// Forward declarations of default functions (we'll use NULL for most)
static void test_board_set_power_save_mode(Board* self, bool enabled) {
    LINX_LOGI(TAG, "Test board power save mode: %s", enabled ? "ON" : "OFF");
}

static const char* test_board_get_board_json(Board* self) {
    return "{\"type\":\"TestBoard\",\"version\":\"1.0\"}";
}

static const char* test_board_get_device_status_json(Board* self) {
    return "{\"status\":\"running\",\"uptime\":123}";
}

// Test board vtable
static const BoardVTable test_board_vtable = {
    .get_board_type = test_board_get_board_type,
    .get_audio_codec = test_board_get_audio_codec,
    .get_network = test_board_get_network,
    .start_network = test_board_start_network,
    .get_network_state_icon = test_board_get_network_state_icon,
    .set_power_save_mode = test_board_set_power_save_mode,
    .get_board_json = test_board_get_board_json,
    .get_device_status_json = test_board_get_device_status_json,
    .get_uuid = NULL,  // Use default implementation
    .get_backlight = NULL,
    .get_led = NULL,
    .get_temperature = NULL,
    .get_display = NULL,  // Use default implementation
    .get_camera = NULL,
    .get_battery_level = NULL,
    .get_system_info_json = NULL,  // Use default implementation
    .destroy = NULL  // Use default implementation
};

// Default system info creation function for testing
SystemInfo* create_system_info(void) {
    // Return NULL to use the default system_info implementation
    // This allows the system_info module to fall back to its default behavior
    return NULL;
}

// Test board creation function
Board* create_board(void) {
    Board* board = board_create();
    if (board) {
        board->vtable = &test_board_vtable;
    }
    return board;
}

int main() {
    printf("=== Board Module Test ===\n");
    
    // Create board instance
    Board* board = create_board();
    if (!board) {
        printf("❌ Failed to create board\n");
        return 1;
    }
    printf("✅ Board created successfully\n");
    
    // Test board type
    const char* board_type = board_get_board_type(board);
    printf("📋 Board type: %s\n", board_type ? board_type : "Unknown");
    
    // Test UUID
    const char* uuid = board_get_uuid(board);
    printf("🆔 Board UUID: %s\n", uuid ? uuid : "None");
    
    // Test settings
    Settings* settings = board_get_settings(board);
    if (settings) {
        printf("✅ Settings module available\n");
        
        // Test setting and getting values
        settings_set_string(settings, "test_key", "test_value");
        const char* value = settings_get_string(settings, "test_key");
        printf("🔧 Settings test: %s\n", value ? value : "NULL");
        
        settings_set_int(settings, "test_int", 42);
        int int_value = settings_get_int(settings, "test_int", 0);
        printf("🔢 Settings int test: %d\n", int_value);
    } else {
        printf("❌ Settings module not available\n");
    }
    
    // Test display
    void* display = board_get_display(board);
    if (display) {
        printf("✅ Display interface available\n");
    } else {
        printf("❌ Display interface not available\n");
    }
    
    // Test power save mode
    printf("🔋 Current power save mode: %s\n", board_get_power_save_mode(board) ? "ON" : "OFF");
    board_set_power_save_mode(board, true);
    printf("🔋 Power save mode after setting to ON: %s\n", board_get_power_save_mode(board) ? "ON" : "OFF");
    
    // Test system info JSON
    const char* system_info = board_get_system_info_json(board);
    if (system_info) {
        printf("📊 System info JSON (first 100 chars): %.100s...\n", system_info);
    } else {
        printf("❌ System info JSON not available\n");
    }
    
    // Test board JSON
    const char* board_json = board_get_board_json(board);
    if (board_json) {
        printf("📋 Board JSON: %s\n", board_json);
    } else {
        printf("❌ Board JSON not available\n");
    }
    
    // Test device status JSON
    const char* device_status = board_get_device_status_json(board);
    if (device_status) {
        printf("📱 Device status JSON: %s\n", device_status);
    } else {
        printf("❌ Device status JSON not available\n");
    }
    
    // Test cache invalidation
    board_invalidate_cache(board);
    printf("🗑️  Cache invalidated\n");
    
    // Test settings save
    if (board_save_settings(board)) {
        printf("💾 Settings saved successfully\n");
    } else {
        printf("❌ Failed to save settings\n");
    }
    
    // Cleanup
    board_destroy(board);
    printf("🧹 Board destroyed\n");
    
    printf("=== Test completed ===\n");
    return 0;
}