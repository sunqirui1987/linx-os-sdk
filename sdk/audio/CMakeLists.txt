cmake_minimum_required(VERSION 3.10)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/PlatformDetection.cmake)
# Detect platform
detect_linx_platform()

# 音频库通用源文件
set(AUDIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_interface.c
)

set(AUDIO_HEADERS
    audio_interface.h
)

# 平台特定的音频实现
if(LINX_TARGET_PLATFORM STREQUAL "macos")
    # PortAudio 实现 (macOS)
    list(APPEND AUDIO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/portaudio_mac.c)
    list(APPEND AUDIO_HEADERS portaudio_mac.h)
    set(AUDIO_PLATFORM_LIBS portaudio)
    set(AUDIO_INCLUDE_DIRS 
        /opt/homebrew/opt/portaudio/include
    )
    set(AUDIO_LIBRARY_DIRS
        /opt/homebrew/opt/portaudio/lib
    )
    
elseif(LINX_TARGET_PLATFORM STREQUAL "esp32")
    # ESP32 I2S 实现
    # TODO: 添加 ESP32 I2S 音频驱动实现
    # list(APPEND AUDIO_SOURCES esp32_i2s.c)
    # list(APPEND AUDIO_HEADERS esp32_i2s.h)
    # set(AUDIO_PLATFORM_LIBS idf::driver)
    message(WARNING "ESP32 audio implementation not yet available")
    
elseif(LINX_TARGET_PLATFORM STREQUAL "windows")
    # Windows WASAPI 实现
    # TODO: 添加 Windows WASAPI 音频驱动实现
    # list(APPEND AUDIO_SOURCES wasapi_windows.c)
    # list(APPEND AUDIO_HEADERS wasapi_windows.h)
    # set(AUDIO_PLATFORM_LIBS winmm)
    message(WARNING "Windows audio implementation not yet available")
    
elseif(LINX_TARGET_PLATFORM STREQUAL "android")
    # Android OpenSL ES 实现
    # TODO: 添加 Android OpenSL ES 音频驱动实现
    # list(APPEND AUDIO_SOURCES opensles_android.c)
    # list(APPEND AUDIO_HEADERS opensles_android.h)
    # set(AUDIO_PLATFORM_LIBS OpenSLES)
    message(WARNING "Android audio implementation not yet available")
    
else()
    # 未知平台 - 使用空实现
    message(WARNING "Unknown platform: ${LINX_TARGET_PLATFORM}, using stub audio implementation")
    list(APPEND AUDIO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/audio_stub.c)
    list(APPEND AUDIO_HEADERS audio_stub.h)
    set(AUDIO_PLATFORM_LIBS "")
endif()

# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将音频模块的源文件列表设置为父作用域变量
set(LINX_AUDIO_SOURCES ${AUDIO_SOURCES} PARENT_SCOPE)

# 将音频模块的头文件列表设置为父作用域变量  
set(LINX_AUDIO_HEADERS ${AUDIO_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_AUDIO_PLATFORM_LIBS ${AUDIO_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_AUDIO_INCLUDE_DIRS ${AUDIO_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_AUDIO_LIBRARY_DIRS ${AUDIO_LIBRARY_DIRS} PARENT_SCOPE)

