set(LVGL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/third/liblvgl/v9")

# Collect LVGL source files
file(GLOB_RECURSE LVGL_SRCS 
    "${LVGL_ROOT}/lvgl/src/*.c"
    "${LVGL_ROOT}/port/*.c"
)

# Exclude test files
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*test.*\\.c$")

idf_component_register(
    SRCS ${LVGL_SRCS}
    INCLUDE_DIRS 
        "${LVGL_ROOT}"
        "${LVGL_ROOT}/lvgl"
        "${LVGL_ROOT}/port"
        "${LVGL_ROOT}/conf"
        "${LVGL_ROOT}/lvgl/src"
    REQUIRES driver freertos
)