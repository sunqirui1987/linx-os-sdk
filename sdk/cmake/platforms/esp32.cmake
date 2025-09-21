# ESP32 Platform Configuration for Linx SDK

message(STATUS "Configuring for ESP32 platform")

# Set ESP32 specific flags
set(LINX_PLATFORM_ESP32 TRUE)
set(LINX_PLATFORM_EMBEDDED TRUE)

# ESP32 specific definitions
add_definitions(-DLINX_PLATFORM_ESP32=1)
add_definitions(-DLINX_PLATFORM_EMBEDDED=1)

# ESP32 memory constraints
add_definitions(-DLINX_MEMORY_CONSTRAINED=1)

# Disable features not suitable for ESP32
set(LINX_ENABLE_AUDIO FALSE)  # Limited audio capabilities
set(LINX_ENABLE_WEBSOCKET TRUE)  # ESP32 supports WebSocket
set(LINX_ENABLE_MCP TRUE)  # MCP can work on ESP32

# ESP32 specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlongcalls")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mlongcalls")

# Function to check ESP-IDF environment
function(check_esp_idf_environment)
    message(STATUS "Checking ESP-IDF environment...")
    
    # Check for IDF_PATH
    if(NOT DEFINED ENV{IDF_PATH})
        message(FATAL_ERROR "IDF_PATH environment variable is not set. Please source ESP-IDF environment.")
    endif()
    
    set(IDF_PATH $ENV{IDF_PATH})
    message(STATUS "ESP-IDF path: ${IDF_PATH}")
    
    # Check if ESP-IDF exists
    if(NOT EXISTS "${IDF_PATH}/components")
        message(FATAL_ERROR "ESP-IDF not found at ${IDF_PATH}. Please check your ESP-IDF installation.")
    endif()
    
    # Check for ESP-IDF version
    if(EXISTS "${IDF_PATH}/version.txt")
        file(READ "${IDF_PATH}/version.txt" IDF_VERSION)
        string(STRIP "${IDF_VERSION}" IDF_VERSION)
        message(STATUS "ESP-IDF version: ${IDF_VERSION}")
    endif()
    
    # Set ESP-IDF related variables
    set(LINX_ESP_IDF_PATH ${IDF_PATH} PARENT_SCOPE)
    set(LINX_ESP_IDF_FOUND TRUE PARENT_SCOPE)
endfunction()

# Function to setup ESP32 toolchain
function(setup_esp32_toolchain)
    if(NOT LINX_ESP_IDF_FOUND)
        message(FATAL_ERROR "ESP-IDF not found. Cannot setup ESP32 toolchain.")
    endif()
    
    # Set target chip
    if(NOT DEFINED IDF_TARGET)
        set(IDF_TARGET "esp32" CACHE STRING "ESP32 target chip")
    endif()
    
    message(STATUS "ESP32 target: ${IDF_TARGET}")
    
    # Include ESP-IDF build system
    if(EXISTS "${LINX_ESP_IDF_PATH}/tools/cmake/idf.cmake")
        include("${LINX_ESP_IDF_PATH}/tools/cmake/idf.cmake")
    else()
        message(FATAL_ERROR "ESP-IDF CMake build system not found")
    endif()
endfunction()

# Function to configure ESP32 components
function(configure_esp32_components)
    message(STATUS "Configuring ESP32 components...")
    
    # Required ESP-IDF components
    set(LINX_ESP32_COMPONENTS
        "freertos"
        "esp_wifi"
        "esp_http_client"
        "esp_websocket_client"
        "nvs_flash"
        "esp_netif"
        "esp_event"
        "tcp_transport"
        "json"
        "log"
    )
    
    # Optional components based on features
    if(LINX_ENABLE_AUDIO)
        list(APPEND LINX_ESP32_COMPONENTS "esp_adc_cal" "driver")
    endif()
    
    set(LINX_ESP32_REQUIRED_COMPONENTS ${LINX_ESP32_COMPONENTS} PARENT_SCOPE)
    message(STATUS "Required ESP32 components: ${LINX_ESP32_COMPONENTS}")
endfunction()

# Function to set ESP32 memory optimization
function(set_esp32_memory_optimization)
    message(STATUS "Setting ESP32 memory optimizations...")
    
    # Optimize for size
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
    
    # Remove debug symbols in release builds
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    
    # Enable link time optimization
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    
    # Set stack size limits
    add_definitions(-DLINX_MAX_STACK_SIZE=4096)
    add_definitions(-DLINX_MAX_HEAP_SIZE=32768)
endfunction()

# Function to check ESP32 dependencies
function(check_esp32_dependencies)
    message(STATUS "Checking ESP32 dependencies...")
    
    # Check for required tools
    find_program(ESPTOOL_EXECUTABLE esptool.py)
    if(ESPTOOL_EXECUTABLE)
        message(STATUS "✓ esptool.py found")
    else()
        message(WARNING "✗ esptool.py not found in PATH")
    endif()
    
    # Check for xtensa toolchain
    find_program(XTENSA_GCC xtensa-esp32-elf-gcc)
    if(XTENSA_GCC)
        message(STATUS "✓ Xtensa toolchain found")
    else()
        message(WARNING "✗ Xtensa toolchain not found")
    endif()
endfunction()

# Check ESP-IDF environment
check_esp_idf_environment()

# Check dependencies
check_esp32_dependencies()

# Configure components
configure_esp32_components()

# Set memory optimizations
set_esp32_memory_optimization()

# ESP32 specific build settings
set(LINX_ENABLE_TESTS FALSE)  # Disable tests on ESP32
set(LINX_ENABLE_EXAMPLES FALSE)  # Disable examples on ESP32

# ESP32 partition and flash settings
set(LINX_ESP32_PARTITION_TABLE "partitions.csv")
set(LINX_ESP32_FLASH_SIZE "4MB")
set(LINX_ESP32_FLASH_MODE "dio")
set(LINX_ESP32_FLASH_FREQ "40m")

message(STATUS "ESP32 platform configuration completed")