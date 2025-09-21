# ESP32 Toolchain File for Linx SDK

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

# Set cross-compilation flag
set(CMAKE_CROSSCOMPILING TRUE)

# Check for ESP-IDF environment
if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH environment variable is not set. Please source ESP-IDF environment.")
endif()

set(IDF_PATH $ENV{IDF_PATH})

# Set target chip (default to esp32)
if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET esp32)
endif()

# ESP32 toolchain paths
if(IDF_TARGET STREQUAL "esp32")
    set(TOOLCHAIN_PREFIX xtensa-esp32-elf)
elseif(IDF_TARGET STREQUAL "esp32s2")
    set(TOOLCHAIN_PREFIX xtensa-esp32s2-elf)
elseif(IDF_TARGET STREQUAL "esp32s3")
    set(TOOLCHAIN_PREFIX xtensa-esp32s3-elf)
elseif(IDF_TARGET STREQUAL "esp32c3")
    set(TOOLCHAIN_PREFIX riscv32-esp-elf)
else()
    message(FATAL_ERROR "Unsupported ESP32 target: ${IDF_TARGET}")
endif()

# Find toolchain
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "ESP32 toolchain not found. Please install ESP-IDF and ensure it's in PATH.")
endif()

# Set other tools
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}-objcopy)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}-objdump)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}-size)
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
find_program(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)

# Set compiler flags
set(CMAKE_C_FLAGS_INIT "-mlongcalls")
set(CMAKE_CXX_FLAGS_INIT "-mlongcalls")

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib")

# Don't run the linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set find root path
set(CMAKE_FIND_ROOT_PATH ${IDF_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ESP32 specific definitions
add_definitions(-DESP_PLATFORM=1)
add_definitions(-DLINX_TARGET_ESP32=1)

message(STATUS "ESP32 toolchain configured for target: ${IDF_TARGET}")
message(STATUS "Toolchain prefix: ${TOOLCHAIN_PREFIX}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")