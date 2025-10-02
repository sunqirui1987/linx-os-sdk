cmake_minimum_required(VERSION 3.10)
project(linx_audio)

# Set project name
set(LINX_AUDIO_TARGET linx_audio)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs
    ${CMAKE_CURRENT_SOURCE_DIR}/play
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/opus/include
)

# Collect all source files from subdirectories
set(LINX_AUDIO_SOURCES
    # Audio interface sources
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_stub.c
    
    # Codec sources
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/audio_codec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/opus_codec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/codec_stub.c
    
    # Player sources
    ${CMAKE_CURRENT_SOURCE_DIR}/play/linx_player.c
)

# Create static library
add_library(${LINX_AUDIO_TARGET} STATIC ${LINX_AUDIO_SOURCES})

# Set target properties for static library
set_target_properties(${LINX_AUDIO_TARGET} PROPERTIES
    OUTPUT_NAME "linx_audio"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PREFIX "lib"
    SUFFIX ".a"
)

# Target include directories (for consumers of this library)
target_include_directories(${LINX_AUDIO_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs
    ${CMAKE_CURRENT_SOURCE_DIR}/play
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/opus/include
)

# Compiler flags
target_compile_options(${LINX_AUDIO_TARGET} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function
    -O2
)

# Link with common libraries if they exist
if(TARGET linx_common)
    target_link_libraries(${LINX_AUDIO_TARGET} PUBLIC linx_common)
endif()

# Link with opus library if available
if(TARGET opus)
    target_link_libraries(${LINX_AUDIO_TARGET} PUBLIC opus)
endif()

# Platform-specific linking
if(APPLE)
    # macOS specific libraries for audio
    target_link_libraries(${LINX_AUDIO_TARGET} PUBLIC
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework AudioUnit"
    )
elseif(UNIX)
    # Linux specific libraries
    target_link_libraries(${LINX_AUDIO_TARGET} PUBLIC
        pthread
        m
        asound
    )
endif()

# Install rules
install(TARGETS ${LINX_AUDIO_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files with proper directory structure
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_stub.h
    DESTINATION include/linx_audio/audio
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/audio_codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/opus_codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs/codec_stub.h
    DESTINATION include/linx_audio/codecs
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/play/linx_player.h
    DESTINATION include/linx_audio/play
)

# Print build information
message(STATUS "Building LINX Audio Library")
message(STATUS "  - Combined library: linx_audio (includes audio interface, codecs, and player)")