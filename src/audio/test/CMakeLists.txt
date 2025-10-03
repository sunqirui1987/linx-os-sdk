cmake_minimum_required(VERSION 3.10)
project(AudioServiceTest)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# 添加数学库
find_library(MATH_LIBRARY m)

# 查找pthread库
find_package(Threads REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
)

# 源文件列表
set(AUDIO_SERVICE_SOURCES
    ../audio_service.c
    ../audio_packet_queue.c
    ../audio_task_queue.c
    ../timestamp_queue.c
    ../audio/audio_interface.c
    ../audio/audio_stub.c
    ../codecs/audio_codec.c
    ../codecs/codec_stub.c
    # ../codecs/opus_codec.c  # 暂时排除，需要opus库
    ../processor/audio_processor.c
    ../wake_words/wake_word_interface.c
    ../wake_words/wake_word_stub.c
    ../../common/log/linx_log.c
)

# 测试源文件
set(FULL_TEST_SOURCES
    audio_service_test.c
)

set(SIMPLE_TEST_SOURCES
    simple_audio_test.c
)

set(MAC_TEST_SOURCES
    mac_audio_test.c
)

# 创建完整测试可执行文件
add_executable(audio_service_test 
    ${FULL_TEST_SOURCES}
    ${AUDIO_SERVICE_SOURCES}
)

# 创建简化测试可执行文件
add_executable(simple_audio_test 
    ${SIMPLE_TEST_SOURCES}
    ${AUDIO_SERVICE_SOURCES}
)

# 创建Mac测试可执行文件（仅在macOS上且PortAudio可用时）
if(APPLE)
    # 查找PortAudio
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PORTAUDIO_CHECK portaudio-2.0)
    endif()
    
    if(NOT PORTAUDIO_CHECK_FOUND)
         # 手动查找PortAudio，优先使用Homebrew版本
         find_path(PORTAUDIO_CHECK_INCLUDE_DIR portaudio.h
             PATHS /opt/homebrew /usr/local /opt/local /usr
             PATH_SUFFIXES include
             NO_DEFAULT_PATH
         )
         find_library(PORTAUDIO_CHECK_LIBRARY portaudio
             PATHS /opt/homebrew /usr/local /opt/local /usr
             PATH_SUFFIXES lib
             NO_DEFAULT_PATH
         )
         if(PORTAUDIO_CHECK_INCLUDE_DIR AND PORTAUDIO_CHECK_LIBRARY)
             set(PORTAUDIO_CHECK_FOUND TRUE)
             set(PORTAUDIO_CHECK_LIBRARIES ${PORTAUDIO_CHECK_LIBRARY})
             set(PORTAUDIO_CHECK_INCLUDE_DIRS ${PORTAUDIO_CHECK_INCLUDE_DIR})
             message(STATUS "Found PortAudio manually: ${PORTAUDIO_CHECK_LIBRARY}")
         endif()
     endif()
    
    if(PORTAUDIO_CHECK_FOUND)
        message(STATUS "PortAudio found, building Mac audio test")
        # Mac平台特定的源文件
        set(MAC_AUDIO_SOURCES
            ../../../board/mac/common/audio/audio/portaudio_mac.c
            ../../../board/mac/common/audio/processor/audio_processor_mac.c
        )
        
        add_executable(mac_audio_test 
            ${MAC_TEST_SOURCES}
            ${AUDIO_SERVICE_SOURCES}
            ${MAC_AUDIO_SOURCES}
        )
        set(MAC_AUDIO_TEST_BUILT TRUE)
    else()
        message(WARNING "PortAudio not found or incompatible, skipping Mac audio test")
        message(WARNING "To install PortAudio: brew install portaudio")
        set(MAC_AUDIO_TEST_BUILT FALSE)
    endif()
endif()

# 链接库
target_link_libraries(audio_service_test 
    Threads::Threads
    ${MATH_LIBRARY}
)

target_link_libraries(simple_audio_test 
    Threads::Threads
    ${MATH_LIBRARY}
)

# Mac测试的链接库（仅在Mac测试被构建时）
if(APPLE AND MAC_AUDIO_TEST_BUILT)
    target_link_libraries(mac_audio_test 
        Threads::Threads
        ${MATH_LIBRARY}
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework Accelerate"
    )
    
    if(PORTAUDIO_CHECK_FOUND)
        # 使用pkg-config的结果，确保使用正确的库路径
        if(PORTAUDIO_CHECK_LIBRARIES)
            target_link_libraries(mac_audio_test ${PORTAUDIO_CHECK_LIBRARIES})
            target_include_directories(mac_audio_test PRIVATE ${PORTAUDIO_CHECK_INCLUDE_DIRS})
            # 添加库路径以确保链接正确的版本
            if(PORTAUDIO_CHECK_LIBRARY_DIRS)
                target_link_directories(mac_audio_test PRIVATE ${PORTAUDIO_CHECK_LIBRARY_DIRS})
            endif()
        else()
            # 手动指定Homebrew的PortAudio路径
            target_link_libraries(mac_audio_test /opt/homebrew/lib/libportaudio.dylib)
            target_include_directories(mac_audio_test PRIVATE /opt/homebrew/include)
        endif()
    endif()
    
    # 添加Mac特定的包含目录
    target_include_directories(mac_audio_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio
    )
endif()

# 如果有opus库，可以添加
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(OPUS opus)
# if(OPUS_FOUND)
#     target_include_directories(audio_service_test PRIVATE ${OPUS_INCLUDE_DIRS})
#     target_link_libraries(audio_service_test ${OPUS_LIBRARIES})
#     target_compile_definitions(audio_service_test PRIVATE HAVE_OPUS=1)
# endif()

# 设置输出目录
set_target_properties(audio_service_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(simple_audio_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(APPLE AND MAC_AUDIO_TEST_BUILT)
    set_target_properties(mac_audio_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# 添加编译定义
target_compile_definitions(audio_service_test PRIVATE
    _GNU_SOURCE
    AUDIO_SERVICE_TEST=1
)

target_compile_definitions(simple_audio_test PRIVATE
    _GNU_SOURCE
    SIMPLE_AUDIO_TEST=1
)

if(APPLE AND MAC_AUDIO_TEST_BUILT)
    target_compile_definitions(mac_audio_test PRIVATE
        _GNU_SOURCE
        MAC_AUDIO_TEST=1
        __APPLE__
    )
endif()

# 如果是Debug模式，添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(audio_service_test PRIVATE DEBUG=1)
    target_compile_definitions(simple_audio_test PRIVATE DEBUG=1)
    if(APPLE AND MAC_AUDIO_TEST_BUILT)
        target_compile_definitions(mac_audio_test PRIVATE DEBUG=1)
    endif()
endif()

# 安装规则（可选）
if(APPLE AND MAC_AUDIO_TEST_BUILT)
    install(TARGETS audio_service_test simple_audio_test mac_audio_test
        RUNTIME DESTINATION bin
    )
else()
    install(TARGETS audio_service_test simple_audio_test
        RUNTIME DESTINATION bin
    )
endif()

# 添加测试（可选）
enable_testing()
add_test(NAME AudioServiceBasicTest COMMAND simple_audio_test)
add_test(NAME AudioServiceFullTest COMMAND audio_service_test)
if(APPLE AND MAC_AUDIO_TEST_BUILT)
    add_test(NAME MacAudioTest COMMAND mac_audio_test)
endif()