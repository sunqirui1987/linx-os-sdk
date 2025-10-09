set(LINX_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src")

# Collect all source files from src directory (excluding third party)
file(GLOB_RECURSE LINX_SDK_SRCS
    "${LINX_SDK_ROOT}/board/*.c"
    "${LINX_SDK_ROOT}/audio/*.c"
    "${LINX_SDK_ROOT}/display/*.c"
    "${LINX_SDK_ROOT}/camera/*.c"
    "${LINX_SDK_ROOT}/common/*.c"
    "${LINX_SDK_ROOT}/linxsdk/*.c"
)

# Exclude test files
list(FILTER LINX_SDK_SRCS EXCLUDE REGEX ".*test.*\\.c$")

idf_component_register(
    SRCS ${LINX_SDK_SRCS}
    INCLUDE_DIRS 
        "${LINX_SDK_ROOT}"
        "${LINX_SDK_ROOT}/board"
        "${LINX_SDK_ROOT}/audio"
        "${LINX_SDK_ROOT}/display"
        "${LINX_SDK_ROOT}/camera"
        "${LINX_SDK_ROOT}/common"
        "${LINX_SDK_ROOT}/common/cjson"
        "${LINX_SDK_ROOT}/common/log"
        "${LINX_SDK_ROOT}/common/http"
        "${LINX_SDK_ROOT}/common/std"
        "${LINX_SDK_ROOT}/linxsdk"
        "${LINX_SDK_ROOT}/linxsdk/mcp"
        "${LINX_SDK_ROOT}/linxsdk/ota"
        "${LINX_SDK_ROOT}/linxsdk/protocols"
        "${LINX_SDK_ROOT}/third"
    REQUIRES 
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        driver
        freertos
        esp_timer
        esp_system
        esp_http_client
        esp_https_ota
        json
        mqtt
        mongoose
        opus
        fonts
        liblvgl
)