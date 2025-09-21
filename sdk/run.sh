#!/bin/bash

# LINX SDK Build Script
# This script clones and builds third-party dependencies (mongoose, opus) and builds the main project
# Usage: ./run.sh [--toolchain <toolchain_file>] [--help]
#   --toolchain <file>  : Use specified CMake toolchain file for cross-compilation
#   --help             : Show this help message

set -e  # Exit on any error

# Global variables
TOOLCHAIN_FILE=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_DIR="${SCRIPT_DIR}/third"
CMAKE_DIR="${SCRIPT_DIR}/cmake"

print_info "Starting LINX SDK build process..."
print_info "Script directory: ${SCRIPT_DIR}"
print_info "Third-party directory: ${THIRD_DIR}"

# Create third directory if it doesn't exist
mkdir -p "${THIRD_DIR}"

# Function to clone or update repository
clone_or_update_repo() {
    local repo_url="$1"
    local repo_name="$2"
    local repo_dir="${THIRD_DIR}/${repo_name}"
    
    # Check if directory exists
    if [ -d "${repo_dir}" ]; then
        print_info "Directory ${repo_name} already exists at ${repo_dir}"
        
        # Check if it's a valid git repository
        if [ -d "${repo_dir}/.git" ] || git -C "${repo_dir}" rev-parse --git-dir >/dev/null 2>&1; then
            print_info "Repository ${repo_name} is a valid git repository, updating..."
            cd "${repo_dir}"
            
            # Try to update from remote
            git pull origin main 2>/dev/null || git pull origin master 2>/dev/null || {
                print_warning "Failed to update ${repo_name} from remote, continuing with existing version"
            }
        else
            print_warning "Directory ${repo_dir} exists but is not a git repository"
            print_info "Removing existing directory and cloning fresh..."
            rm -rf "${repo_dir}" || {
                print_error "Failed to remove existing directory ${repo_dir}"
                exit 1
            }
            
            # Clone the repository
            print_info "Cloning ${repo_name} from ${repo_url}..."
            cd "${THIRD_DIR}"
            git clone "${repo_url}" "${repo_name}" || {
                print_error "Failed to clone ${repo_name}"
                exit 1
            }
        fi
    else
        print_info "Directory ${repo_dir} does not exist"
        print_info "Cloning ${repo_name} from ${repo_url}..."
        cd "${THIRD_DIR}"
        git clone "${repo_url}" "${repo_name}" || {
            print_error "Failed to clone ${repo_name}"
            exit 1
        }
    fi
}

