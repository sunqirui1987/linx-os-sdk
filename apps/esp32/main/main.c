#include "application.h"
#include "../../src/common/linx_log.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>

#define TAG "ESP32Main"

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    LINX_LOG_INFO(TAG, "ESP32 Bread Compact WiFi Board Starting...");
    LINX_LOG_INFO(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    
    // Create and start application
    ESP32Application* app = esp32_app_create();
    if (!app) {
        LINX_LOG_ERROR(TAG, "Failed to create application");
        esp_restart();
        return;
    }
    
    if (!esp32_app_start(app)) {
        LINX_LOG_ERROR(TAG, "Failed to start application");
        esp32_app_destroy(app);
        esp_restart();
        return;
    }
    
    LINX_LOG_INFO(TAG, "Application started successfully");
    
    // Main application loop runs in separate tasks
    // This task can be used for monitoring or other purposes
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        
        // Print system status every 10 seconds
        LINX_LOG_INFO(TAG, "System status - Free heap: %d bytes, State: %d", 
                     esp_get_free_heap_size(), esp32_app_get_device_state(app));
    }
}