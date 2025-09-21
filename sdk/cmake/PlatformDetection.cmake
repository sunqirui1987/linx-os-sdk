# Platform Detection Module for Linx SDK
# This module detects the target platform and sets appropriate variables

# Initialize platform variables
set(LINX_PLATFORM_DETECTED FALSE)
set(LINX_PLATFORM_NAME "unknown")
set(LINX_PLATFORM_TYPE "unknown")
set(LINX_IS_EMBEDDED FALSE)
set(LINX_IS_DESKTOP FALSE)

# Function to detect platform
function(detect_linx_platform)
    # Skip if already detected
    if(LINX_PLATFORM_DETECTED)
        message(STATUS "Platform already detected: ${LINX_PLATFORM_NAME}")
        return()
    endif()
    
    # Initialize variables
    set(platform_name "unknown")
    set(platform_type "unknown")
    set(is_desktop FALSE)
    set(is_embedded FALSE)
    
    # Check if platform is explicitly set via command line
    if(DEFINED LINX_TARGET_PLATFORM AND NOT LINX_TARGET_PLATFORM STREQUAL "")
        set(platform_name ${LINX_TARGET_PLATFORM})
        message(STATUS "Platform explicitly set to: ${LINX_TARGET_PLATFORM}")
        
        # Set platform type based on explicit setting
        if(LINX_TARGET_PLATFORM MATCHES "^(esp32|esp8266|stm32|arduino|rpi_pico|embedded_linux|arm_embedded|generic_embedded)$")
            set(platform_type "embedded")
            set(is_embedded TRUE)
            set(is_desktop FALSE)
        elseif(LINX_TARGET_PLATFORM MATCHES "^(macos|linux|windows)$")
            set(platform_type "desktop")
            set(is_desktop TRUE)
            set(is_embedded FALSE)
        else()
            message(WARNING "Unknown explicit platform: ${LINX_TARGET_PLATFORM}")
        endif()
    else()
        # Auto-detect platform based on CMAKE variables
        message(STATUS "Auto-detecting platform...")
        message(STATUS "CMAKE_SYSTEM_NAME = '${CMAKE_SYSTEM_NAME}'")
        message(STATUS "CMAKE_SYSTEM_PROCESSOR = '${CMAKE_SYSTEM_PROCESSOR}'")
        message(STATUS "CMAKE_CROSSCOMPILING = '${CMAKE_CROSSCOMPILING}'")
        
        # Platform detection logic
        if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(platform_name "macos")
            set(platform_type "desktop")
            set(is_desktop TRUE)
            set(is_embedded FALSE)
            message(STATUS "Detected macOS platform")
            
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            # Check if it's a cross-compilation for embedded systems
            if(CMAKE_CROSSCOMPILING)
                if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)$")
                    set(platform_name "embedded_linux")
                    set(platform_type "embedded")
                    set(is_embedded TRUE)
                    set(is_desktop FALSE)
                    message(STATUS "Detected embedded Linux (ARM) platform")
                elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "xtensa")
                    set(platform_name "esp32")
                    set(platform_type "embedded")
                    set(is_embedded TRUE)
                    set(is_desktop FALSE)
                    message(STATUS "Detected ESP32 platform")
                else()
                    set(platform_name "linux")
                    set(platform_type "desktop")
                    set(is_desktop TRUE)
                    set(is_embedded FALSE)
                    message(STATUS "Detected Linux platform (cross-compiled)")
                endif()
            else()
                set(platform_name "linux")
                set(platform_type "desktop")
                set(is_desktop TRUE)
                set(is_embedded FALSE)
                message(STATUS "Detected native Linux platform")
            endif()
            
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set(platform_name "windows")
            set(platform_type "desktop")
            set(is_desktop TRUE)
            set(is_embedded FALSE)
            message(STATUS "Detected Windows platform")
            
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic")
            # This usually indicates embedded systems
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "xtensa")
                set(platform_name "esp32")
                set(platform_type "embedded")
                set(is_embedded TRUE)
                set(is_desktop FALSE)
                message(STATUS "Detected ESP32 embedded platform")
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|cortex)")
                set(platform_name "arm_embedded")
                set(platform_type "embedded")
                set(is_embedded TRUE)
                set(is_desktop FALSE)
                message(STATUS "Detected ARM embedded platform")
            else()
                set(platform_name "generic_embedded")
                set(platform_type "embedded")
                set(is_embedded TRUE)
                set(is_desktop FALSE)
                message(STATUS "Detected generic embedded platform")
            endif()
            
        else()
            # Fallback for unknown systems
            set(platform_name "unknown")
            set(platform_type "unknown")
            set(is_desktop FALSE)
            set(is_embedded FALSE)
            message(WARNING "Unknown system: CMAKE_SYSTEM_NAME='${CMAKE_SYSTEM_NAME}', CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}'")
        endif()
    endif()

    # Set all variables in parent scope
    set(LINX_PLATFORM_NAME ${platform_name} PARENT_SCOPE)
    set(LINX_PLATFORM_TYPE ${platform_type} PARENT_SCOPE)
    set(LINX_IS_DESKTOP ${is_desktop} PARENT_SCOPE)
    set(LINX_IS_EMBEDDED ${is_embedded} PARENT_SCOPE)
    set(LINX_TARGET_PLATFORM ${platform_name} PARENT_SCOPE)
    set(LINX_PLATFORM_DETECTED TRUE PARENT_SCOPE)

    # Final status message
    message(STATUS "Platform detection complete:")
    message(STATUS "  Platform Name: ${platform_name}")
    message(STATUS "  Platform Type: ${platform_type}")
    message(STATUS "  Is Desktop: ${is_desktop}")
    message(STATUS "  Is Embedded: ${is_embedded}")
    
endfunction()

# Function to print platform information
function(print_platform_info)
    message(STATUS "=== Linx SDK Platform Information ===")
    message(STATUS "Platform Name: ${LINX_PLATFORM_NAME}")
    message(STATUS "Platform Type: ${LINX_PLATFORM_TYPE}")
    message(STATUS "Is Embedded: ${LINX_IS_EMBEDDED}")
    message(STATUS "Is Desktop: ${LINX_IS_DESKTOP}")
    message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "System Processor: ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "Cross Compiling: ${CMAKE_CROSSCOMPILING}")
    message(STATUS "=====================================")
endfunction()

# Auto-detect platform when this module is included
detect_linx_platform()