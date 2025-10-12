# LinxOS Audio Drivers CMake Configuration
cmake_minimum_required(VERSION 3.16)
project(LinxAudioDrivers VERSION 1.0.0 LANGUAGES C)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
)

# Source files
set(DRIVER_SOURCES
    driver_factory.c
    dummy_driver.c
)

set(DRIVER_HEADERS
    audio_driver.h
)

# Platform-specific drivers
if(APPLE)
    list(APPEND DRIVER_SOURCES coreaudio_driver.c)
    set(PLATFORM_LIBS "-framework CoreAudio -framework AudioToolbox")
    add_definitions(-DLINX_PLATFORM_MACOS=1)
elseif(UNIX AND NOT APPLE)
    # Linux drivers (to be implemented)
    # list(APPEND DRIVER_SOURCES alsa_driver.c pulse_driver.c jack_driver.c)
    find_package(PkgConfig REQUIRED)
    
    # ALSA
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        add_definitions(-DLINX_HAVE_ALSA=1)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ALSA_LIBRARIES})
        include_directories(${ALSA_INCLUDE_DIRS})
    endif()
    
    # PulseAudio
    pkg_check_modules(PULSE libpulse-simple)
    if(PULSE_FOUND)
        add_definitions(-DLINX_HAVE_PULSE=1)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${PULSE_LIBRARIES})
        include_directories(${PULSE_INCLUDE_DIRS})
    endif()
    
    # JACK
    pkg_check_modules(JACK jack)
    if(JACK_FOUND)
        add_definitions(-DLINX_HAVE_JACK=1)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${JACK_LIBRARIES})
        include_directories(${JACK_INCLUDE_DIRS})
    endif()
    
    add_definitions(-DLINX_PLATFORM_LINUX=1)
elseif(WIN32)
    # Windows drivers (to be implemented)
    # list(APPEND DRIVER_SOURCES wasapi_driver.c)
    add_definitions(-DLINX_PLATFORM_WINDOWS=1)
endif()

# Find required libraries
find_package(Threads REQUIRED)

# Create static library
add_library(LinxAudioDrivers_static STATIC ${DRIVER_SOURCES})
target_link_libraries(LinxAudioDrivers_static 
    Threads::Threads 
    m
    ${PLATFORM_LIBS}
)

# Create shared library (optional)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    add_library(LinxAudioDrivers_shared SHARED ${DRIVER_SOURCES})
    target_link_libraries(LinxAudioDrivers_shared 
        Threads::Threads 
        m
        ${PLATFORM_LIBS}
    )
    set_target_properties(LinxAudioDrivers_shared PROPERTIES
        OUTPUT_NAME LinxAudioDrivers
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Set target properties
set_target_properties(LinxAudioDrivers_static PROPERTIES
    OUTPUT_NAME LinxAudioDrivers
    CLEAN_DIRECT_OUTPUT 1
)

# Platform-specific settings
if(APPLE)
    set_target_properties(LinxAudioDrivers_static PROPERTIES
        MACOSX_RPATH ON
        INSTALL_RPATH "@loader_path"
    )
    if(BUILD_SHARED_LIBS)
        set_target_properties(LinxAudioDrivers_shared PROPERTIES
            MACOSX_RPATH ON
            INSTALL_RPATH "@loader_path"
        )
    endif()
elseif(UNIX)
    set_target_properties(LinxAudioDrivers_static PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
    if(BUILD_SHARED_LIBS)
        set_target_properties(LinxAudioDrivers_shared PROPERTIES
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endif()

# Installation
install(TARGETS LinxAudioDrivers_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(BUILD_SHARED_LIBS)
    install(TARGETS LinxAudioDrivers_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(FILES ${DRIVER_HEADERS}
    DESTINATION include/linx/audio/drivers
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
message(STATUS "LinxAudioDrivers Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Platform libs: ${PLATFORM_LIBS}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")

if(APPLE)
    message(STATUS "  Platform: macOS")
    message(STATUS "  CoreAudio support: YES")
elseif(UNIX)
    message(STATUS "  Platform: Linux")
    message(STATUS "  ALSA support: ${ALSA_FOUND}")
    message(STATUS "  PulseAudio support: ${PULSE_FOUND}")
    message(STATUS "  JACK support: ${JACK_FOUND}")
elseif(WIN32)
    message(STATUS "  Platform: Windows")
endif()