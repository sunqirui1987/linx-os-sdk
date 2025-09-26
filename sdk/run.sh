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
    local tag="$3"  # Optional tag parameter
    local repo_dir="${THIRD_DIR}/${repo_name}"
    
    # Check if directory exists
    if [ -d "${repo_dir}" ]; then
        print_info "Directory ${repo_name} already exists at ${repo_dir}"
        
        # Check if it's a valid git repository
        if [ -d "${repo_dir}/.git" ] || git -C "${repo_dir}" rev-parse --git-dir >/dev/null 2>&1; then
            print_info "Repository ${repo_name} is a valid git repository, updating..."
            cd "${repo_dir}"
            
            # Fetch latest tags and branches
            git fetch --tags origin || {
                print_warning "Failed to fetch from remote for ${repo_name}"
            }
            
            # If tag is specified, checkout the tag
            if [ -n "${tag}" ]; then
                print_info "Checking out tag ${tag} for ${repo_name}..."
                git checkout "tags/${tag}" || {
                    print_error "Failed to checkout tag ${tag} for ${repo_name}"
                    exit 1
                }
            else
                # Try to update from remote
                git pull origin main 2>/dev/null || git pull origin master 2>/dev/null || {
                    print_warning "Failed to update ${repo_name} from remote, continuing with existing version"
                }
            fi
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
            
            # If tag is specified, checkout the tag after cloning
            if [ -n "${tag}" ]; then
                cd "${repo_dir}"
                print_info "Checking out tag ${tag} for ${repo_name}..."
                git checkout "tags/${tag}" || {
                    print_error "Failed to checkout tag ${tag} for ${repo_name}"
                    exit 1
                }
            fi
        fi
    else
        print_info "Directory ${repo_dir} does not exist"
        print_info "Cloning ${repo_name} from ${repo_url}..."
        cd "${THIRD_DIR}"
        git clone "${repo_url}" "${repo_name}" || {
            print_error "Failed to clone ${repo_name}"
            exit 1
        }
        
        # If tag is specified, checkout the tag after cloning
        if [ -n "${tag}" ]; then
            cd "${repo_dir}"
            print_info "Checking out tag ${tag} for ${repo_name}..."
            git checkout "tags/${tag}" || {
                print_error "Failed to checkout tag ${tag} for ${repo_name}"
                exit 1
            }
        fi
    fi
}

# Function to build mongoose using CMake
copy_mongoose() {
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
    
}

# Main execution flow
main() {

    print_info "=== LINX SDK Build Process Started ==="
    
    # Clone/update repositories
    print_info "=== Step 1: Cloning/Updating Dependencies ==="
    clone_or_update_repo "https://github.com/cesanta/mongoose.git" "mongoose" "7.19"
    clone_or_update_repo "https://github.com/xiph/opus.git" "opus" "v1.5.2"
    
    # Build dependencies
    print_info "=== Step 2: Building Dependencies ==="
    copy_mongoose
  
    print_success "=== LINX SDK Build Process Completed Successfully ==="
    print_info "Build artifacts are located in: ${SCRIPT_DIR}/build"
}

