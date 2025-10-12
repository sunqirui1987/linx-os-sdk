# LinxOS Audio Utils CMake Configuration
cmake_minimum_required(VERSION 3.16)
project(LinxAudioUtils VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

# Source files
set(UTILS_SOURCES
    audio_utils.c
)

set(UTILS_HEADERS
    audio_utils.h
)

# Find required libraries
find_package(Threads REQUIRED)

# Math library
find_library(MATH_LIBRARY m)

# Create static library
add_library(LinxAudioUtils_static STATIC ${UTILS_SOURCES})
target_link_libraries(LinxAudioUtils_static 
    Threads::Threads
)

if(MATH_LIBRARY)
    target_link_libraries(LinxAudioUtils_static ${MATH_LIBRARY})
endif()

# Create shared library (optional)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(LinxAudioUtils_shared SHARED ${UTILS_SOURCES})
    target_link_libraries(LinxAudioUtils_shared 
        Threads::Threads
    )
    
    if(MATH_LIBRARY)
        target_link_libraries(LinxAudioUtils_shared ${MATH_LIBRARY})
    endif()
    
    set_target_properties(LinxAudioUtils_shared PROPERTIES
        OUTPUT_NAME LinxAudioUtils
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Set target properties
set_target_properties(LinxAudioUtils_static PROPERTIES
    OUTPUT_NAME LinxAudioUtils
    CLEAN_DIRECT_OUTPUT 1
)

# Platform-specific settings
if(APPLE)
    set_target_properties(LinxAudioUtils_static PROPERTIES
        MACOSX_RPATH ON
        INSTALL_RPATH "@loader_path"
    )
    if(BUILD_SHARED_LIBS)
        set_target_properties(LinxAudioUtils_shared PROPERTIES
            MACOSX_RPATH ON
            INSTALL_RPATH "@loader_path"
        )
    endif()
    add_definitions(-DLINX_PLATFORM_MACOS=1)
elseif(UNIX)
    set_target_properties(LinxAudioUtils_static PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
    if(BUILD_SHARED_LIBS)
        set_target_properties(LinxAudioUtils_shared PROPERTIES
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
    add_definitions(-DLINX_PLATFORM_LINUX=1)
elseif(WIN32)
    add_definitions(-DLINX_PLATFORM_WINDOWS=1)
endif()

# Installation
install(TARGETS LinxAudioUtils_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SHARED_LIBS)
    install(TARGETS LinxAudioUtils_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(FILES ${UTILS_HEADERS}
    DESTINATION include/linx/audio/utils
)

# Tests (optional)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples (optional)
option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Debug information
message(STATUS "LinxAudioUtils Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Math library: ${MATH_LIBRARY}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")