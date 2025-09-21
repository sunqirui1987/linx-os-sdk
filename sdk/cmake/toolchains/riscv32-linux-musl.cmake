# RISC-V 32-bit Linux musl Cross-Compilation Toolchain for Linx SDK
# Suitable for RISC-V 32-bit embedded Linux systems with musl libc

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

# Set cross-compilation flag
set(CMAKE_CROSSCOMPILING TRUE)

# Toolchain prefix
set(TOOLCHAIN_PREFIX riscv32-linux-musl)

# Default toolchain path - can be overridden by environment variable
if(DEFINED ENV{RISCV32_TOOLCHAIN_PATH})
    set(TOOLCHAIN_PATH $ENV{RISCV32_TOOLCHAIN_PATH})
else()
    set(TOOLCHAIN_PATH "/home/sqr-ubuntu/nds32le-linux-musl-v5d")
endif()

# Find the cross-compiler
find_program(CMAKE_C_COMPILER 
    NAMES ${TOOLCHAIN_PREFIX}-gcc
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_CXX_COMPILER 
    NAMES ${TOOLCHAIN_PREFIX}-g++
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

# Alternative toolchain names to try if not found
if(NOT CMAKE_C_COMPILER)
    set(ALT_TOOLCHAIN_PREFIXES
        "riscv32-linux-musl"
        "riscv32-unknown-linux-musl"
        "riscv32-buildroot-linux-musl"
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
    message(FATAL_ERROR "RISC-V 32-bit cross-compiler not found. Please install riscv32-linux-musl toolchain or set RISCV32_TOOLCHAIN_PATH environment variable.")
endif()

# Set other tools
find_program(CMAKE_AR 
    NAMES ${TOOLCHAIN_PREFIX}-ar ${TOOLCHAIN_PREFIX}-gcc-ar
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_RANLIB 
    NAMES ${TOOLCHAIN_PREFIX}-ranlib ${TOOLCHAIN_PREFIX}-gcc-ranlib
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_STRIP 
    NAMES ${TOOLCHAIN_PREFIX}-strip
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_OBJCOPY 
    NAMES ${TOOLCHAIN_PREFIX}-objcopy
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_OBJDUMP 
    NAMES ${TOOLCHAIN_PREFIX}-objdump
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_SIZE 
    NAMES ${TOOLCHAIN_PREFIX}-size
    PATHS ${TOOLCHAIN_PATH}/bin
    NO_DEFAULT_PATH
)

# Set compiler flags for RISC-V 32-bit
# Using RV32IMAFD (Integer, Multiplication, Atomic, Float, Double) instruction set with ilp32d ABI
set(CMAKE_C_FLAGS_INIT "-march=rv32imafd -mabi=ilp32d -mcmodel=medlow")
set(CMAKE_CXX_FLAGS_INIT "-march=rv32imafd -mabi=ilp32d -mcmodel=medlow")

# Set linker flags to find the correct library and startup files paths
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${TOOLCHAIN_PATH}/sysroot/lib32/ilp32d -B${TOOLCHAIN_PATH}/sysroot/lib32/ilp32d")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${TOOLCHAIN_PATH}/sysroot/lib32/ilp32d -B${TOOLCHAIN_PATH}/sysroot/lib32/ilp32d")

# Set sysroot if available
if(DEFINED ENV{RISCV32_SYSROOT})
    set(CMAKE_SYSROOT $ENV{RISCV32_SYSROOT})
    message(STATUS "Using sysroot: ${CMAKE_SYSROOT}")
elseif(EXISTS "${TOOLCHAIN_PATH}/sysroot")
    set(CMAKE_SYSROOT "${TOOLCHAIN_PATH}/sysroot")
    message(STATUS "Using default RISC-V sysroot: ${CMAKE_SYSROOT}")
elseif(EXISTS "${TOOLCHAIN_PATH}/riscv32-linux-musl")
    set(CMAKE_SYSROOT "${TOOLCHAIN_PATH}/riscv32-linux-musl")
    message(STATUS "Using toolchain sysroot: ${CMAKE_SYSROOT}")
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

# RISC-V specific definitions
add_definitions(-DLINX_TARGET_RISCV32=1)
add_definitions(-DLINX_CROSS_COMPILE=1)
add_definitions(-DLINX_MUSL_LIBC=1)

# Set target properties
set(CMAKE_SYSTEM_VERSION 1)

# Optimization flags for embedded systems
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections")

message(STATUS "RISC-V 32-bit cross-compilation toolchain configured")
message(STATUS "Toolchain prefix: ${TOOLCHAIN_PREFIX}")
message(STATUS "Toolchain path: ${TOOLCHAIN_PATH}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
if(CMAKE_SYSROOT)
    message(STATUS "Sysroot: ${CMAKE_SYSROOT}")
endif()
message(STATUS "Find root path: ${CMAKE_FIND_ROOT_PATH}")