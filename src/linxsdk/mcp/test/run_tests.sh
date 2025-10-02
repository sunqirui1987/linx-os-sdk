#!/bin/bash

# MCP C SDK 测试运行脚本
# 
# 这个脚本提供了简洁的测试执行选项，包括：
# - 编译和运行测试
# - 运行示例程序
# - 基本的错误处理

set -e  # 遇到错误时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# 全局变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
VERBOSE=false
SELECTED_TESTS=""
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 可用的测试列表
AVAILABLE_TESTS=("types" "utils" "property" "tool" "server" "integration")

# 显示帮助信息
show_help() {
    echo "MCP C SDK 测试运行脚本"
    echo ""
    echo "用法: $0 [选项] [测试名称...]"
    echo ""
    echo "选项:"
    echo "  -h, --help              显示此帮助信息"
    echo "  -v, --verbose           详细输出模式"
    echo "  -l, --list              列出所有可用测试"
    echo "  -b, --build             仅编译，不运行测试"
    echo "  -c, --clean             清理并重新编译"
    echo "  -e, --examples          运行示例程序"
    echo ""
    echo "测试名称:"
    echo "  types                   类型定义测试"
    echo "  utils                   工具函数测试"
    echo "  property                属性管理测试"
    echo "  tool                    工具管理测试"
    echo "  server                  服务器功能测试"
    echo "  integration             集成测试"
    echo "  all                     所有测试（默认）"
    echo ""
    echo "示例:"
    echo "  $0                      # 运行所有测试"
    echo "  $0 types utils          # 只运行类型和工具测试"
    echo "  $0 -v integration       # 详细模式运行集成测试"
    echo "  $0 -e                   # 运行示例程序"
}

# 列出所有可用测试
list_tests() {
    echo "可用的测试:"
    echo "  types       - 类型定义测试"
    echo "  utils       - 工具函数测试"
    echo "  property    - 属性管理测试"
    echo "  tool        - 工具管理测试"
    echo "  server      - 服务器功能测试"
    echo "  integration - 集成测试"
}

# 打印带颜色的消息
print_message() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# 打印成功消息
print_success() {
    print_message "$GREEN" "✓ $1"
}

# 打印错误消息
print_error() {
    print_message "$RED" "✗ $1"
}

# 打印警告消息
print_warning() {
    print_message "$YELLOW" "⚠ $1"
}

# 打印信息消息
print_info() {
    print_message "$BLUE" "ℹ $1"
}

# 打印标题
print_title() {
    print_message "$PURPLE" "=== $1 ==="
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查编译器
    if ! command -v gcc >/dev/null 2>&1; then
        print_error "gcc 编译器未找到，请安装 gcc"
        exit 1
    fi
    
    # 检查 make
    if ! command -v make >/dev/null 2>&1; then
        print_error "make 工具未找到，请安装 make"
        exit 1
    fi
    
    # 检查内置 cJSON 源文件
    if [ ! -f "../../cjson/cJSON.c" ]; then
        print_error "内置 cJSON 源文件未找到，请检查项目结构"
        exit 1
    fi
    
    print_success "依赖检查完成"
}

# 编译函数
compile_tests() {
    print_info "编译测试和示例程序..."
    
    if make all; then
        print_success "编译完成"
        return 0
    else
        print_error "编译失败"
        return 1
    fi
}

# 运行单个测试
run_single_test() {
    local test_name=$1
    local test_executable="$BUILD_DIR/test_$test_name"
    
    if [ ! -f "$test_executable" ]; then
        print_error "测试可执行文件不存在: $test_executable"
        return 1
    fi
    
    print_info "运行 $test_name 测试..."
    
    local output
    local exit_code
    
    if [ "$VERBOSE" = true ]; then
        "$test_executable"
        exit_code=$?
    else
        output=$("$test_executable" 2>&1)
        exit_code=$?
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ $exit_code -eq 0 ]; then
        print_success "$test_name 测试通过"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        print_error "$test_name 测试失败 (退出码: $exit_code)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        
        if [ "$VERBOSE" = false ] && [ -n "$output" ]; then
            echo "错误输出:"
            echo "$output"
        fi
        
        return 1
    fi
}

# 运行所有选定的测试
run_tests() {
    print_title "运行测试"
    
    local tests_to_run=()
    
    if [ -z "$SELECTED_TESTS" ] || [[ "$SELECTED_TESTS" == *"all"* ]]; then
        tests_to_run=("${AVAILABLE_TESTS[@]}")
    else
        IFS=' ' read -ra tests_to_run <<< "$SELECTED_TESTS"
    fi
    
    for test in "${tests_to_run[@]}"; do
        run_single_test "$test"
    done
}

# 运行示例程序
run_examples() {
    print_title "运行示例程序"
    
    if make test-examples; then
        print_success "示例程序运行完成"
        return 0
    else
        print_error "示例程序运行失败"
        return 1
    fi
}

# 显示测试结果统计
show_test_summary() {
    print_title "测试结果统计"
    
    echo "总测试数: $TOTAL_TESTS"
    
    if [ $PASSED_TESTS -gt 0 ]; then
        print_success "通过: $PASSED_TESTS"
    fi
    
    if [ $FAILED_TESTS -gt 0 ]; then
        print_error "失败: $FAILED_TESTS"
    fi
    
    local success_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    
    echo "成功率: $success_rate%"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_success "所有测试通过！"
        return 0
    else
        print_error "有测试失败！"
        return 1
    fi
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -l|--list)
            list_tests
            exit 0
            ;;
        -b|--build)
            check_dependencies
            compile_tests
            exit 0
            ;;
        -c|--clean)
            cd "$SCRIPT_DIR"
            make clean
            check_dependencies
            compile_tests
            exit 0
            ;;
        -e|--examples)
            check_dependencies
            compile_tests || exit 1
            run_examples
            exit $?
            ;;
        -*)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
        *)
            if [ -z "$SELECTED_TESTS" ]; then
                SELECTED_TESTS="$1"
            else
                SELECTED_TESTS="$SELECTED_TESTS $1"
            fi
            shift
            ;;
    esac
done

# 主执行流程
main() {
    print_title "MCP C SDK 测试运行器"
    
    # 检查依赖
    check_dependencies
    
    # 编译测试
    compile_tests || exit 1
    
    # 运行测试
    run_tests
    
    # 显示结果统计
    show_test_summary
}

# 运行主函数
main