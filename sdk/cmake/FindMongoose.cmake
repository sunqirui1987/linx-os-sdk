# FindMongoose.cmake
# Find the Mongoose HTTP/WebSocket library
#
# This module defines:
#  Mongoose_FOUND - True if Mongoose is found
#  MONGOOSE_INCLUDE_DIRS - Include directories for Mongoose
#  Mongoose_LIBRARIES - Libraries to link against
#  Mongoose_SOURCES - Source files to compile
#
# You can set these variables to help this module find Mongoose:
#  Mongoose_ROOT - Root directory of Mongoose installation
#  MONGOOSE_ROOT - Alternative name for Mongoose_ROOT

# Set search paths
set(_mongoose_search_paths)

# Check for user-specified root directory
if(Mongoose_ROOT)
    list(APPEND _mongoose_search_paths ${Mongoose_ROOT})
endif()

if(MONGOOSE_ROOT)
    list(APPEND _mongoose_search_paths ${MONGOOSE_ROOT})
endif()

# Check for environment variables
if(DEFINED ENV{MONGOOSE_ROOT})
    list(APPEND _mongoose_search_paths $ENV{MONGOOSE_ROOT})
endif()

# Add default search paths
list(APPEND _mongoose_search_paths
    ${CMAKE_CURRENT_SOURCE_DIR}/third/mongoose/install
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/mongoose/install
)

# Find mongoose.h header file
find_path(Mongoose_INCLUDE_DIR
    NAMES mongoose.h
    PATHS ${_mongoose_search_paths}
    PATH_SUFFIXES include src .
    DOC "Mongoose include directory"
)

# Find mongoose.c source file
find_file(Mongoose_SOURCE_FILE
    NAMES mongoose.c
    PATHS ${_mongoose_search_paths}
    PATH_SUFFIXES src .
    DOC "Mongoose source file"
)

# Set variables
if(Mongoose_INCLUDE_DIR AND Mongoose_SOURCE_FILE)
    set(Mongoose_FOUND TRUE)
    set(MONGOOSE_INCLUDE_DIRS ${Mongoose_INCLUDE_DIR})
    set(Mongoose_SOURCES ${Mongoose_SOURCE_FILE})
    
    # Set the parent scope variables so they can be used in the main CMakeLists.txt
    set(Mongoose_FOUND TRUE PARENT_SCOPE)
    set(MONGOOSE_INCLUDE_DIRS ${Mongoose_INCLUDE_DIR} PARENT_SCOPE)
    set(Mongoose_SOURCES ${Mongoose_SOURCE_FILE} PARENT_SCOPE)
    
    # Mongoose is a header-only library with a single source file
    # No libraries to link against
    set(Mongoose_LIBRARIES "")
    
    # Get the directory containing mongoose.c for consistency
    get_filename_component(Mongoose_SOURCE_DIR ${Mongoose_SOURCE_FILE} DIRECTORY)
    
    message(STATUS "Found Mongoose: ${Mongoose_INCLUDE_DIR}")
    message(STATUS "Mongoose source: ${Mongoose_SOURCE_FILE}")
else()
    set(Mongoose_FOUND FALSE)
    if(Mongoose_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find Mongoose library. Please set Mongoose_ROOT or MONGOOSE_ROOT.")
    endif()
endif()

# Create imported target if found
if(Mongoose_FOUND AND NOT TARGET Mongoose::Mongoose)
    add_library(Mongoose::Mongoose INTERFACE IMPORTED)
    set_target_properties(Mongoose::Mongoose PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MONGOOSE_INCLUDE_DIRS}"
    )
    
    # Add compile definitions that might be needed
    set_target_properties(Mongoose::Mongoose PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "MG_ENABLE_LINES=1"
    )
endif()

# Mark variables as advanced
mark_as_advanced(
    Mongoose_INCLUDE_DIR
    Mongoose_SOURCE_FILE
)