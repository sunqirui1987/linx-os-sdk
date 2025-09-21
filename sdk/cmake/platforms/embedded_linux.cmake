# Embedded Linux Platform Configuration for Linx SDK
# Suitable for ARM-based embedded systems like Raspberry Pi, BeagleBone, etc.

message(STATUS "Configuring for Embedded Linux platform")

# Set embedded Linux specific flags
set(LINX_PLATFORM_EMBEDDED_LINUX TRUE)
set(LINX_PLATFORM_EMBEDDED TRUE)

# Embedded Linux specific definitions
add_definitions(-DLINX_PLATFORM_EMBEDDED_LINUX=1)
add_definitions(-DLINX_PLATFORM_EMBEDDED=1)

# Memory constraints for embedded systems
add_definitions(-DLINX_MEMORY_CONSTRAINED=1)

# Cross-compilation settings
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for embedded Linux")
    set(LINX_CROSS_COMPILING TRUE)
else()
    message(STATUS "Native compilation for embedded Linux")
    set(LINX_CROSS_COMPILING FALSE)
endif()

# Embedded-specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")  # Optimize for size
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")

# Remove debug symbols in release builds to save space
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# Function to detect embedded platform type
function(detect_embedded_platform)
    # Check for Raspberry Pi
    if(EXISTS "/proc/device-tree/model")
        file(READ "/proc/device-tree/model" DEVICE_MODEL)
        if(DEVICE_MODEL MATCHES "Raspberry Pi")
            set(LINX_EMBEDDED_TYPE "raspberry_pi" PARENT_SCOPE)
            message(STATUS "Detected Raspberry Pi")
        endif()
    endif()
    
    # Check for BeagleBone
    if(EXISTS "/proc/device-tree/model")
        file(READ "/proc/device-tree/model" DEVICE_MODEL)
        if(DEVICE_MODEL MATCHES "BeagleBone")
            set(LINX_EMBEDDED_TYPE "beaglebone" PARENT_SCOPE)
            message(STATUS "Detected BeagleBone")
        endif()
    endif()
    
    # Check CPU info for ARM
    if(EXISTS "/proc/cpuinfo")
        file(READ "/proc/cpuinfo" CPU_INFO)
        if(CPU_INFO MATCHES "ARM")
            if(NOT DEFINED LINX_EMBEDDED_TYPE)
                set(LINX_EMBEDDED_TYPE "generic_arm" PARENT_SCOPE)
                message(STATUS "Detected generic ARM platform")
            endif()
        endif()
    endif()
    
    # Default to generic embedded if not detected
    if(NOT DEFINED LINX_EMBEDDED_TYPE)
        set(LINX_EMBEDDED_TYPE "generic_embedded" PARENT_SCOPE)
        message(STATUS "Using generic embedded configuration")
    endif()
endfunction()

# Function to check embedded Linux dependencies
function(check_embedded_dependencies)
    message(STATUS "Checking embedded Linux dependencies...")
    
    # Check for cross-compilation toolchain
    if(LINX_CROSS_COMPILING)
        if(NOT CMAKE_C_COMPILER)
            message(FATAL_ERROR "Cross-compiler not specified. Set CMAKE_C_COMPILER.")
        endif()
        
        message(STATUS "Using cross-compiler: ${CMAKE_C_COMPILER}")
    endif()
    
    # Check for pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        message(STATUS "✓ pkg-config found")
        
        # Check for minimal required libraries
        pkg_check_modules(THREADS pthread)
        if(THREADS_FOUND)
            message(STATUS "✓ pthread found")
            set(LINX_THREADS_FOUND TRUE PARENT_SCOPE)
        endif()
    else()
        message(WARNING "pkg-config not found. Some dependencies may not be detected.")
    endif()
    
    # Check for basic system libraries
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        message(STATUS "✓ Math library found")
        set(LINX_MATH_LIBRARY ${MATH_LIBRARY} PARENT_SCOPE)
    endif()
    
    find_library(DL_LIBRARY dl)
    if(DL_LIBRARY)
        message(STATUS "✓ Dynamic loading library found")
        set(LINX_DL_LIBRARY ${DL_LIBRARY} PARENT_SCOPE)
    endif()
endfunction()

# Function to configure embedded features
function(configure_embedded_features)
    message(STATUS "Configuring features for embedded platform...")
    
    # Disable resource-intensive features
    set(LINX_ENABLE_AUDIO FALSE PARENT_SCOPE)  # Audio often not available
    set(LINX_ENABLE_WEBSOCKET TRUE PARENT_SCOPE)  # WebSocket is useful for IoT
    set(LINX_ENABLE_MCP TRUE PARENT_SCOPE)  # MCP is lightweight
    
    # Enable minimal logging
    set(LINX_ENABLE_MINIMAL_LOGGING TRUE PARENT_SCOPE)
    
    # Disable tests and examples to save space
    set(LINX_ENABLE_TESTS FALSE PARENT_SCOPE)
    set(LINX_ENABLE_EXAMPLES FALSE PARENT_SCOPE)
    
    # Set memory limits
    add_definitions(-DLINX_MAX_BUFFER_SIZE=1024)
    add_definitions(-DLINX_MAX_CONNECTIONS=4)
    add_definitions(-DLINX_MAX_MESSAGE_SIZE=512)
    
    message(STATUS "Embedded features configured")
endfunction()

# Function to setup embedded build options
function(setup_embedded_build_options)
    # Static linking preferred for embedded systems
    set(BUILD_SHARED_LIBS OFF PARENT_SCOPE)
    
    # Strip symbols in release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s" PARENT_SCOPE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s" PARENT_SCOPE)
    endif()
    
    # Enable link-time optimization for size
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE PARENT_SCOPE)
    
    # Set installation prefix for embedded systems
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/opt/linx" CACHE PATH "Installation prefix for embedded Linux" FORCE)
    endif()
endfunction()

# Function to find embedded libraries
function(find_embedded_libraries)
    # Find threading library
    find_package(Threads REQUIRED)
    set(LINX_SYSTEM_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
    
    # Add math library if found
    if(LINX_MATH_LIBRARY)
        list(APPEND LINX_SYSTEM_LIBRARIES ${LINX_MATH_LIBRARY})
        set(LINX_SYSTEM_LIBRARIES ${LINX_SYSTEM_LIBRARIES} PARENT_SCOPE)
    endif()
    
    # Add dl library if found
    if(LINX_DL_LIBRARY)
        list(APPEND LINX_SYSTEM_LIBRARIES ${LINX_DL_LIBRARY})
        set(LINX_SYSTEM_LIBRARIES ${LINX_SYSTEM_LIBRARIES} PARENT_SCOPE)
    endif()
    
    message(STATUS "Embedded system libraries: ${LINX_SYSTEM_LIBRARIES}")
endfunction()

# Detect embedded platform type
detect_embedded_platform()

# Check dependencies
check_embedded_dependencies()

# Configure features
configure_embedded_features()

# Setup build options
setup_embedded_build_options()

# Find libraries
find_embedded_libraries()

message(STATUS "Embedded Linux platform configuration completed")