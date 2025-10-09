set(BOARD_ESP32_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../board/esp32")

# Collect ESP32 board source files
file(GLOB BOARD_ESP32_SRCS
    "${BOARD_ESP32_ROOT}/*.c"
)

# Exclude test files
list(FILTER BOARD_ESP32_SRCS EXCLUDE REGEX ".*test.*\\.c$")

idf_component_register(
    SRCS ${BOARD_ESP32_SRCS}
    INCLUDE_DIRS 
        "${BOARD_ESP32_ROOT}"
    REQUIRES 
        driver
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        freertos
        esp_timer
        esp_system
        linx_sdk
)