# Function to build mongoose using CMake
build_mongoose() {
    print_info "Building mongoose with CMake..."
    local mongoose_dir="${THIRD_DIR}/mongoose"
    local install_dir="${mongoose_dir}/install"
    local patch_dir="${CMAKE_DIR}/patch/mongoose"
    
    if [ ! -d "${mongoose_dir}" ]; then
        print_error "Mongoose directory not found"
        exit 1
    fi
    
    # Check if mongoose.c and mongoose.h exist
    if [ ! -f "${mongoose_dir}/mongoose.c" ] || [ ! -f "${mongoose_dir}/mongoose.h" ]; then
        print_error "Mongoose source files not found"
        exit 1
    fi
    
    # Copy patch files to mongoose directory
    if [ -d "${patch_dir}" ]; then
        print_info "Copying mongoose patch files from ${patch_dir}..."
        cp -r "${patch_dir}"/* "${mongoose_dir}/" || {
            print_error "Failed to copy mongoose patch files"
            exit 1
        }
        print_success "Mongoose patch files copied successfully"
    else
        print_warning "Mongoose patch directory not found: ${patch_dir}"
    fi
    
    cd "${mongoose_dir}"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with CMake
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_INSTALL_PREFIX=${install_dir}"
    )
    
    # Add toolchain file if specified
    if [ -n "${TOOLCHAIN_FILE}" ]; then
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
        print_info "Using toolchain: ${TOOLCHAIN_FILE}"
    fi
    
    cmake .. "${cmake_args[@]}" || {
        print_error "Failed to configure mongoose with CMake"
        exit 1
    }
    
    # Build
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
        print_error "Failed to build mongoose"
        exit 1
    }
    
    # Install
    make install || {
        print_error "Failed to install mongoose"
        exit 1
    }
    
    print_success "Mongoose built successfully with CMake"
}

# Function to build opus using CMake
build_opus() {
    print_info "Building opus with CMake..."
    local opus_dir="${THIRD_DIR}/opus"
    local install_dir="${opus_dir}/install"
    
    if [ ! -d "${opus_dir}" ]; then
        print_error "Opus directory not found"
        exit 1
    fi
    
    # Create install directory and ensure it's writable
    print_info "Creating install directory: ${install_dir}"
    mkdir -p "${install_dir}" || {
        print_error "Failed to create install directory: ${install_dir}"
        exit 1
    }
    
    # Verify install directory is writable
    if [ ! -w "${install_dir}" ]; then
        print_error "Install directory is not writable: ${install_dir}"
        exit 1
    fi
    
    cd "${opus_dir}"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Get absolute path for install directory
    local abs_install_dir="$(cd "${install_dir}" && pwd)"
    print_info "Using absolute install path: ${abs_install_dir}"
    
    # Configure with CMake
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_INSTALL_PREFIX=${abs_install_dir}"
        "-DOPUS_BUILD_SHARED_LIBRARY=OFF"
        "-DOPUS_BUILD_PROGRAMS=OFF"
        "-DOPUS_BUILD_TESTING=OFF"
        "-DOPUS_STACK_PROTECTOR=OFF"
    )
    
    # Add toolchain file if specified
    if [ -n "${TOOLCHAIN_FILE}" ]; then
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
        print_info "Using toolchain: ${TOOLCHAIN_FILE}"
    fi
    
    print_info "Configuring opus with CMake..."
    cmake .. "${cmake_args[@]}" || {
        print_error "Failed to configure opus with CMake"
        exit 1
    }
    
    # Build
    print_info "Building opus..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
        print_error "Failed to build opus"
        exit 1
    }
    
    # Install
    print_info "Installing opus to ${abs_install_dir}..."
    make install || {
        print_error "Failed to install opus"
        exit 1
    }
    
    # Verify installation
    if [ -f "${abs_install_dir}/lib/libopus.a" ] || [ -f "${abs_install_dir}/lib/libopus.so" ]; then
        print_success "Opus built and installed successfully"
    else
        print_warning "Opus installation completed but library files not found in expected location"
        print_info "Checking install directory contents:"
        ls -la "${abs_install_dir}" || true
        if [ -d "${abs_install_dir}/lib" ]; then
            ls -la "${abs_install_dir}/lib" || true
        fi
    fi
}

# Function to build main project
build_main_project() {
    print_info "Building main LINX SDK project..."
    
    cd "${SCRIPT_DIR}"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Set up environment variables for finding third-party libraries
    export PKG_CONFIG_PATH="${THIRD_DIR}/opus/install/lib/pkgconfig:${PKG_CONFIG_PATH}"
    export CMAKE_PREFIX_PATH="${THIRD_DIR}/mongoose/install:${THIRD_DIR}/opus/install:${CMAKE_PREFIX_PATH}"
    
    # Configure with cmake
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=Release"
        "-DCMAKE_MODULE_PATH=${CMAKE_DIR}"
        "-DMongoose_ROOT=${THIRD_DIR}/mongoose/install"
        "-DOPUS_ROOT=${THIRD_DIR}/opus/install"
    )
    
    # Add toolchain file if specified
    if [ -n "${TOOLCHAIN_FILE}" ]; then
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
        print_info "Using toolchain for main project: ${TOOLCHAIN_FILE}"
    fi
    
    cmake .. "${cmake_args[@]}" || {
        print_error "Failed to configure main project"
        exit 1
    }
    
    # Build
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
        print_error "Failed to build main project"
        exit 1
    }
    make install || {
        print_error "Failed to install main project"
        exit 1
    }
    
    print_success "Main project built successfully"
}

# Function to show help
show_help() {
    echo "LINX SDK Build Script"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --toolchain <file>    Use specified CMake toolchain file for cross-compilation"
    echo "                        Available toolchains:"
    echo "                          ${CMAKE_DIR}/toolchains/arm-linux-gnueabihf.cmake"
    echo "                          ${CMAKE_DIR}/toolchains/esp32.cmake"
    echo "  --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                           # Build for host platform"
    echo "  $0 --toolchain cmake/toolchains/arm-linux-gnueabihf.cmake  # Cross-compile for ARM"
    echo ""
}

# Function to parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --toolchain)
                TOOLCHAIN_FILE="$2"
                if [ ! -f "${TOOLCHAIN_FILE}" ]; then
                    # Try relative to script directory
                    if [ -f "${SCRIPT_DIR}/${TOOLCHAIN_FILE}" ]; then
                        TOOLCHAIN_FILE="${SCRIPT_DIR}/${TOOLCHAIN_FILE}"
                    else
                        print_error "Toolchain file not found: ${TOOLCHAIN_FILE}"
                        exit 1
                    fi
                fi
                print_info "Using toolchain: ${TOOLCHAIN_FILE}"
                shift 2
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Main execution flow
main() {
    # Parse command line arguments
    parse_arguments "$@"
    
    print_info "=== LINX SDK Build Process Started ==="
    
    # Clone/update repositories
    print_info "=== Step 1: Cloning/Updating Dependencies ==="
    clone_or_update_repo "https://github.com/cesanta/mongoose.git" "mongoose"
    clone_or_update_repo "https://github.com/xiph/opus.git" "opus"
    
    # Build dependencies
    print_info "=== Step 2: Building Dependencies ==="
    build_mongoose
    build_opus
    
    # Build main project
    print_info "=== Step 3: Building Main Project ==="
    build_main_project
    
    print_success "=== LINX SDK Build Process Completed Successfully ==="
    print_info "Build artifacts are located in: ${SCRIPT_DIR}/build"
}

# Check if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi