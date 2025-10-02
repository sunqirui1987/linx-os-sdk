# Fonts Library CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(Fonts LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Automatically collect all source files
file(GLOB_RECURSE FONTS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Automatically collect all header files
file(GLOB FONTS_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# Create the fonts library
add_library(fonts ${FONTS_SOURCES} ${FONTS_PUBLIC_HEADERS})
add_library(Fonts::fonts ALIAS fonts)

# Set target properties
set_target_properties(fonts
    PROPERTIES
    PUBLIC_HEADER "${FONTS_PUBLIC_HEADERS}"
    VERSION 1.0.0
    SOVERSION 1
)

# Configure include directories
target_include_directories(fonts
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link with LVGL library (v9 target from liblvgl)
target_link_libraries(fonts PUBLIC v9)

# Add LVGL include directories to fonts target
target_include_directories(fonts PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../liblvgl/v9/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/../liblvgl/v9
)

message(STATUS "Fonts library linked with LVGL v9")
message(STATUS "LVGL include paths added")

# Compiler definitions
target_compile_definitions(fonts PRIVATE FONTS_BUILD)