cmake_minimum_required(VERSION 3.10)
project(linx_audio_test C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Building LINX Audio Test Suite")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")

# Common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Platform-specific flags
if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

# Core audio module includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs
    ${CMAKE_CURRENT_SOURCE_DIR}/../processor
    ${CMAKE_CURRENT_SOURCE_DIR}/../wake_words
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# Platform-specific includes
if(APPLE)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/audio
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/processor
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/wake_words
    )
endif()

# Core audio library sources
set(AUDIO_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_packet_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_task_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../timestamp_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio/audio_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio/audio_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs/audio_codec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs/codec_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../processor/audio_processor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../wake_words/wake_word_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../wake_words/wake_word_stub.c
)

# Common library sources
set(COMMON_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log/linx_log.c
)

# Platform-specific sources
if(APPLE)
    list(APPEND AUDIO_LIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/audio/portaudio_mac.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/processor/audio_processor_mac.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/wake_words/wake_word_porcupine.c
    )
endif()

# Test sources
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_service_demo_mac.c
)

# Create static library for audio components
add_library(linx_audio_test_lib STATIC 
    ${AUDIO_LIB_SOURCES}
    ${COMMON_LIB_SOURCES}
)

# Set library properties
set_target_properties(linx_audio_test_lib PROPERTIES
    OUTPUT_NAME "linx_audio_test"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Library compile options
target_compile_options(linx_audio_test_lib PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-function
)

# AudioService Demo for Mac
add_executable(audio_service_demo_mac ${TEST_SOURCES})

# Link with the audio library
target_link_libraries(audio_service_demo_mac 
    linx_audio_test_lib
)

# Find required libraries first
if(APPLE)
    # Try to find and link PortAudio
    find_library(PORTAUDIO_LIBRARY portaudio)
    if(PORTAUDIO_LIBRARY)
        message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
    else()
        message(WARNING "PortAudio not found. Real audio functionality may not work.")
        message(STATUS "To install PortAudio: brew install portaudio")
    endif()
    
    # Try to find and link Porcupine (optional)
    # Look for Porcupine in common installation paths
    find_library(PORCUPINE_LIBRARY 
        NAMES pv_porcupine libpv_porcupine
        PATHS
            /usr/local/lib
            /opt/homebrew/lib
            ${CMAKE_SOURCE_DIR}/third_party/porcupine/lib
            $ENV{PORCUPINE_ROOT}/lib
        NO_DEFAULT_PATH
    )
    
    # If not found in specific paths, try system paths
    if(NOT PORCUPINE_LIBRARY)
        find_library(PORCUPINE_LIBRARY 
            NAMES pv_porcupine libpv_porcupine
        )
    endif()
    
    if(PORCUPINE_LIBRARY)
        message(STATUS "Found Porcupine: ${PORCUPINE_LIBRARY}")
        # Also look for Porcupine headers
        find_path(PORCUPINE_INCLUDE_DIR 
            NAMES pv_porcupine.h
            PATHS
                /usr/local/include
                /opt/homebrew/include
                ${CMAKE_SOURCE_DIR}/third_party/porcupine/include
                $ENV{PORCUPINE_ROOT}/include
        )
        if(PORCUPINE_INCLUDE_DIR)
            message(STATUS "Found Porcupine headers: ${PORCUPINE_INCLUDE_DIR}")
        endif()
    else()
        message(STATUS "Porcupine not found. Wake word functionality will use stub implementation.")
        message(STATUS "To install Porcupine:")
        message(STATUS "  1. Visit https://picovoice.ai/")
        message(STATUS "  2. Download and install Porcupine SDK")
        message(STATUS "  3. Set PICOVOICE_ACCESS_KEY environment variable")
        message(STATUS "  4. Or set PORCUPINE_ROOT environment variable to Porcupine installation path")
    endif()
endif()

# Platform-specific linking
if(APPLE)
    # macOS frameworks and libraries
    target_link_libraries(audio_service_demo_mac
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework Foundation"
    )
    
    # Link PortAudio if found
    if(PORTAUDIO_LIBRARY)
        target_link_libraries(audio_service_demo_mac ${PORTAUDIO_LIBRARY})
    endif()
    
    # Link Porcupine if found
    if(PORCUPINE_LIBRARY)
        target_link_libraries(audio_service_demo_mac ${PORCUPINE_LIBRARY})
        target_compile_definitions(audio_service_demo_mac PRIVATE HAVE_PORCUPINE=1)
        if(PORCUPINE_INCLUDE_DIR)
            target_include_directories(audio_service_demo_mac PRIVATE ${PORCUPINE_INCLUDE_DIR})
        endif()
    endif()
    
elseif(UNIX)
    # Linux libraries
    target_link_libraries(audio_service_demo_mac
        pthread
        m
        asound
    )
endif()

# Set executable properties
set_target_properties(audio_service_demo_mac PROPERTIES
    OUTPUT_NAME "audio_service_demo_mac"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Create a simple audio test
add_executable(audio_simple_test 
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_service_demo_mac.c
)

target_link_libraries(audio_simple_test linx_audio_test_lib)

# Simplified version without platform-specific dependencies
target_compile_definitions(audio_simple_test PRIVATE 
    SIMPLE_TEST_MODE=1
    NO_REAL_AUDIO=1
)

if(APPLE)
    # macOS frameworks and libraries
    target_link_libraries(audio_simple_test
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework Foundation"
    )
    
    # Link PortAudio if found
    if(PORTAUDIO_LIBRARY)
        target_link_libraries(audio_simple_test ${PORTAUDIO_LIBRARY})
    endif()
    
    # Link Porcupine if found
    if(PORCUPINE_LIBRARY)
        target_link_libraries(audio_simple_test ${PORCUPINE_LIBRARY})
        target_compile_definitions(audio_simple_test PRIVATE HAVE_PORCUPINE=1)
        if(PORCUPINE_INCLUDE_DIR)
            target_include_directories(audio_simple_test PRIVATE ${PORCUPINE_INCLUDE_DIR})
        endif()
    endif()
    
elseif(UNIX)
    target_link_libraries(audio_simple_test pthread m)
endif()

set_target_properties(audio_simple_test PROPERTIES
    OUTPUT_NAME "audio_simple_test"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install test executables
install(TARGETS audio_service_demo_mac audio_simple_test
    RUNTIME DESTINATION bin
)

# Install test documentation
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/README_DEMO.md
    DESTINATION share/doc/linx_audio_test
)

# Create a target to run the basic test
add_custom_target(run_basic_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_service_demo_mac --mode basic --duration 5
    DEPENDS audio_service_demo_mac
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running basic audio service test"
)

# Create a target to show test help
add_custom_target(test_help
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_service_demo_mac --help
    DEPENDS audio_service_demo_mac
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Showing audio service demo help"
)

message(STATUS "Audio Test Configuration:")
message(STATUS "  - Main demo: audio_service_demo_mac")
message(STATUS "  - Simple test: audio_simple_test")
message(STATUS "  - Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  - Library directory: ${CMAKE_BINARY_DIR}/lib")

if(APPLE)
    message(STATUS "macOS Build Configuration:")
    message(STATUS "  - PortAudio support: ${PORTAUDIO_LIBRARY}")
    message(STATUS "  - Porcupine support: ${PORCUPINE_LIBRARY}")
    message(STATUS "  - To install PortAudio: brew install portaudio")
    message(STATUS "  - To install Porcupine: visit https://picovoice.ai/")
endif()