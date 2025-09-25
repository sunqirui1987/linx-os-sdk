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

