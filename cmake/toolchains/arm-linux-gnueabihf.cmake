# ARM Linux Cross-Compilation Toolchain for Linx SDK
# Suitable for Raspberry Pi and other ARM-based embedded Linux systems

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set cross-compilation flag
set(CMAKE_CROSSCOMPILING TRUE)

# Toolchain prefix
set(TOOLCHAIN_PREFIX arm-linux-gnueabihf)

# Find the cross-compiler
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

# Alternative toolchain names to try
if(NOT CMAKE_C_COMPILER)
    set(ALT_TOOLCHAIN_PREFIXES
        "arm-linux-gnueabihf"
        "arm-buildroot-linux-gnueabihf"
        "arm-rpi-linux-gnueabihf"
        "armv7l-linux-gnueabihf"
    )
    
    foreach(prefix ${ALT_TOOLCHAIN_PREFIXES})
        find_program(CMAKE_C_COMPILER ${prefix}-gcc)
        if(CMAKE_C_COMPILER)
            set(TOOLCHAIN_PREFIX ${prefix})
            find_program(CMAKE_CXX_COMPILER ${prefix}-g++)
            break()
        endif()
    endforeach()
endif()

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "ARM cross-compiler not found. Please install arm-linux-gnueabihf toolchain.")
endif()

# Set other tools
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
find_program(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)
find_program(CMAKE_STRIP ${TOOLCHAIN_PREFIX}-strip)
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}-objcopy)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}-objdump)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}-size)

# Set compiler flags for ARM
set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")

# Set sysroot if available
if(DEFINED ENV{ARM_SYSROOT})
    set(CMAKE_SYSROOT $ENV{ARM_SYSROOT})
    message(STATUS "Using sysroot: ${CMAKE_SYSROOT}")
elseif(EXISTS "/opt/cross-pi-gcc/arm-linux-gnueabihf/libc")
    set(CMAKE_SYSROOT "/opt/cross-pi-gcc/arm-linux-gnueabihf/libc")
    message(STATUS "Using default Raspberry Pi sysroot: ${CMAKE_SYSROOT}")
endif()

# Set find root path
if(CMAKE_SYSROOT)
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
else()
    # Try to find toolchain root
    get_filename_component(TOOLCHAIN_ROOT ${CMAKE_C_COMPILER} DIRECTORY)
    get_filename_component(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} DIRECTORY)
    set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_ROOT}/${TOOLCHAIN_PREFIX})
endif()

# Set find modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ARM specific definitions
add_definitions(-DLINX_TARGET_ARM=1)
add_definitions(-DLINX_CROSS_COMPILE=1)

# Set target properties
set(CMAKE_SYSTEM_VERSION 1)

message(STATUS "ARM cross-compilation toolchain configured")
message(STATUS "Toolchain prefix: ${TOOLCHAIN_PREFIX}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
if(CMAKE_SYSROOT)
    message(STATUS "Sysroot: ${CMAKE_SYSROOT}")
endif()
message(STATUS "Find root path: ${CMAKE_FIND_ROOT_PATH}")