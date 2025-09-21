# Windows Platform Configuration for Linx SDK

message(STATUS "Configuring for Windows platform")

# Set Windows specific flags
set(LINX_PLATFORM_WINDOWS TRUE)

# Windows specific definitions
add_definitions(-DLINX_PLATFORM_WINDOWS=1)
add_definitions(-DLINX_PLATFORM_DESKTOP=1)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-DWIN32_LEAN_AND_MEAN)

# Windows specific compiler flags
if(MSVC)
    # MSVC specific flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    
    # Enable parallel compilation
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    
    # Runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # MinGW or other compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Function to find vcpkg
function(find_vcpkg)
    # Check environment variable
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_ROOT $ENV{VCPKG_ROOT})
        message(STATUS "Found vcpkg via VCPKG_ROOT: ${VCPKG_ROOT}")
    else()
        # Try common locations
        set(VCPKG_PATHS
            "C:/vcpkg"
            "C:/tools/vcpkg"
            "C:/dev/vcpkg"
            "${CMAKE_SOURCE_DIR}/vcpkg"
            "${CMAKE_SOURCE_DIR}/../vcpkg"
        )
        
        foreach(path ${VCPKG_PATHS})
            if(EXISTS "${path}/vcpkg.exe")
                set(VCPKG_ROOT ${path})
                message(STATUS "Found vcpkg at: ${path}")
                break()
            endif()
        endforeach()
    endif()
    
    if(VCPKG_ROOT AND EXISTS "${VCPKG_ROOT}/vcpkg.exe")
        set(LINX_VCPKG_ROOT ${VCPKG_ROOT} PARENT_SCOPE)
        set(LINX_VCPKG_FOUND TRUE PARENT_SCOPE)
        
        # Set vcpkg toolchain
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" PARENT_SCOPE)
        message(STATUS "Using vcpkg toolchain: ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    else()
        set(LINX_VCPKG_FOUND FALSE PARENT_SCOPE)
        message(WARNING "vcpkg not found. Manual dependency management required.")
    endif()
endfunction()

# Function to check Windows dependencies
function(check_windows_dependencies)
    message(STATUS "Checking Windows dependencies...")
    
    if(LINX_VCPKG_FOUND)
        # List of required vcpkg packages
        set(REQUIRED_VCPKG_PACKAGES
            "portaudio"
            "opus"
            "cjson"
        )
        
        # Check each package
        set(MISSING_PACKAGES "")
        foreach(package ${REQUIRED_VCPKG_PACKAGES})
            # Check if package is installed
            execute_process(
                COMMAND "${LINX_VCPKG_ROOT}/vcpkg.exe" list ${package}
                OUTPUT_VARIABLE VCPKG_LIST_OUTPUT
                ERROR_QUIET
            )
            
            if(VCPKG_LIST_OUTPUT MATCHES "${package}:")
                message(STATUS "✓ ${package} is installed via vcpkg")
            else()
                message(STATUS "✗ ${package} is not installed via vcpkg")
                list(APPEND MISSING_PACKAGES ${package})
            endif()
        endforeach()
        
        # Report missing packages
        if(MISSING_PACKAGES)
            message(STATUS "Missing vcpkg packages: ${MISSING_PACKAGES}")
            message(STATUS "To install missing packages, run:")
            
            # Determine architecture
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(VCPKG_TRIPLET "x64-windows")
            else()
                set(VCPKG_TRIPLET "x86-windows")
            endif()
            
            foreach(package ${MISSING_PACKAGES})
                message(STATUS "  vcpkg install ${package}:${VCPKG_TRIPLET}")
            endforeach()
            
            set(LINX_MISSING_DEPENDENCIES TRUE PARENT_SCOPE)
        else()
            message(STATUS "All required vcpkg packages are installed")
            set(LINX_MISSING_DEPENDENCIES FALSE PARENT_SCOPE)
        endif()
    else()
        message(WARNING "vcpkg not found. Please install dependencies manually or install vcpkg.")
        set(LINX_MISSING_DEPENDENCIES TRUE PARENT_SCOPE)
    endif()
endfunction()

# Function to find Windows-specific libraries
function(find_windows_libraries)
    # Find Windows system libraries
    set(WINDOWS_SYSTEM_LIBS
        ws2_32      # Winsock
        winmm       # Windows Multimedia
        ole32       # OLE
        oleaut32    # OLE Automation
        uuid        # UUID
        advapi32    # Advanced API
    )
    
    set(LINX_SYSTEM_LIBRARIES ${WINDOWS_SYSTEM_LIBS} PARENT_SCOPE)
    message(STATUS "Windows system libraries: ${WINDOWS_SYSTEM_LIBS}")
    
    # Try to find libraries via vcpkg or system
    if(LINX_VCPKG_FOUND)
        # vcpkg should handle library finding automatically
        find_package(PkgConfig QUIET)
        
        # Find PortAudio
        find_package(portaudio CONFIG QUIET)
        if(portaudio_FOUND)
            set(LINX_PORTAUDIO_FOUND TRUE PARENT_SCOPE)
            set(LINX_PORTAUDIO_LIBRARIES portaudio PARENT_SCOPE)
            message(STATUS "Found PortAudio via vcpkg")
        endif()
        
        # Find Opus
        find_package(Opus CONFIG QUIET)
        if(Opus_FOUND)
            set(LINX_OPUS_FOUND TRUE PARENT_SCOPE)
            set(LINX_OPUS_LIBRARIES Opus::opus PARENT_SCOPE)
            message(STATUS "Found Opus via vcpkg")
        endif()
    endif()
endfunction()

# Function to setup Windows build environment
function(setup_windows_build_env)
    # Set output directories
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin PARENT_SCOPE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib PARENT_SCOPE)
    
    # Set debug postfix
    set(CMAKE_DEBUG_POSTFIX "d" PARENT_SCOPE)
    
    # Enable folder organization in Visual Studio
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endfunction()

# Find vcpkg
find_vcpkg()

# Check dependencies
check_windows_dependencies()

# Find libraries
find_windows_libraries()

# Setup build environment
setup_windows_build_env()

# Set platform-specific build options
set(LINX_ENABLE_AUDIO TRUE)
set(LINX_ENABLE_WEBSOCKET TRUE)
set(LINX_ENABLE_MCP TRUE)

# Windows specific installation paths
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/LinxSDK" CACHE PATH "Installation prefix for Windows" FORCE)
endif()

message(STATUS "Windows platform configuration completed")