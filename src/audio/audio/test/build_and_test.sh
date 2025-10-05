#!/bin/bash

# LINX 音频测试构建和运行脚本

set -e  # 遇到错误时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
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

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

print_info "LINX 音频测试构建脚本"
print_info "脚本目录: ${SCRIPT_DIR}"
print_info "构建目录: ${BUILD_DIR}"

# 检查是否安装了必要的工具
check_dependencies() {
    print_info "检查依赖..."
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    if ! command -v make &> /dev/null; then
        print_error "Make 未安装，请先安装 Make"
        exit 1
    fi
    
    # 检查PortAudio
    if pkg-config --exists portaudio-2.0; then
        PORTAUDIO_VERSION=$(pkg-config --modversion portaudio-2.0)
        print_success "找到 PortAudio 版本: ${PORTAUDIO_VERSION}"
    else
        print_warning "未找到 PortAudio，将使用桩实现"
        print_info "要安装 PortAudio，请运行: brew install portaudio"
    fi
}

# 创建构建目录
setup_build_dir() {
    print_info "设置构建目录..."
    
    if [ -d "${BUILD_DIR}" ]; then
        print_info "清理现有构建目录..."
        rm -rf "${BUILD_DIR}"
    fi
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
}

# 配置项目
configure_project() {
    print_info "配置项目..."
    
    cmake "${SCRIPT_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if [ $? -eq 0 ]; then
        print_success "项目配置成功"
    else
        print_error "项目配置失败"
        exit 1
    fi
}

# 编译项目
build_project() {
    print_info "编译项目..."
    
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ $? -eq 0 ]; then
        print_success "编译成功"
    else
        print_error "编译失败"
        exit 1
    fi
}

# 运行测试
run_tests() {
    print_info "运行基本测试..."
    
    if [ -f "${BUILD_DIR}/bin/audio_test" ]; then
        "${BUILD_DIR}/bin/audio_test"
        
        if [ $? -eq 0 ]; then
            print_success "基本测试通过"
        else
            print_error "基本测试失败"
            exit 1
        fi
    else
        print_error "测试可执行文件不存在"
        exit 1
    fi
}

# 运行交互式测试
run_interactive_test() {
    print_info "运行交互式测试..."
    print_warning "请确保已连接麦克风和扬声器/耳机"
    
    read -p "按回车键继续运行交互式测试，或按 Ctrl+C 跳过..."
    
    "${BUILD_DIR}/bin/audio_test" --interactive
    
    if [ $? -eq 0 ]; then
        print_success "交互式测试完成"
    else
        print_warning "交互式测试被中断或失败"
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -c, --clean         清理构建目录"
    echo "  -b, --build-only    仅编译，不运行测试"
    echo "  -i, --interactive   运行交互式测试"
    echo "  -t, --test-only     仅运行测试（假设已编译）"
    echo ""
    echo "示例:"
    echo "  $0                  完整构建和测试流程"
    echo "  $0 --build-only     仅编译"
    echo "  $0 --interactive    编译并运行交互式测试"
    echo "  $0 --clean          清理构建目录"
}

# 清理构建目录
clean_build() {
    print_info "清理构建目录..."
    
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        print_success "构建目录已清理"
    else
        print_info "构建目录不存在，无需清理"
    fi
}

# 主函数
main() {
    local build_only=false
    local test_only=false
    local interactive=false
    local clean_only=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_only=true
                shift
                ;;
            -b|--build-only)
                build_only=true
                shift
                ;;
            -i|--interactive)
                interactive=true
                shift
                ;;
            -t|--test-only)
                test_only=true
                shift
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 执行相应操作
    if [ "$clean_only" = true ]; then
        clean_build
        exit 0
    fi
    
    if [ "$test_only" = false ]; then
        check_dependencies
        setup_build_dir
        configure_project
        build_project
    fi
    
    if [ "$build_only" = false ]; then
        run_tests
        
        if [ "$interactive" = true ]; then
            run_interactive_test
        fi
    fi
    
    print_success "所有操作完成！"
    print_info "可执行文件位置: ${BUILD_DIR}/bin/audio_test"
}

# 运行主函数
main "$@"