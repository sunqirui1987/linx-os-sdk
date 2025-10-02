cmake_minimum_required(VERSION 3.10)
project(linx_camera)

# Set project name
set(LINX_CAMERA_TARGET linx_camera)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
)

# Collect all source files
set(LINX_CAMERA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_stub.c
)

# Create static library
add_library(${LINX_CAMERA_TARGET} STATIC ${LINX_CAMERA_SOURCES})

# Set target properties for static library
set_target_properties(${LINX_CAMERA_TARGET} PROPERTIES
    OUTPUT_NAME "linx_camera"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PREFIX "lib"
    SUFFIX ".a"
)

# Target include directories (for consumers of this library)
target_include_directories(${LINX_CAMERA_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler flags
target_compile_options(${LINX_CAMERA_TARGET} PRIVATE
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
    target_link_libraries(${LINX_CAMERA_TARGET} PUBLIC linx_common)
endif()

# Install rules
install(TARGETS ${LINX_CAMERA_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_stub.h
    DESTINATION include/linx_camera
)

# Print build information
message(STATUS "Building LINX Camera Library")
message(STATUS "  - Static library: linx_camera")