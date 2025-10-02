cmake_minimum_required(VERSION 3.10)
project(linx_display)

# Set project name
set(LINX_DISPLAY_TARGET linx_display)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/fonts/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/liblvgl/v9/
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/liblvgl/v9/conf/
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/liblvgl/v9/lvgl/
)

# Collect LVGL display source files recursively
file(GLOB_RECURSE LVGL_DISPLAY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display/*.c"
)

# Exclude test files
list(FILTER LVGL_DISPLAY_SOURCES EXCLUDE REGEX ".*/test/.*")

# Collect LVGL display header files recursively
file(GLOB_RECURSE LVGL_DISPLAY_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display/*.h"
)

# Exclude test headers
list(FILTER LVGL_DISPLAY_HEADERS EXCLUDE REGEX ".*/test/.*")

# Collect all source files
set(LINX_DISPLAY_SOURCES
    # Main display sources
    ${CMAKE_CURRENT_SOURCE_DIR}/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/display_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/theme/theme.c
    
    # LVGL display sources (auto-discovered)
    ${LVGL_DISPLAY_SOURCES}
)

# Create static library
add_library(${LINX_DISPLAY_TARGET} STATIC ${LINX_DISPLAY_SOURCES})

# Set target properties for static library
set_target_properties(${LINX_DISPLAY_TARGET} PROPERTIES
    OUTPUT_NAME "linx_display"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PREFIX "lib"
    SUFFIX ".a"
)

# Target include directories (for consumers of this library)
target_include_directories(${LINX_DISPLAY_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/theme
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
)

# Compiler flags
target_compile_options(${LINX_DISPLAY_TARGET} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function
    -O2
)

# Link with common libraries if they exist
if(TARGET linx_log)
    target_link_libraries(${LINX_DISPLAY_TARGET} PUBLIC linx_log)
endif()

if(TARGET lvgl)
    target_link_libraries(${LINX_DISPLAY_TARGET} PUBLIC lvgl)
endif()

# Install rules
install(TARGETS ${LINX_DISPLAY_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install header files
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/display.h
    ${CMAKE_CURRENT_SOURCE_DIR}/display_stub.h
    DESTINATION include/linx_display
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/theme/theme.h
    DESTINATION include/linx_display/theme
)

# Install LVGL display headers (auto-discovered)
foreach(HEADER_FILE ${LVGL_DISPLAY_HEADERS})
    # Get relative path from lvgl_display directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lvgl_display" "${HEADER_FILE}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    
    # Install to appropriate subdirectory
    if(REL_DIR STREQUAL "")
        install(FILES "${HEADER_FILE}" DESTINATION include/linx_display/lvgl_display)
    else()
        install(FILES "${HEADER_FILE}" DESTINATION include/linx_display/lvgl_display/${REL_DIR})
    endif()
endforeach()
