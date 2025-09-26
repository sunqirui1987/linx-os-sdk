cmake_minimum_required(VERSION 3.10)
project(linx_test_play C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)



# Include find modules first
list(APPEND CMAKE_MODULE_PATH ${SDK_DIR}/cmake)

# Add the play subdirectory with explicit binary directory
add_subdirectory(${SDK_DIR}/play ${CMAKE_CURRENT_BINARY_DIR}/play)
# Add other required subdirectories
add_subdirectory(${SDK_DIR}/audio ${CMAKE_CURRENT_BINARY_DIR}/audio)
add_subdirectory(${SDK_DIR}/codecs ${CMAKE_CURRENT_BINARY_DIR}/codecs)
message(STATUS "Building test play ${SDK_DIR}")

# Test executable
set(TEST_SOURCES
    play_audio_test.c
    ${CMAKE_CURRENT_LIST_DIR}/../../log/linx_log.c
    ${LINX_PLAY_SOURCES}
    ${LINX_AUDIO_SOURCES}
    ${LINX_CODEC_SOURCES}
)

# Create test executable
add_executable(play_audio_test ${TEST_SOURCES})

# Include directories
target_include_directories(play_audio_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../../
    ${CMAKE_CURRENT_LIST_DIR}/../../audio
    ${CMAKE_CURRENT_LIST_DIR}/../../codecs
    ${CMAKE_CURRENT_LIST_DIR}/../../log
    ${CMAKE_CURRENT_LIST_DIR}/../../third/opus/include
    ${LINX_AUDIO_INCLUDE_DIRS}
    ${LINX_CODEC_INCLUDE_DIRS}
)


# macOS 系统框架
if(APPLE)
    # 查找 PortAudio
    find_library(PORTAUDIO_LIBRARY portaudio)
    if(PORTAUDIO_LIBRARY)
        message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
    else()
        message(WARNING "PortAudio library not found, you may need to install it")
    endif()

    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
endif()

# Link with required dependencies
target_link_libraries(play_audio_test
    ${LINX_AUDIO_PLATFORM_LIBS}
    ${LINX_CODEC_PLATFORM_LIBS}
    ${LINX_PLAY_PLATFORM_LIBS}
    ${PORTAUDIO_LIBRARY}
    ${SDK_DIR}/third/opus/install/lib/libopus.a
    pthread
    m  # Math library for sin/cos functions in test
)

# 链接 PortAudio 库
if(PORTAUDIO_LIBRARY)
    target_link_libraries(play_audio_test ${PORTAUDIO_LIBRARY})
endif()

# 链接 macOS 系统框架
if(APPLE)
    target_link_libraries(play_audio_test
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()

# Compiler flags
target_compile_features(play_audio_test PRIVATE c_std_99)
target_compile_options(play_audio_test PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Add definitions
target_compile_definitions(play_audio_test PRIVATE
    OPUS_BUILD
)

# Enable testing
enable_testing()

# Add test
add_test(NAME play_basic_test COMMAND play_audio_test --basic)

# Set test properties
set_tests_properties(play_basic_test PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Custom target to run tests
add_custom_target(run_play_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS play_audio_test
    COMMENT "Running play tests"
)