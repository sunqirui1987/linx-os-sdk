cmake_minimum_required(VERSION 3.10)
project(linx_sdk C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


# Include find modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# copy files
set(patch_dir "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch/mongoose")
set(mongoose_dir "${CMAKE_CURRENT_SOURCE_DIR}/third/mongoose")

if(EXISTS "${patch_dir}")
    message(STATUS "Copying mongoose patch files from ${patch_dir} to ${mongoose_dir}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${patch_dir}" "${mongoose_dir}"
        RESULT_VARIABLE copy_result
    )
    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy mongoose patch files")
    endif()
endif()


# =============================================================================
# Third-party libraries (build first as dependencies)
# =============================================================================

# Add third-party libraries
add_subdirectory(third/mongoose)
add_subdirectory(third/opus)
add_subdirectory(third/fonts)
add_subdirectory(third/liblvgl)

# =============================================================================
# Add subdirectories in dependency order
# =============================================================================

add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(audio)
add_subdirectory(display)
add_subdirectory(linxsdk)


# =============================================================================
# Create unified static library
# =============================================================================

# Create the main SDK library that links all component libraries
add_library(linx_sdk_static STATIC
    linxsdk/linx_sdk.c
)

# =============================================================================
# Configure target properties
# =============================================================================

# Set target properties
set_target_properties(linx_sdk_static PROPERTIES
    OUTPUT_NAME "linx_sdk"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    PREFIX "lib"
    SUFFIX ".a"
)

# Set public include directories for consumers of this library
target_include_directories(linx_sdk_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/linxsdk
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/display
)

# Link with component libraries
target_link_libraries(linx_sdk_static PUBLIC
    linx_common
    linx_camera
    linx_audio
    linx_display
    linx_sdk
)

# Link with third-party libraries
if(TARGET mongoose)
    target_link_libraries(linx_sdk_static PUBLIC mongoose)
endif()

if(TARGET opus)
    target_link_libraries(linx_sdk_static PUBLIC opus)
endif()

if(TARGET fonts)
    target_link_libraries(linx_sdk_static PUBLIC fonts)
endif()

if(TARGET lvgl)
    target_link_libraries(linx_sdk_static PUBLIC lvgl)
endif()

# Platform-specific libraries
if(APPLE)
    target_link_libraries(linx_sdk_static PUBLIC
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework AudioUnit"
    )
elseif(UNIX)
    target_link_libraries(linx_sdk_static PUBLIC
        pthread
        m
        asound
    )
endif()

# Compiler flags
target_compile_options(linx_sdk_static PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function
    -O2
)




# =============================================================================
# Installation configuration
# =============================================================================

# Set installation directory to project root/out/linx
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../out/linx CACHE PATH "Installation directory" FORCE)
message(STATUS "SDK installation directory: ${CMAKE_INSTALL_PREFIX}")

# Install all libraries
install(TARGETS linx_sdk_static linx_common linx_camera linx_audio linx_display linx_sdk
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)



# Install component headers with src directory structure
install(DIRECTORY common/
    DESTINATION include/common
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY camera/
    DESTINATION include/camera
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY audio/
    DESTINATION include/audio
    FILES_MATCHING PATTERN "*.h"
    PATTERN "test" EXCLUDE
)

install(DIRECTORY display/
    DESTINATION include/display
    FILES_MATCHING PATTERN "*.h"
    PATTERN "test" EXCLUDE
)

install(DIRECTORY linxsdk/
    DESTINATION include/linxsdk
    FILES_MATCHING PATTERN "*.h"
)

# Install third-party headers with src directory structure
install(DIRECTORY third/
    DESTINATION include/third
    FILES_MATCHING PATTERN "*.h"
    PATTERN "test" EXCLUDE
    PATTERN "build" EXCLUDE
    PATTERN ".git*" EXCLUDE
    PATTERN "examples" EXCLUDE
    PATTERN "tutorials" EXCLUDE
    PATTERN "doc" EXCLUDE
    PATTERN "docs" EXCLUDE
)
