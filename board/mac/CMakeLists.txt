cmake_minimum_required(VERSION 3.10)
project(linx_board_mac C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include platform detection
include(${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/cmake/PlatformDetection.cmake)

# Verify this is macOS platform
if(NOT LINX_PLATFORM_NAME STREQUAL "macos")
    message(FATAL_ERROR "This board configuration is only for macOS platform. Detected: ${LINX_PLATFORM_NAME}")
endif()

message(STATUS "Building Linx SDK for macOS platform ${CMAKE_CURRENT_SOURCE_DIR}")

# =============================================================================
# Installation Directory Configuration
# =============================================================================

# Set the installation base directory to project root/out/linx
set(LINX_MAC_INSTALL_BASE "${CMAKE_CURRENT_SOURCE_DIR}/../../out/linx")
message(STATUS "Mac board installation base directory: ${LINX_MAC_INSTALL_BASE}")

# =============================================================================
# Mac Platform Audio Sources
# =============================================================================

# Mac platform specific audio sources
set(MAC_AUDIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/audio/portaudio_mac.c
)

set(MAC_AUDIO_HEADERS
    common/audio/portaudio_mac.h
)

# =============================================================================
# PortAudio Dependency Configuration
# =============================================================================

# Try to find PortAudio using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PORTAUDIO portaudio-2.0)
endif()

# If pkg-config didn't find PortAudio, try manual detection
if(NOT PORTAUDIO_FOUND)
    # Common PortAudio installation paths on macOS
    set(PORTAUDIO_SEARCH_PATHS
        /usr/local
        /opt/homebrew
        /opt/local
        /usr
    )
    
    # Find PortAudio header
    find_path(PORTAUDIO_INCLUDE_DIR
        NAMES portaudio.h
        PATHS ${PORTAUDIO_SEARCH_PATHS}
        PATH_SUFFIXES include
        DOC "PortAudio include directory"
    )
    
    # Find PortAudio library
    find_library(PORTAUDIO_LIBRARY
        NAMES portaudio
        PATHS ${PORTAUDIO_SEARCH_PATHS}
        PATH_SUFFIXES lib
        DOC "PortAudio library"
    )
    
    if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
        set(PORTAUDIO_FOUND TRUE)
        set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})
        set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
        message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
    else()
        message(WARNING "PortAudio not found. Please install PortAudio:")
        message(WARNING "  brew install portaudio")
        message(WARNING "  or")
        message(WARNING "  sudo port install portaudio")
    endif()
endif()

# =============================================================================
# Mac Platform Libraries
# =============================================================================

# macOS system frameworks and libraries required for audio
set(MAC_PLATFORM_LIBS
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework AudioUnit"
    "-framework CoreFoundation"
    "-framework CoreServices"
    pthread
)

# Add PortAudio if found
if(PORTAUDIO_FOUND)
    list(APPEND MAC_PLATFORM_LIBS ${PORTAUDIO_LIBRARIES})
endif()

# =============================================================================
# Include Directories
# =============================================================================

set(MAC_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/common/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/log
)

# Add PortAudio include directories if found
if(PORTAUDIO_FOUND)
    list(APPEND MAC_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIRS})
endif()

# =============================================================================
# Compiler Flags for macOS
# =============================================================================

set(MAC_COMPILE_FLAGS
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fPIC
)

# Debug/Release specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND MAC_COMPILE_FLAGS -g -O0 -DDEBUG)
else()
    list(APPEND MAC_COMPILE_FLAGS -O2 -DNDEBUG)
endif()

# =============================================================================
# Export Variables to Parent Scope
# =============================================================================

# Export Mac audio sources for use by parent CMakeLists.txt
set(LINX_MAC_AUDIO_SOURCES ${MAC_AUDIO_SOURCES} PARENT_SCOPE)
set(LINX_MAC_AUDIO_HEADERS ${MAC_AUDIO_HEADERS} PARENT_SCOPE)

# Export platform-specific libraries
set(LINX_MAC_PLATFORM_LIBS ${MAC_PLATFORM_LIBS} PARENT_SCOPE)

