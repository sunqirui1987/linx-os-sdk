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

# =============================================================================
# Compiler flags
# =============================================================================

# Common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Platform-specific flags
if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

# =============================================================================
# Include directories
# =============================================================================

# Core audio module includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..                    # audio module root
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio              # audio interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs             # audio codecs
    ${CMAKE_CURRENT_SOURCE_DIR}/../processor          # audio processor
    ${CMAKE_CURRENT_SOURCE_DIR}/../wake_words         # wake word interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log      # logging
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common          # common utilities
)

# Platform-specific includes
if(APPLE)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/audio      # PortAudio Mac
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/processor  # Audio Processor Mac
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../board/mac/common/audio/wake_words # Wake Words Mac
    )
endif()

# Third-party includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third/opus/include  # Opus codec
)

# =============================================================================
# Source files
# =============================================================================

# Core audio library sources (needed for linking)
set(AUDIO_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_packet_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio_task_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../timestamp_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../audio/audio_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs/audio_codec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../codecs/codec_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../processor/audio_processor_stub.c
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

# =============================================================================
# Create audio library target
# =============================================================================

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

# =============================================================================
# Create test executables
# =============================================================================

# AudioService Demo for Mac
add_executable(audio_service_demo_mac ${TEST_SOURCES})

# Link with the audio library
target_link_libraries(audio_service_demo_mac 
    linx_audio_test_lib
)

# Platform-specific linking
if(APPLE)
    # macOS frameworks and libraries
    target_link_libraries(audio_service_demo_mac
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework Foundation"
    )
    
    # Try to find and link PortAudio
    find_library(PORTAUDIO_LIBRARY portaudio)
    if(PORTAUDIO_LIBRARY)
        target_link_libraries(audio_service_demo_mac ${PORTAUDIO_LIBRARY})
        message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
    else()
        message(WARNING "PortAudio not found. Real audio functionality may not work.")
    endif()
    
    # Try to find and link Porcupine (optional)
    find_library(PORCUPINE_LIBRARY pv_porcupine)
    if(PORCUPINE_LIBRARY)
        target_link_libraries(audio_service_demo_mac ${PORCUPINE_LIBRARY})
        target_compile_definitions(audio_service_demo_mac PRIVATE HAVE_PORCUPINE=1)
        message(STATUS "Found Porcupine: ${PORCUPINE_LIBRARY}")
    else()
        message(STATUS "Porcupine not found. Wake word functionality will use stub implementation.")
    endif()
    
elseif(UNIX)
    # Linux libraries
    target_link_libraries(audio_service_demo_mac
        pthread
        m
        asound  # ALSA
    )
endif()

# Set executable properties
set_target_properties(audio_service_demo_mac PROPERTIES
    OUTPUT_NAME "audio_service_demo_mac"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# =============================================================================
# Additional test targets (optional)
# =============================================================================

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
    target_link_libraries(audio_simple_test
        "-framework AudioToolbox"
        "-framework CoreAudio"
    )
elseif(UNIX)
    target_link_libraries(audio_simple_test pthread m)
endif()

set_target_properties(audio_simple_test PROPERTIES
    OUTPUT_NAME "audio_simple_test"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# =============================================================================
# Install rules
# =============================================================================

# Install test executables
install(TARGETS audio_service_demo_mac audio_simple_test
    RUNTIME DESTINATION bin
)

# Install test documentation
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/README_DEMO.md
    DESTINATION share/doc/linx_audio_test
)

# =============================================================================
# Custom targets and commands
# =============================================================================

# Create a target to run the basic test
add_custom_target(run_basic_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_service_demo_mac --mode basic --duration 5
    DEPENDS audio_service_demo_mac
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running basic audio service test"
)

# Create a target to run the simple test
add_custom_target(run_simple_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_simple_test --help
    DEPENDS audio_simple_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running simple audio test"
)

# Create a target to show test help
add_custom_target(test_help
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_service_demo_mac --help
    DEPENDS audio_service_demo_mac
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Showing audio service demo help"
)

# =============================================================================
# Build information
# =============================================================================

message(STATUS "Audio Test Configuration:")
message(STATUS "  - Main demo: audio_service_demo_mac")
message(STATUS "  - Simple test: audio_simple_test")
message(STATUS "  - Output directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  - Library directory: ${CMAKE_BINARY_DIR}/lib")

# Print available custom targets
message(STATUS "Available custom targets:")
message(STATUS "  - make run_basic_test    : Run basic audio service test")
message(STATUS "  - make run_simple_test   : Run simple audio test")
message(STATUS "  - make test_help         : Show demo help")

# =============================================================================
# Platform-specific messages
# =============================================================================

if(APPLE)
    message(STATUS "macOS Build Configuration:")
    message(STATUS "  - PortAudio support: ${PORTAUDIO_LIBRARY}")
    message(STATUS "  - Porcupine support: ${PORCUPINE_LIBRARY}")
    message(STATUS "  - To install PortAudio: brew install portaudio")
    message(STATUS "  - To install Porcupine: visit https://picovoice.ai/")
endif()