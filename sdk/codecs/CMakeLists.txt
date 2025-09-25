cmake_minimum_required(VERSION 3.10)

# 获取当前CMakeLists.txt文件所在目录
set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Current directory: ${CMAKE_CURRENT_LIST_DIR}")

# 或者直接使用
message(STATUS "Current directory: ${CMAKE_CURRENT_LIST_DIR}")

# 包含平台检测模块
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/PlatformDetection.cmake)
# Detect platform
detect_linx_platform()


# 编解码器库基础源文件
set(CODEC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_codec.c
)

set(CODEC_HEADERS
    audio_codec.h
)
message(STATUS "LINX_TARGET_PLATFORM: ${LINX_TARGET_PLATFORM}")

# macOS/Linux - 使用软编码 Opus
find_package(Opus REQUIRED)

if(Opus_FOUND)
    list(APPEND CODEC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/opus_codec.c)
    list(APPEND CODEC_HEADERS opus_codec.h)
    list(APPEND CODEC_PLATFORM_LIBS opus)
    list(APPEND CODEC_LIBRARY_DIRS ${Opus_LIBRARIES})
    list(APPEND CODEC_INCLUDE_DIRS ${Opus_INCLUDE_DIRS})
    message(STATUS "Using Opus software codec for ${LINX_TARGET_PLATFORM}")
    message(STATUS "Opus libraries: ${Opus_LIBRARIES}")
    message(STATUS "Opus include dirs: ${Opus_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Opus library not found")
endif()

list(APPEND CODEC_SOURCES ${CMAKE_CURRENT_LIST_DIR}/codec_stub.c)
list(APPEND CODEC_HEADERS codec_stub.h)





# ========================================
# 对外暴露变量 - 供父级 CMakeLists.txt 使用
# ========================================

# 将编解码器模块的源文件列表设置为父作用域变量
set(LINX_CODEC_SOURCES ${CODEC_SOURCES} PARENT_SCOPE)

# 将编解码器模块的头文件列表设置为父作用域变量  
set(LINX_CODEC_HEADERS ${CODEC_HEADERS} PARENT_SCOPE)

# 将平台特定的库依赖设置为父作用域变量
set(LINX_CODEC_PLATFORM_LIBS ${CODEC_PLATFORM_LIBS} PARENT_SCOPE)

# 将包含目录设置为父作用域变量
set(LINX_CODEC_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR} ${CODEC_INCLUDE_DIRS} PARENT_SCOPE)

# 将库目录设置为父作用域变量
set(LINX_CODEC_LIBRARY_DIRS ${CODEC_LIBRARY_DIRS} PARENT_SCOPE)

