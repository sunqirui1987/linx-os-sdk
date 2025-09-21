# macOS Platform Configuration for Linx SDK

message(STATUS "Configuring for macOS platform")

# Set macOS specific flags
set(LINX_PLATFORM_MACOS TRUE)

# macOS specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.12")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.12")

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# macOS specific definitions
add_definitions(-DLINX_PLATFORM_MACOS=1)
add_definitions(-DLINX_PLATFORM_DESKTOP=1)

# Function to find Homebrew prefix
function(find_homebrew_prefix)
    # Try common Homebrew locations
    set(HOMEBREW_PATHS
        "/opt/homebrew"      # Apple Silicon Macs
        "/usr/local"         # Intel Macs
    )
    
    foreach(path ${HOMEBREW_PATHS})
        if(EXISTS "${path}/bin/brew")
            set(HOMEBREW_PREFIX ${path} PARENT_SCOPE)
            message(STATUS "Found Homebrew at: ${path}")
            return()
        endif()
    endforeach()
    
    # Try to get from brew command
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE HOMEBREW_PREFIX_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(HOMEBREW_PREFIX_OUTPUT)
        set(HOMEBREW_PREFIX ${HOMEBREW_PREFIX_OUTPUT} PARENT_SCOPE)
        message(STATUS "Found Homebrew via command at: ${HOMEBREW_PREFIX_OUTPUT}")
    else()
        message(WARNING "Homebrew not found. Some dependencies may not be available.")
        set(HOMEBREW_PREFIX "" PARENT_SCOPE)
    endif()
endfunction()

# Find Homebrew
find_homebrew_prefix()

# Set PKG_CONFIG_PATH to include Homebrew
if(HOMEBREW_PREFIX)
    set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    
    # Add Homebrew paths to CMAKE_PREFIX_PATH
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}/lib")
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}/include")
    
    # Set library and include paths
    set(LINX_HOMEBREW_LIB_PATH "${HOMEBREW_PREFIX}/lib")
    set(LINX_HOMEBREW_INCLUDE_PATH "${HOMEBREW_PREFIX}/include")
endif()

# Function to check and install macOS dependencies
function(check_macos_dependencies)
    message(STATUS "Checking macOS dependencies...")
    
    # Check for required tools
    find_program(BREW_EXECUTABLE brew)
    if(NOT BREW_EXECUTABLE)
        message(FATAL_ERROR "Homebrew is required but not found. Please install Homebrew first.")
    endif()
    
    # List of required packages
    set(REQUIRED_PACKAGES
        "cmake"
        "pkg-config"
        "portaudio"
        "opus"
    )
    
    # Check each package
    foreach(package ${REQUIRED_PACKAGES})
        execute_process(
            COMMAND ${BREW_EXECUTABLE} list ${package}
            RESULT_VARIABLE PACKAGE_CHECK_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )
        
        if(PACKAGE_CHECK_RESULT EQUAL 0)
            message(STATUS "✓ ${package} is installed")
        else()
            message(STATUS "✗ ${package} is not installed")
            set(MISSING_PACKAGES "${MISSING_PACKAGES} ${package}")
        endif()
    endforeach()
    
    # Report missing packages
    if(MISSING_PACKAGES)
        message(STATUS "Missing packages:${MISSING_PACKAGES}")
        message(STATUS "To install missing packages, run:")
        message(STATUS "  brew install${MISSING_PACKAGES}")
        set(LINX_MISSING_DEPENDENCIES TRUE PARENT_SCOPE)
    else()
        message(STATUS "All required dependencies are installed")
        set(LINX_MISSING_DEPENDENCIES FALSE PARENT_SCOPE)
    endif()
endfunction()

# Function to find macOS-specific libraries
function(find_macos_libraries)
    # Find PortAudio
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PORTAUDIO portaudio-2.0)
        if(PORTAUDIO_FOUND)
            set(LINX_PORTAUDIO_FOUND TRUE PARENT_SCOPE)
            set(LINX_PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES} PARENT_SCOPE)
            set(LINX_PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIRS} PARENT_SCOPE)
            message(STATUS "Found PortAudio via pkg-config")
        endif()
    endif()
    
    # Find Opus
    if(PkgConfig_FOUND)
        pkg_check_modules(OPUS opus)
        if(OPUS_FOUND)
            set(LINX_OPUS_FOUND TRUE PARENT_SCOPE)
            set(LINX_OPUS_LIBRARIES ${OPUS_LIBRARIES} PARENT_SCOPE)
            set(LINX_OPUS_INCLUDE_DIRS ${OPUS_INCLUDE_DIRS} PARENT_SCOPE)
            message(STATUS "Found Opus via pkg-config")
        endif()
    endif()
    
    # Find system frameworks
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
    
    if(COREAUDIO_FRAMEWORK AND AUDIOUNIT_FRAMEWORK AND AUDIOTOOLBOX_FRAMEWORK)
        set(LINX_MACOS_FRAMEWORKS 
            ${COREAUDIO_FRAMEWORK}
            ${AUDIOUNIT_FRAMEWORK}
            ${AUDIOTOOLBOX_FRAMEWORK}
            PARENT_SCOPE
        )
        message(STATUS "Found macOS audio frameworks")
    endif()
endfunction()

# Check dependencies
check_macos_dependencies()

# Find libraries
find_macos_libraries()

# Set platform-specific build options
set(LINX_ENABLE_AUDIO TRUE)
set(LINX_ENABLE_WEBSOCKET TRUE)
set(LINX_ENABLE_MCP TRUE)

# macOS specific installation paths
set(LINX_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix for macOS")
set(CMAKE_INSTALL_PREFIX ${LINX_INSTALL_PREFIX})

message(STATUS "macOS platform configuration completed")