# Export include directories
set(LINX_MAC_INCLUDE_DIRS ${MAC_INCLUDE_DIRS} PARENT_SCOPE)

# Export compile flags
set(LINX_MAC_COMPILE_FLAGS ${MAC_COMPILE_FLAGS} PARENT_SCOPE)

# =============================================================================
# Status Messages
# =============================================================================

message(STATUS "=== Mac Board Configuration ===")
message(STATUS "Mac Audio Sources: ${MAC_AUDIO_SOURCES}")
message(STATUS "Mac Include Dirs: ${MAC_INCLUDE_DIRS}")
message(STATUS "Mac Platform Libs: ${MAC_PLATFORM_LIBS}")
message(STATUS "PortAudio Found: ${PORTAUDIO_FOUND}")
if(PORTAUDIO_FOUND)
    message(STATUS "PortAudio Include: ${PORTAUDIO_INCLUDE_DIRS}")
    message(STATUS "PortAudio Library: ${PORTAUDIO_LIBRARIES}")
endif()
message(STATUS "===============================")

# =============================================================================
# Create Mac-specific static library
# =============================================================================

# Create the Mac board static library
add_library(linx_board_mac STATIC ${MAC_AUDIO_SOURCES})

# Set library properties
set_target_properties(linx_board_mac PROPERTIES
    OUTPUT_NAME linx_board_mac
    VERSION 1.0.0
    SOVERSION 1
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Set include directories for the library
target_include_directories(linx_board_mac 
    PUBLIC 
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common/audio>
        $<INSTALL_INTERFACE:include/board/mac>
        $<INSTALL_INTERFACE:include/sdk/audio>
        $<INSTALL_INTERFACE:include/sdk/log>
    PRIVATE
        ${MAC_INCLUDE_DIRS}
)

# Link required libraries
target_link_libraries(linx_board_mac 
    PRIVATE 
        ${MAC_PLATFORM_LIBS}
)

# Set compile options
target_compile_options(linx_board_mac 
    PRIVATE 
        ${MAC_COMPILE_FLAGS}
)

# Set compile definitions for macOS
target_compile_definitions(linx_board_mac 
    PRIVATE
        __APPLE__
        PORTAUDIO_MAC_IMPLEMENTATION
)

# =============================================================================
# Installation Configuration
# =============================================================================

# Install the static library
install(TARGETS linx_board_mac
    EXPORT LinxBoardMacTargets
    ARCHIVE DESTINATION ${LINX_MAC_INSTALL_BASE}/lib
    LIBRARY DESTINATION ${LINX_MAC_INSTALL_BASE}/lib
    RUNTIME DESTINATION ${LINX_MAC_INSTALL_BASE}/bin
    PUBLIC_HEADER DESTINATION ${LINX_MAC_INSTALL_BASE}/include/board/mac
)

# Install headers
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/common/audio/portaudio_mac.h
    DESTINATION ${LINX_MAC_INSTALL_BASE}/include/board/mac/audio
)

# Export targets for use by other projects
install(EXPORT LinxBoardMacTargets
    FILE LinxBoardMacTargets.cmake
    NAMESPACE LinxSDK::
    DESTINATION ${LINX_MAC_INSTALL_BASE}/lib/cmake/LinxBoardMac
)

# Create and install package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    LinxBoardMacConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

# Create a simple config file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/LinxBoardMacConfig.cmake
"# LinxBoardMac Package Configuration File
include(\"\${CMAKE_CURRENT_LIST_DIR}/LinxBoardMacTargets.cmake\")
set(LinxBoardMac_FOUND TRUE)
set(LinxBoardMac_LIBRARIES LinxSDK::linx_board_mac)
set(LinxBoardMac_INCLUDE_DIRS \"\${CMAKE_CURRENT_LIST_DIR}/../../include/board/mac\")
")

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LinxBoardMacConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LinxBoardMacConfigVersion.cmake
    DESTINATION ${LINX_MAC_INSTALL_BASE}/lib/cmake/LinxBoardMac
)