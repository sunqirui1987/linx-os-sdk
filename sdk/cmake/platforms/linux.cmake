# Linux Platform Configuration for Linx SDK

message(STATUS "Configuring for Linux platform")

# Set Linux specific flags
set(LINX_PLATFORM_LINUX TRUE)

# Linux specific definitions
add_definitions(-DLINX_PLATFORM_LINUX=1)
add_definitions(-DLINX_PLATFORM_DESKTOP=1)

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Linux specific compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Function to detect Linux distribution
function(detect_linux_distro)
    # Try to read /etc/os-release
    if(EXISTS "/etc/os-release")
        file(READ "/etc/os-release" OS_RELEASE_CONTENT)
        
        # Extract distribution ID
        string(REGEX MATCH "ID=([^\n]*)" _ ${OS_RELEASE_CONTENT})
        if(CMAKE_MATCH_1)
            string(REPLACE "\"" "" DISTRO_ID ${CMAKE_MATCH_1})
            set(LINX_LINUX_DISTRO ${DISTRO_ID} PARENT_SCOPE)
            message(STATUS "Detected Linux distribution: ${DISTRO_ID}")
        endif()
        
        # Extract version
        string(REGEX MATCH "VERSION_ID=\"([^\"]*)" _ ${OS_RELEASE_CONTENT})
        if(CMAKE_MATCH_1)
            set(LINX_LINUX_VERSION ${CMAKE_MATCH_1} PARENT_SCOPE)
            message(STATUS "Distribution version: ${CMAKE_MATCH_1}")
        endif()
    else()
        # Fallback detection methods
        if(EXISTS "/etc/debian_version")
            set(LINX_LINUX_DISTRO "debian" PARENT_SCOPE)
        elseif(EXISTS "/etc/redhat-release")
            set(LINX_LINUX_DISTRO "redhat" PARENT_SCOPE)
        elseif(EXISTS "/etc/arch-release")
            set(LINX_LINUX_DISTRO "arch" PARENT_SCOPE)
        else()
            set(LINX_LINUX_DISTRO "unknown" PARENT_SCOPE)
        endif()
        message(STATUS "Detected Linux distribution: ${LINX_LINUX_DISTRO}")
    endif()
endfunction()

# Function to get package manager
function(get_package_manager)
    if(LINX_LINUX_DISTRO MATCHES "ubuntu|debian")
        set(LINX_PACKAGE_MANAGER "apt" PARENT_SCOPE)
        set(LINX_PACKAGE_INSTALL_CMD "sudo apt-get install -y" PARENT_SCOPE)
    elseif(LINX_LINUX_DISTRO MATCHES "fedora|rhel|centos")
        # Check for dnf first, then yum
        find_program(DNF_EXECUTABLE dnf)
        if(DNF_EXECUTABLE)
            set(LINX_PACKAGE_MANAGER "dnf" PARENT_SCOPE)
            set(LINX_PACKAGE_INSTALL_CMD "sudo dnf install -y" PARENT_SCOPE)
        else()
            set(LINX_PACKAGE_MANAGER "yum" PARENT_SCOPE)
            set(LINX_PACKAGE_INSTALL_CMD "sudo yum install -y" PARENT_SCOPE)
        endif()
    elseif(LINX_LINUX_DISTRO MATCHES "arch|manjaro")
        set(LINX_PACKAGE_MANAGER "pacman" PARENT_SCOPE)
        set(LINX_PACKAGE_INSTALL_CMD "sudo pacman -S --noconfirm" PARENT_SCOPE)
    elseif(LINX_LINUX_DISTRO MATCHES "opensuse")
        set(LINX_PACKAGE_MANAGER "zypper" PARENT_SCOPE)
        set(LINX_PACKAGE_INSTALL_CMD "sudo zypper install -y" PARENT_SCOPE)
    else()
        set(LINX_PACKAGE_MANAGER "unknown" PARENT_SCOPE)
        set(LINX_PACKAGE_INSTALL_CMD "echo 'Unknown package manager'" PARENT_SCOPE)
    endif()
    
    message(STATUS "Package manager: ${LINX_PACKAGE_MANAGER}")
endfunction()

