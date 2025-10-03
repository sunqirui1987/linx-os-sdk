#!/bin/bash

# 音频服务测试构建和运行脚本
# 使用方法: ./build_and_run.sh [simple|full|both]

set -e  # 遇到错误立即退出

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

# 检查依赖
check_dependencies() {
    print_info "检查构建依赖..."
    
    # 检查cmake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    # 检查编译器
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        print_error "未找到 GCC 或 Clang 编译器"
        exit 1
    fi
    
    print_success "依赖检查通过"
}

# 构建项目
build_project() {
    print_info "开始构建项目..."
    
    # 创建构建目录
    if [ ! -d "build" ]; then
        mkdir build
        print_info "创建构建目录"
    fi
    
    cd build
    
    # 配置CMake
    print_info "配置 CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    
    # 编译
    print_info "编译项目..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd ..
    print_success "构建完成"
}

# 运行简化测试
run_simple_test() {
    print_info "运行简化测试..."
    echo "=================================="
    
    if [ -f "build/bin/simple_audio_test" ]; then
        ./build/bin/simple_audio_test
        local exit_code=$?
        
        echo "=================================="
        if [ $exit_code -eq 0 ]; then
            print_success "简化测试通过"
        else
            print_error "简化测试失败 (退出码: $exit_code)"
        fi
        return $exit_code
    else
        print_error "简化测试程序不存在"
        return 1
    fi
}

# 运行完整测试
run_full_test() {
    print_info "运行完整测试..."
    echo "=================================="
    
    if [ -f "build/bin/audio_service_test" ]; then
        print_warning "完整测试将运行30秒，按 Ctrl+C 可提前停止"
        sleep 2
        
        ./build/bin/audio_service_test
        local exit_code=$?
        
        echo "=================================="
        if [ $exit_code -eq 0 ]; then
            print_success "完整测试通过"
        else
            print_error "完整测试失败 (退出码: $exit_code)"
        fi
        return $exit_code
    else
        print_error "完整测试程序不存在"
        return 1
    fi
}

# 运行Mac测试
run_mac_test() {
    print_info "运行Mac音频测试..."
    echo "=================================="
    
    # 检查是否在macOS上
    if [[ "$OSTYPE" != "darwin"* ]]; then
        print_warning "Mac测试只能在macOS上运行"
        return 0
    fi
    
    if [ -f "build/bin/mac_audio_test" ]; then
        print_warning "Mac测试将运行30秒，按 Ctrl+C 可提前停止"
        print_info "注意：Mac测试需要PortAudio库，请确保已安装 (brew install portaudio)"
        sleep 2
        
        ./build/bin/mac_audio_test
        local exit_code=$?
        
        echo "=================================="
        if [ $exit_code -eq 0 ]; then
            print_success "Mac测试通过"
        else
            print_error "Mac测试失败 (退出码: $exit_code)"
        fi
        return $exit_code
    else
        print_error "Mac测试程序不存在"
        return 1
    fi
}

# 清理构建文件
clean_build() {
    print_info "清理构建文件..."
    if [ -d "build" ]; then
        rm -rf build
        print_success "构建文件已清理"
    else
        print_info "没有构建文件需要清理"
    fi
}

# 显示帮助信息
show_help() {
    echo "音频服务测试构建和运行脚本"
    echo ""
    echo "使用方法:"
    echo "  $0 [选项]"
    echo ""
    echo "选项:"
    echo "  simple    - 只运行简化测试"
    echo "  full      - 只运行完整测试"
    echo "  mac       - 只运行Mac音频测试 (仅macOS)"
    echo "  both      - 运行简化和完整测试 (默认)"
    echo "  all       - 运行所有测试 (包括Mac测试)"
    echo "  build     - 只构建，不运行测试"
    echo "  clean     - 清理构建文件"
    echo "  help      - 显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0              # 构建并运行基本测试"
    echo "  $0 simple       # 构建并运行简化测试"
    echo "  $0 mac          # 构建并运行Mac测试"
    echo "  $0 all          # 构建并运行所有测试"
    echo "  $0 build        # 只构建项目"
    echo "  $0 clean        # 清理构建文件"
}

# 主函数
main() {
    local action=${1:-both}
    
    case $action in
        "help"|"-h"|"--help")
            show_help
            exit 0
            ;;
        "clean")
            clean_build
            exit 0
            ;;
        "build")
            check_dependencies
            build_project
            print_success "构建完成，可执行文件位于 build/bin/ 目录"
            exit 0
            ;;
        "simple")
            check_dependencies
            build_project
            run_simple_test
            exit $?
            ;;
        "full")
            check_dependencies
            build_project
            run_full_test
            exit $?
            ;;
        "mac")
            check_dependencies
            build_project
            run_mac_test
            exit $?
            ;;
        "both")
            check_dependencies
            build_project
            
            print_info "开始运行测试套件..."
            
            # 运行简化测试
            run_simple_test
            simple_result=$?
            
            echo ""
            
            # 运行完整测试
            run_full_test
            full_result=$?
            
            echo ""
            print_info "测试套件运行完成"
            
            if [ $simple_result -eq 0 ] && [ $full_result -eq 0 ]; then
                print_success "所有测试都通过了！"
                exit 0
            else
                print_error "部分测试失败"
                exit 1
            fi
            ;;
        "all")
            check_dependencies
            build_project
            
            print_info "开始运行完整测试套件..."
            
            # 运行简化测试
            run_simple_test
            simple_result=$?
            
            echo ""
            
            # 运行完整测试
            run_full_test
            full_result=$?
            
            echo ""
            
            # 运行Mac测试（如果在macOS上）
            mac_result=0
            if [[ "$OSTYPE" == "darwin"* ]]; then
                run_mac_test
                mac_result=$?
            else
                print_info "跳过Mac测试（非macOS平台）"
            fi
            
            echo ""
            print_info "完整测试套件运行完成"
            
            if [ $simple_result -eq 0 ] && [ $full_result -eq 0 ] && [ $mac_result -eq 0 ]; then
                print_success "所有测试都通过了！"
                exit 0
            else
                print_error "部分测试失败"
                exit 1
            fi
            ;;
        *)
            print_error "未知选项: $action"
            show_help
            exit 1
            ;;
    esac
}

# 脚本入口点
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi