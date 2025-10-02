/**
 * @file ota_test.c
 * @brief Test program for OTA functionality
 */

#include "../linx_ota.h"
#include "../../log/linx_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG "OTA_TEST"

// Progress callback
static void progress_callback(int percentage) {
    printf("Download progress: %d%%\n", percentage);
}

int main(int argc, char *argv[]) {
    // 初始化日志系统
    log_config_t log_config = LOG_DEFAULT_CONFIG;
    log_config.level = LOG_LEVEL_DEBUG;  // 设置为DEBUG级别
    log_config.enable_timestamp = true;
    log_config.enable_color = true;
    if (log_init(&log_config) != 0) {
        LOG_ERROR("日志系统初始化失败");
        return 0;
    }
    
    printf("LinX OS SDK OTA Test\n");
    
    // OTA configuration - matching the JavaScript fetch request
    linx_ota_config_t config = {
        .ota_server_url = "http://xrobo.qiniuapi.com/v1/ota/",
        .device_id = "1A:2B:3C:4D:5E:6F",
        .client_id = "web_test_client",
        .user_agent = "\"Chromium\";v=\"140\", \"Not=A?Brand\";v=\"24\", \"Google Chrome\";v=\"140\"",
        .current_version = "1.0.0",
        .elf_sha256 = "1234567890abcdef1234567890abcdef1234567890abcdef",
        .board_type = "xiaoniu-web-test",
        .board_name = "xiaoniu-web-test",
        .ssid = "xiaoniu-web-test",
        .rssi = 0,
        .mac_address = "1A:2B:3C:4D:5E:6F",
        .chip_model = "",
        
        // Additional fields to match JavaScript request
        .app_name = "xiaoniu-web-test",
        .compile_time = "2025-04-16 10:00:00",
        .idf_version = "4.4.3",
        .ota_label = "xiaoniu-web-test",
        .wifi_channel = 0,
        .ip_address = "192.168.1.1",
        .flash_size = 0,
        .minimum_free_heap_size = 0,
        
        // Chip info
        .chip_model_id = 0,
        .chip_cores = 0,
        .chip_revision = 0,
        .chip_features = 0,
        
        .progress_cb = progress_callback
    };
    
    // Initialize OTA module
    linx_ota_status_t status = linx_ota_init(&config);
    if (status != LINX_OTA_SUCCESS) {
        printf("Failed to initialize OTA module: %s\n", linx_ota_status_str(status));
        return -1;
    }
    
    // Check for updates
    linx_ota_info_t info;
    status = linx_ota_check_update(&info);
    
    if (status == LINX_OTA_SUCCESS) {
        printf("[OTA] Update available\n");
        printf("[OTA] Activation code: %s\n", info.activation_code);
        printf("[OTA] WebSocket URL: %s\n", info.websocket_url);
        printf("[OTA] Firmware version: %s\n", info.firmware_version);
        printf("[OTA] Firmware URL: %s\n", info.firmware_url);
        
        // // Download firmware
        // const char *download_path = "/tmp/linx_firmware.bin";
        // status = linx_ota_download(&info, download_path);
        
        // if (status == LINX_OTA_SUCCESS) {
        //     printf("Firmware downloaded successfully\n");
            
        //     // Apply update
        //     status = linx_ota_apply(download_path);
        //     if (status == LINX_OTA_SUCCESS) {
        //         printf("Firmware update applied successfully\n");
        //     } else {
        //         printf("Failed to apply firmware update: %s\n", linx_ota_status_str(status));
        //     }
        // } else {
        //     printf("Failed to download firmware: %s\n", linx_ota_status_str(status));
        // }
    } else if (status == LINX_OTA_NO_UPDATE) {
        printf("No update available\n");
    } else {
        printf("Failed to check for updates: %s\n", linx_ota_status_str(status));
    }
    
    // Cleanup
    linx_ota_cleanup();
    
    return 0;
}