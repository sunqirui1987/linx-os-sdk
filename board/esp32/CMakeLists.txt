cmake_minimum_required(VERSION 3.10)
project(linx_esp32_board)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/board
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/display
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/common/log
)

# Collect source files
set(ESP32_BOARD_SOURCES
    esp32_board.c
    esp32_system_info.c
    ../../src/board/board.c
    ../../src/common/settings.c
    ../../src/common/system_info.c
    ../../src/common/log/linx_log.c
    ../../src/common/cjson/cJSON.c
    ../../src/display/display.c
    ../../src/display/display_stub.c
    ../../src/camera/camera_interface.c
    ../../src/camera/camera_stub.c
)

# Create ESP32 board library
add_library(esp32_board STATIC ${ESP32_BOARD_SOURCES})

# Add compiler flags
target_compile_options(esp32_board PRIVATE
    -Wall
    -Wextra
    -std=c99
)

# Link libraries
target_link_libraries(esp32_board
    m  # Math library
)

# Create test executable
add_executable(esp32_board_test
    test_esp32_board.c
    ${ESP32_BOARD_SOURCES}
)

# Link libraries for test
target_link_libraries(esp32_board_test
    m  # Math library
)