cmake_minimum_required(VERSION 3.10)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/PlatformDetection.cmake)
# Detect platform
detect_linx_platform()

# 播放器库通用源文件
set(PLAY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/linx_player.c
)

set(PLAY_HEADERS
    linx_player.h
)

# 平台特定的播放器实现
if(LINX_TARGET_PLATFORM STREQUAL "macos")
    # macOS 播放器实现
    # 当前使用通用实现，未来可以添加平台特定优化
    # list(APPEND PLAY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/macos_player.c)
    # list(APPEND PLAY_HEADERS macos_player.h)
    set(PLAY_PLATFORM_LIBS "")
    
elseif(LINX_TARGET_PLATFORM STREQUAL "esp32")
    # ESP32 播放器实现
    # TODO: 添加 ESP32 播放器实现
    # list(APPEND PLAY_SOURCES esp32_player.c)
    # list(APPEND PLAY_HEADERS esp32_player.h)
    # set(PLAY_PLATFORM_LIBS "")
    message(WARNING "ESP32 player implementation not yet available")
    
elseif(LINX_TARGET_PLATFORM STREQUAL "windows")
    # Windows 播放器实现
    # TODO: 添加 Windows 播放器实现
    # list(APPEND PLAY_SOURCES windows_player.c)
    # list(APPEND PLAY_HEADERS windows_player.h)
    # set(PLAY_PLATFORM_LIBS "")
    message(WARNING "Windows player implementation not yet available")
    
elseif(LINX_TARGET_PLATFORM STREQUAL "android")
    # Android 播放器实现
    # TODO: 添加 Android 播放器实现
    # list(APPEND PLAY_SOURCES android_player.c)
    # list(APPEND PLAY_HEADERS android_player.h)
    # set(PLAY_PLATFORM_LIBS "")
    message(WARNING "Android player implementation not yet available")
    
else()
    # 未知平台 - 使用通用实现
    message(WARNING "Unknown platform: ${LINX_TARGET_PLATFORM}, using generic player implementation")
    set(PLAY_PLATFORM_LIBS "")
endif()

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将播放器模块的源文件列表设置为父作用域变量
set(LINX_PLAY_SOURCES ${PLAY_SOURCES} PARENT_SCOPE)

# 将播放器模块的头文件列表设置为父作用域变量  
set(LINX_PLAY_HEADERS ${PLAY_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_PLAY_PLATFORM_LIBS ${PLAY_PLATFORM_LIBS} PARENT_SCOPE)