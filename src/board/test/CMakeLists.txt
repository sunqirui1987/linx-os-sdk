cmake_minimum_required(VERSION 3.10)
project(linx_board_test)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../display
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third
)

# Collect source files
set(BOARD_SOURCES
    ../board.c
    ../../common/settings.c
    ../../common/system_info.c
    ../../common/log/linx_log.c
    ../../common/cjson/cJSON.c
    ../../display/display.c
    ../../display/display_stub.c
)

# Create test executable
add_executable(board_test
    test_board.c
    ${BOARD_SOURCES}
)

# Link libraries
target_link_libraries(board_test
    m  # Math library
)

# Add compiler flags
target_compile_options(board_test PRIVATE
    -Wall
    -Wextra
    -std=c99
)