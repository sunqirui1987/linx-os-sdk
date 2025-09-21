cmake_minimum_required(VERSION 3.10)
project(mongoose C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Check if mongoose.c and mongoose.h exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mongoose.c")
    message(FATAL_ERROR "mongoose.c not found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mongoose.h")
    message(FATAL_ERROR "mongoose.h not found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Create mongoose static library
add_library(mongoose STATIC mongoose.c)

# Set target properties
set_target_properties(mongoose PROPERTIES
    OUTPUT_NAME mongoose
    VERSION 7.0.0
    SOVERSION 7
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Include directories
target_include_directories(mongoose PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Compiler definitions
target_compile_definitions(mongoose PUBLIC
    MG_ENABLE_LINES=1
    MG_ENABLE_IPV6=1
    MG_ENABLE_LOG=1
)

# Compiler flags
target_compile_options(mongoose PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fPIC
)

# Debug/Release specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(mongoose PRIVATE -g -O0)
    target_compile_definitions(mongoose PRIVATE MG_ENABLE_LOG=1)
else()
    target_compile_options(mongoose PRIVATE -O2)
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(mongoose PRIVATE MG_ARCH=MG_ARCH_UNIX)
elseif(UNIX)
    target_compile_definitions(mongoose PRIVATE MG_ARCH=MG_ARCH_UNIX)
    target_link_libraries(mongoose PRIVATE rt)
elseif(WIN32)
    target_compile_definitions(mongoose PRIVATE MG_ARCH=MG_ARCH_WIN32)
    target_link_libraries(mongoose PRIVATE ws2_32)
endif()

# Install configuration
install(TARGETS mongoose
    EXPORT mongooseTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES mongoose.h DESTINATION include)

# Export targets
install(EXPORT mongooseTargets
    FILE mongooseTargets.cmake
    NAMESPACE mongoose::
    DESTINATION lib/cmake/mongoose
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/mongooseConfigVersion.cmake"
    VERSION 7.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/mongooseConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mongooseConfig.cmake"
    INSTALL_DESTINATION lib/cmake/mongoose
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mongooseConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mongooseConfigVersion.cmake"
    DESTINATION lib/cmake/mongoose
)

# Print configuration summary
message(STATUS "=== Mongoose Configuration Summary ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Library Output: ${CMAKE_BINARY_DIR}/lib/libmongoose.a")
message(STATUS "======================================")