# Function to check Linux dependencies
function(check_linux_dependencies)
    message(STATUS "Checking Linux dependencies...")
    
    # Define package names for different distributions
    if(LINX_LINUX_DISTRO MATCHES "ubuntu|debian")
        set(REQUIRED_PACKAGES
            "build-essential"
            "cmake"
            "pkg-config"
            "libportaudio2"
            "libportaudio-dev"
            "libopus0"
            "libopus-dev"
            "git"
        )
    elseif(LINX_LINUX_DISTRO MATCHES "fedora|rhel|centos")
        set(REQUIRED_PACKAGES
            "gcc"
            "gcc-c++"
            "cmake"
            "pkgconfig"
            "portaudio-devel"
            "opus-devel"
            "git"
        )
    elseif(LINX_LINUX_DISTRO MATCHES "arch|manjaro")
        set(REQUIRED_PACKAGES
            "base-devel"
            "cmake"
            "pkgconf"
            "portaudio"
            "opus"
            "git"
        )
    else()
        set(REQUIRED_PACKAGES
            "cmake"
            "pkg-config"
            "portaudio"
            "opus"
            "git"
        )
    endif()
    
    # Check for pkg-config first
    find_package(PkgConfig QUIET)
    if(NOT PkgConfig_FOUND)
        message(WARNING "pkg-config not found. Some dependencies may not be detected properly.")
        set(LINX_MISSING_DEPENDENCIES TRUE PARENT_SCOPE)
        return()
    endif()
    
    # Check each dependency
    set(MISSING_PACKAGES "")
    
    # Check PortAudio
    pkg_check_modules(PORTAUDIO portaudio-2.0)
    if(NOT PORTAUDIO_FOUND)
        list(APPEND MISSING_PACKAGES "portaudio")
    else()
        message(STATUS "✓ PortAudio found")
        set(LINX_PORTAUDIO_FOUND TRUE PARENT_SCOPE)
        set(LINX_PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES} PARENT_SCOPE)
        set(LINX_PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIRS} PARENT_SCOPE)
    endif()
    
    # Check Opus
    pkg_check_modules(OPUS opus)
    if(NOT OPUS_FOUND)
        list(APPEND MISSING_PACKAGES "opus")
    else()
        message(STATUS "✓ Opus found")
        set(LINX_OPUS_FOUND TRUE PARENT_SCOPE)
        set(LINX_OPUS_LIBRARIES ${OPUS_LIBRARIES} PARENT_SCOPE)
        set(LINX_OPUS_INCLUDE_DIRS ${OPUS_INCLUDE_DIRS} PARENT_SCOPE)
    endif()
    
    # Report missing packages
    if(MISSING_PACKAGES)
        message(STATUS "Missing packages: ${MISSING_PACKAGES}")
        message(STATUS "To install missing packages, run:")
        
        if(LINX_LINUX_DISTRO MATCHES "ubuntu|debian")
            message(STATUS "  sudo apt-get update")
            message(STATUS "  ${LINX_PACKAGE_INSTALL_CMD} ${REQUIRED_PACKAGES}")
        elseif(LINX_LINUX_DISTRO MATCHES "fedora|rhel|centos")
            message(STATUS "  ${LINX_PACKAGE_INSTALL_CMD} ${REQUIRED_PACKAGES}")
        elseif(LINX_LINUX_DISTRO MATCHES "arch|manjaro")
            message(STATUS "  ${LINX_PACKAGE_INSTALL_CMD} ${REQUIRED_PACKAGES}")
        else()
            message(STATUS "  Install packages using your distribution's package manager")
        endif()
        
        set(LINX_MISSING_DEPENDENCIES TRUE PARENT_SCOPE)
    else()
        message(STATUS "All required dependencies are available")
        set(LINX_MISSING_DEPENDENCIES FALSE PARENT_SCOPE)
    endif()
endfunction()

# Function to find Linux-specific libraries
function(find_linux_libraries)
    # Find threading library
    find_package(Threads REQUIRED)
    set(LINX_SYSTEM_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)
    
    # Find math library
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        list(APPEND LINX_SYSTEM_LIBRARIES ${MATH_LIBRARY})
        set(LINX_SYSTEM_LIBRARIES ${LINX_SYSTEM_LIBRARIES} PARENT_SCOPE)
    endif()
    
    # Find dl library for dynamic loading
    find_library(DL_LIBRARY dl)
    if(DL_LIBRARY)
        list(APPEND LINX_SYSTEM_LIBRARIES ${DL_LIBRARY})
        set(LINX_SYSTEM_LIBRARIES ${LINX_SYSTEM_LIBRARIES} PARENT_SCOPE)
    endif()
    
    message(STATUS "System libraries: ${LINX_SYSTEM_LIBRARIES}")
endfunction()

# Detect distribution
detect_linux_distro()

# Get package manager
get_package_manager()

# Check dependencies
check_linux_dependencies()

# Find libraries
find_linux_libraries()

# Set platform-specific build options
set(LINX_ENABLE_AUDIO TRUE)
set(LINX_ENABLE_WEBSOCKET TRUE)
set(LINX_ENABLE_MCP TRUE)

# Linux specific installation paths
set(LINX_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix for Linux")
set(CMAKE_INSTALL_PREFIX ${LINX_INSTALL_PREFIX})

message(STATUS "Linux platform configuration completed")