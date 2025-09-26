cmake_minimum_required(VERSION 3.10)
project(linx_sdk C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include platform detection (auto-detects platform)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformDetection.cmake)

# Include find modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)



# =============================================================================
# Add subdirectories to build individual libraries
# =============================================================================

# Add subdirectories in dependency order
add_subdirectory(log)
add_subdirectory(cjson)
add_subdirectory(mcp)
add_subdirectory(protocols)
add_subdirectory(codecs)
add_subdirectory(audio)
add_subdirectory(play)

# =============================================================================
# Create unified static library
# =============================================================================

# Collect all module sources into a single list
set(LINX_SDK_SOURCES
    ${LINX_LOG_SOURCES}
    ${LINX_CJSON_SOURCES}
    ${LINX_MCP_SOURCES}
    ${LINX_PROTOCOLS_SOURCES}
    ${LINX_CODEC_SOURCES}
    ${LINX_AUDIO_SOURCES}
    ${LINX_PLAY_SOURCES}
    linx_sdk.c
)

# Collect all include directories
set(LINX_SDK_DIRS
    ${LINX_AUDIO_INCLUDE_DIRS}
    ${LINX_CODEC_INCLUDE_DIRS}
    ${LINX_LOG_INCLUDE_DIRS}
    ${LINX_MCP_INCLUDE_DIRS}
    ${LINX_PROTOCOLS_INCLUDE_DIRS}
    ${LINX_CJSON_INCLUDE_DIRS}
    ${LINX_PLAY_INCLUDE_DIRS}
)

# Collect all platform-specific libraries
set(LINX_SDK_PLATFORM_LIBS
    ${LINX_AUDIO_PLATFORM_LIBS}
    ${LINX_CODEC_PLATFORM_LIBS}
    ${LINX_PROTOCOLS_PLATFORM_LIBS}
)

message(STATUS "LINX SDK SOURCES: ${LINX_SDK_SOURCES}")
message(STATUS "LINX SDK DIRS: ${LINX_SDK_DIRS}")
message(STATUS "LINX SDK PLATFORM LIBS: ${LINX_SDK_PLATFORM_LIBS}")

# Create the unified static library
add_library(linx_sdk_static STATIC ${LINX_SDK_SOURCES})

# =============================================================================
# Configure target properties
# =============================================================================

# Set public include directories for consumers of this library
target_include_directories(linx_sdk_static PUBLIC ${LINX_SDK_DIRS})

# Link required platform-specific libraries
target_link_libraries(linx_sdk_static PRIVATE ${LINX_SDK_PLATFORM_LIBS})




# =============================================================================
# Installation configuration
# =============================================================================

# Set installation directory to project root/out/linx
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../out/linx CACHE PATH "Installation directory" FORCE)
message(STATUS "SDK installation directory: ${CMAKE_INSTALL_PREFIX}")

# Install the static library
install(TARGETS linx_sdk_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install the main header file
install(FILES linx_sdk.h
    DESTINATION include
)

# Install subdirectory headers
install(DIRECTORY log/ 
    DESTINATION include/log
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY cjson/
    DESTINATION include/cjson  
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY mcp/
    DESTINATION include/mcp
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY protocols/
    DESTINATION include/protocols
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY codecs/
    DESTINATION include/codecs
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY audio/
    DESTINATION include/audio
    FILES_MATCHING PATTERN "*.h"
)

# Install third-party libraries
# Install mongoose library
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/mongoose/install/
    DESTINATION third/mongoose
)

# Install opus library  
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/opus/install/
    DESTINATION third/opus
)
