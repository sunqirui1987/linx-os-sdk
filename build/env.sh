#!/bin/bash
# LinX OS SDK 环境设置脚本
# 类似 ESP-IDF 的环境配置，用于设置编译环境

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LINX_SDK_PATH="$(dirname "$SCRIPT_DIR")"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[LinX SDK]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[LinX SDK]${NC} $1"
}

log_error() {
    echo -e "${RED}[LinX SDK]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[LinX SDK]${NC} $1"
}

# 检查是否已经设置过环境
if [[ "$LINX_SDK_ENV_SET" == "1" ]]; then
    log_warn "LinX SDK 环境已经设置过了"
    return 0 2>/dev/null || exit 0
fi

# 设置基本环境变量
export LINX_SDK_ENV_SET=1
export LINX_SDK_VERSION="1.0.0"
export LINX_BUILD_DIR="$LINX_SDK_PATH/build"
export LINX_TOOLS_DIR="$LINX_SDK_PATH/tools"
export LINX_TOOLCHAINS_DIR="$LINX_SDK_PATH/toolchains"
export LINX_BOARD_DIR="$LINX_SDK_PATH/board"
export LINX_EXAMPLES_DIR="$LINX_SDK_PATH/examples"

# 添加工具到PATH
export PATH="$LINX_TOOLS_DIR:$PATH"
export PATH="$LINX_BUILD_DIR:$PATH"

# 设置Python路径
if [[ -d "$LINX_TOOLS_DIR" ]]; then
    export PYTHONPATH="$LINX_TOOLS_DIR:$PYTHONPATH"
fi

# 工具链检测函数
detect_toolchain() {
    local target="$1"
    local toolchain_path=""
    
    case "$target" in
        "native"|"mac")
            if command -v gcc >/dev/null 2>&1; then
                toolchain_path="$(which gcc)"
                log_info "检测到本机工具链: $toolchain_path"
            fi
            ;;
        "esp32")
            if [[ -n "$IDF_PATH" ]] && [[ -d "$IDF_PATH" ]]; then
                log_info "检测到 ESP-IDF: $IDF_PATH"
                toolchain_path="$IDF_PATH"
            else
                log_warn "未检测到 ESP-IDF 环境，请先运行 get_idf"
            fi
            ;;
        "riscv32")
            local riscv_paths=(
                "/opt/riscv32/bin"
                "/usr/local/riscv32/bin"
                "/opt/riscv/bin"
                "/usr/local/riscv/bin"
                "/home/sqr-ubuntu/nds32le-linux-musl-v5d/bin"
            )
            for path in "${riscv_paths[@]}"; do
                if [[ -d "$path" ]] && [[ -f "$path/riscv32-linux-musl-gcc" || -f "$path/nds32le-linux-musl-v5d-gcc" ]]; then
                    toolchain_path="$path"
                    log_info "检测到 RISC-V 工具链: $toolchain_path"
                    break
                fi
            done
            ;;
        "arm"|"awol")
            local arm_paths=(
                "/opt/arm-linux-gnueabihf/bin"
                "/usr/local/arm-linux-gnueabihf/bin"
                "/opt/gcc-arm-linux-gnueabihf/bin"
            )
            for path in "${arm_paths[@]}"; do
                if [[ -d "$path" ]] && [[ -f "$path/arm-linux-gnueabihf-gcc" ]]; then
                    toolchain_path="$path"
                    log_info "检测到 ARM 工具链: $toolchain_path"
                    break
                fi
            done
            if [[ -z "$toolchain_path" ]] && command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1; then
                toolchain_path="$(dirname "$(which arm-linux-gnueabihf-gcc)")"
                log_info "检测到系统 ARM 工具链: $toolchain_path"
            fi
            ;;
    esac
    
    echo "$toolchain_path"
}

# 设置工具链环境
setup_toolchain() {
    local target="$1"
    local toolchain_path="$(detect_toolchain "$target")"
    
    if [[ -n "$toolchain_path" ]]; then
        export LINX_TOOLCHAIN_PATH="$toolchain_path"
        export PATH="$toolchain_path:$PATH"
        log_success "工具链已设置: $toolchain_path"
    else
        log_warn "未找到 $target 平台的工具链"
    fi
}

# 创建便捷命令函数
lunch() {
    python3 "$LINX_SDK_PATH/lunch.py" "$@"
}

# 构建命令
linx_build() {
    python3 "$LINX_SDK_PATH/lunch.py" build "$@"
}

# 清理命令
linx_clean() {
    python3 "$LINX_SDK_PATH/lunch.py" clean "$@"
}

# 配置命令
linx_menuconfig() {
    python3 "$LINX_SDK_PATH/lunch.py" menuconfig "$@"
}

# 烧录命令（ESP32）
linx_flash() {
    python3 "$LINX_SDK_PATH/lunch.py" flash "$@"
}

# 监控命令（ESP32）
linx_monitor() {
    python3 "$LINX_SDK_PATH/lunch.py" monitor "$@"
}

# 显示帮助
linx_help() {
    echo -e "${CYAN}LinX OS SDK 编译环境${NC}"
    echo ""
    echo "可用命令:"
    echo "  lunch [target]          - 选择目标平台并配置环境"
    echo "  linx_build [options]    - 构建项目"
    echo "  linx_clean              - 清理构建文件"
    echo "  linx_menuconfig         - 打开配置界面"
    echo "  linx_flash              - 烧录到设备 (ESP32)"
    echo "  linx_monitor            - 监控串口输出 (ESP32)"
    echo "  linx_help               - 显示此帮助信息"
    echo ""
    echo "支持的目标平台:"
    echo "  native, mac             - 本机开发"
    echo "  esp32, esp32s3          - ESP32 系列"
    echo "  riscv32                 - RISC-V 32位"
    echo "  arm, awol               - ARM 平台"
    echo ""
    echo "示例用法:"
    echo "  source build/env.sh     - 设置环境"
    echo "  lunch esp32s3           - 选择 ESP32-S3 平台"
    echo "  linx_build              - 构建项目"
    echo "  linx_flash monitor      - 烧录并监控"
}

# 检查Python环境
check_python() {
    if ! command -v python3 >/dev/null 2>&1; then
        log_error "未找到 Python3，请先安装 Python3"
        return 1
    fi
    
    # 检查必要的Python模块
    local required_modules=("argparse" "json" "os" "sys")
    for module in "${required_modules[@]}"; do
        if ! python3 -c "import $module" >/dev/null 2>&1; then
            log_error "Python模块 $module 不可用"
            return 1
        fi
    done
    
    return 0
}

# 主初始化流程
main() {
    log_info "初始化 LinX OS SDK 环境..."
    
    # 检查Python环境
    if ! check_python; then
        log_error "Python环境检查失败"
        return 1
    fi
    
    # 创建必要的目录
    mkdir -p "$LINX_BUILD_DIR"
    mkdir -p "$LINX_BUILD_DIR/cache"
    mkdir -p "$LINX_BUILD_DIR/logs"
    
    # 显示环境信息
    log_success "LinX OS SDK 环境设置完成"
    echo ""
    echo -e "${CYAN}环境信息:${NC}"
    echo "  SDK路径: $LINX_SDK_PATH"
    echo "  构建目录: $LINX_BUILD_DIR"
    echo "  版本: $LINX_SDK_VERSION"
    echo ""
    echo "运行 'linx_help' 查看可用命令"
    echo "运行 'lunch' 选择目标平台"
}

# 如果直接运行脚本，执行主函数
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
else
    # 如果是source执行，直接初始化
    main
fi