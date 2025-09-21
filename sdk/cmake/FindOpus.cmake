# FindOpus.cmake
# Find the Opus audio codec library
#
# This module defines:
#  Opus_FOUND - True if Opus is found
#  Opus_INCLUDE_DIRS - Include directories for Opus
#  Opus_LIBRARIES - Libraries to link against
#  Opus_VERSION - Version of the Opus library
#
# You can set these variables to help this module find Opus:
#  Opus_ROOT - Root directory of Opus installation
#  OPUS_ROOT - Alternative name for Opus_ROOT

# Set search paths
set(_opus_search_paths)

# Check for user-specified root directory
if(Opus_ROOT)
    list(APPEND _opus_search_paths ${Opus_ROOT})
endif()

if(OPUS_ROOT)
    list(APPEND _opus_search_paths ${OPUS_ROOT})
endif()

# Check for environment variables
if(DEFINED ENV{OPUS_ROOT})
    list(APPEND _opus_search_paths $ENV{OPUS_ROOT})
endif()

# Add default search paths
list(APPEND _opus_search_paths
    ${CMAKE_CURRENT_SOURCE_DIR}/third/opus/install
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/opus/install
)

# Find opus.h header file
find_path(Opus_INCLUDE_DIR
    NAMES opus.h
    PATHS ${_opus_search_paths}
    PATH_SUFFIXES include include/opus opus
    DOC "Opus include directory"
)

# Find libopus library
find_library(Opus_LIBRARY
    NAMES opus libopus
    PATHS ${_opus_search_paths}
    PATH_SUFFIXES lib lib64
    DOC "Opus library"
)

# Try to find version information
if(Opus_INCLUDE_DIR)
    file(READ "${Opus_INCLUDE_DIR}/opus_defines.h" _opus_defines_content)
    string(REGEX MATCH "#define OPUS_VERSION \"([^\"]+)\"" _opus_version_match "${_opus_defines_content}")
    if(_opus_version_match)
        set(Opus_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

# Set variables
if(Opus_INCLUDE_DIR AND Opus_LIBRARY)
    set(Opus_FOUND TRUE)
    set(Opus_INCLUDE_DIRS ${Opus_INCLUDE_DIR})
    set(Opus_LIBRARIES ${Opus_LIBRARY})
    
    # Set the parent scope variables so they can be used in the main CMakeLists.txt
    set(Opus_FOUND TRUE PARENT_SCOPE)
    set(Opus_INCLUDE_DIRS ${Opus_INCLUDE_DIR} PARENT_SCOPE)
    set(Opus_LIBRARIES ${Opus_LIBRARY} PARENT_SCOPE)
    
    if(Opus_VERSION)
        set(Opus_VERSION ${Opus_VERSION} PARENT_SCOPE)
    endif()
    
    message(STATUS "Found Opus: ${Opus_LIBRARY}")
    message(STATUS "Opus include: ${Opus_INCLUDE_DIR}")
    if(Opus_VERSION)
        message(STATUS "Opus version: ${Opus_VERSION}")
    endif()
else()
    set(Opus_FOUND FALSE)
    if(Opus_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find Opus library. Please set Opus_ROOT or OPUS_ROOT.")
    endif()
endif()

# Create imported target if found
if(Opus_FOUND AND NOT TARGET Opus::opus)
    add_library(Opus::opus UNKNOWN IMPORTED)
    set_target_properties(Opus::opus PROPERTIES
        IMPORTED_LOCATION "${Opus_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Opus_INCLUDE_DIRS}"
    )
    
    # Add compile definitions that might be needed
    set_target_properties(Opus::opus PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "OPUS_BUILD"
    )
endif()

# Mark variables as advanced
mark_as_advanced(
    Opus_INCLUDE_DIR
    Opus_LIBRARY
)