set(MONGOOSE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/third/mongoose")

idf_component_register(
    SRCS "${MONGOOSE_ROOT}/mongoose.c"
    INCLUDE_DIRS "${MONGOOSE_ROOT}"
    REQUIRES 
        lwip
        esp_wifi
        esp_netif
        esp_event
        esp_system
        esp_https_ota
        esp_http_client
        nvs_flash
        freertos
        app_update
        bootloader_support
        esp_app_format
        esp_timer
        driver
)