cmake_minimum_required(VERSION 3.10)
project(AudioProcessorMacTest)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# 检查是否为Mac平台
if(NOT APPLE)
    message(FATAL_ERROR "This CMakeLists.txt is designed for macOS only")
endif()

# 查找必要的库
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# 查找Mac系统框架
find_library(COREAUDIO_LIBRARY CoreAudio)
find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)

# 查找PortAudio库
find_path(PORTAUDIO_INCLUDE_DIR
    NAMES portaudio.h
    PATHS /usr/local/include /opt/homebrew/include
    DOC "PortAudio include directory")

find_library(PORTAUDIO_LIBRARY
    NAMES portaudio
    PATHS /usr/local/lib /opt/homebrew/lib
    DOC "PortAudio library")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/audio/processor
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/audio/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/common/log
    ${PORTAUDIO_INCLUDE_DIR}
)

# 源文件
set(TEST_SOURCES
    audio_processor_mac_realtime_test.c
)

set(MAC_PROCESSOR_SOURCES
    ../audio_processor_mac.c
    ../../audio/portaudio_mac.c
)

set(COMMON_SOURCES
    ../../../../../../src/audio/processor/audio_processor.c
    ../../../../../../src/common/log/linx_log.c
    ../../../../../../src/audio/audio/audio_interface.c
)

# 创建测试可执行文件

add_executable(audio_processor_mac_realtime_test
    audio_processor_mac_realtime_test.c
    ${MAC_PROCESSOR_SOURCES}
    ${COMMON_SOURCES}
)

# 链接库


target_link_libraries(audio_processor_mac_realtime_test
    ${COREAUDIO_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    ${AUDIOUNIT_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    ${PORTAUDIO_LIBRARY}
    Threads::Threads
    m
)

# 设置包含目录属性


target_include_directories(audio_processor_mac_realtime_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/audio/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/audio/processor
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/audio/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/common/log
    ${PORTAUDIO_INCLUDE_DIR}
)

# 添加自定义目标用于清理
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/Makefile
)

# 打印构建信息
message(STATUS "")
message(STATUS "Audio Processor Mac Test Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  PortAudio include: ${PORTAUDIO_INCLUDE_DIR}")
message(STATUS "  PortAudio library: ${PORTAUDIO_LIBRARY}")
message(STATUS "")