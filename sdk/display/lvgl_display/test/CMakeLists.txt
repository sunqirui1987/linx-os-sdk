cmake_minimum_required(VERSION 3.10)
project(lvgl_display_test C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags for better debugging and warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# 检查 macOS 上的 SDL2 安装
if(APPLE)
    message(STATUS "Checking SDL2 installation on macOS...")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        message(STATUS "Detected Apple Silicon (arm64) architecture")
        if(NOT EXISTS "/opt/homebrew/lib/libSDL2.dylib")
            message(WARNING "SDL2 not found for arm64 architecture!")
            message(WARNING "Please install SDL2 using: brew install sdl2")
            message(WARNING "Make sure you're using Homebrew for Apple Silicon (/opt/homebrew)")
        endif()
    endif()
endif()

# 查找必要的库
find_package(Threads REQUIRED)

# 针对 Apple Silicon Mac 的 SDL2 配置
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    # 优先使用 Homebrew 安装的 SDL2
    set(SDL2_ROOT "/opt/homebrew")
    find_package(SDL2 QUIET)
    
    if(NOT SDL2_FOUND)
        # 如果没找到，尝试使用 pkg-config
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
        
        # 过滤掉 x86_64 架构的库
        set(SDL2_FILTERED_LIBRARIES "")
        foreach(lib ${SDL2_LIBRARIES})
            if(NOT lib MATCHES "x86_64")
                list(APPEND SDL2_FILTERED_LIBRARIES ${lib})
            endif()
        endforeach()
        set(SDL2_LIBRARIES ${SDL2_FILTERED_LIBRARIES})
    endif()
else()
    # 其他平台使用标准方式
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# 设置 LVGL 相关路径
set(LVGL_DIR ${CMAKE_SOURCE_DIR}/../../../third/liblvgl/v9)
set(FONTS_DIR ${CMAKE_SOURCE_DIR}/../../../third/fonts)
set(DISPLAY_DIR ${CMAKE_SOURCE_DIR}/..)
set(LOG_DIR ${CMAKE_SOURCE_DIR}/../../../log)

# 添加 LVGL 子目录（如果存在）
if(EXISTS ${LVGL_DIR}/CMakeLists.txt)
    add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl)
    set(LVGL_TARGET v9)
else()
    message(WARNING "LVGL directory not found at ${LVGL_DIR}")
    set(LVGL_TARGET "")
endif()

# 添加 fonts 子目录（如果存在）
if(EXISTS ${FONTS_DIR}/CMakeLists.txt)
    add_subdirectory(${FONTS_DIR} ${CMAKE_BINARY_DIR}/fonts)
    set(FONTS_TARGET fonts)
else()
    message(WARNING "Fonts directory not found at ${FONTS_DIR}")
    set(FONTS_TARGET "")
endif()

# 检查 log 模块
if(EXISTS ${LOG_DIR}/linx_log.c)
    set(LOG_SOURCES ${LOG_DIR}/linx_log.c)
    set(LOG_INCLUDE_DIRS ${LOG_DIR})
else()
    message(WARNING "Log module not found at ${LOG_DIR}")
    set(LOG_SOURCES "")
    set(LOG_INCLUDE_DIRS "")
endif()

## Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../../third/liblvgl/v9
    ${CMAKE_SOURCE_DIR}/../../../third/liblvgl/v9/lvgl

    ${CMAKE_SOURCE_DIR}/../../../third/fonts/include
    ${CMAKE_SOURCE_DIR}/..
    ${LOG_INCLUDE_DIRS}
)

# Source files
set(DEMO_SOURCES
    lvgl_display_demo.c
)

set(LVGL_DISPLAY_SOURCES
    ../lvgl_display.c
    ../lvgl_image.c
    ../lvgl_theme.c
    ../lvgl_font.c
    ../emoji_collection.c
)

set(DISPLAY_BASE_SOURCES
    ../../display.c
)

# Create executable
add_executable(lvgl_display_demo 
    ${DEMO_SOURCES}
    ${LVGL_DISPLAY_SOURCES}
    ${DISPLAY_BASE_SOURCES}
    ${LOG_SOURCES}
)

# Link libraries
target_link_libraries(lvgl_display_demo
    Threads::Threads
    m
)

# 根据 SDL2 查找方式链接库
if(TARGET SDL2::SDL2)
    target_link_libraries(lvgl_display_demo SDL2::SDL2)
    message(STATUS "Using SDL2::SDL2 target")
else()
    target_link_libraries(lvgl_display_demo ${SDL2_LIBRARIES})
    target_include_directories(lvgl_display_demo PRIVATE ${SDL2_INCLUDE_DIRS})
    target_compile_options(lvgl_display_demo PRIVATE ${SDL2_CFLAGS_OTHER})
    message(STATUS "Using SDL2 libraries: ${SDL2_LIBRARIES}")
endif()

# Link LVGL if available
if(LVGL_TARGET)
    target_link_libraries(lvgl_display_demo ${LVGL_TARGET})
endif()

# Link fonts if available
if(FONTS_TARGET)
    target_link_libraries(lvgl_display_demo ${FONTS_TARGET})
endif()

# Set output directory
set_target_properties(lvgl_display_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add compile definitions
target_compile_definitions(lvgl_display_demo PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    LV_USE_LOG=1
    LV_LOG_LEVEL=LV_LOG_LEVEL_INFO
    LV_USE_SDL=1
)

# Create a custom target to run the demo
add_custom_target(run_lvgl_display_demo
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/lvgl_display_demo
    DEPENDS lvgl_display_demo
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running LVGL Display Demo"
)

# Install target
install(TARGETS lvgl_display_demo
    RUNTIME DESTINATION bin
)

# Print build information
message(STATUS "=== LVGL Display Demo Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "==============================================")