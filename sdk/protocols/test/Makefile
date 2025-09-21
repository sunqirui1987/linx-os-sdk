# Makefile for linx protocols examples
# 用于编译和运行 linx 协议示例程序

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -fPIC
LDFLAGS = -lm -lpthread

# 目录设置
PROTOCOLS_DIR = ../
EXAMPLES_DIR = .
BUILD_DIR = build
CJSON_DIR = ../../cjson
LOG_DIR = ../../log

# 源文件
PROTOCOL_SOURCES = $(PROTOCOLS_DIR)/linx_protocol.c $(PROTOCOLS_DIR)/linx_websocket.c
CJSON_SOURCES = $(CJSON_DIR)/cJSON.c $(CJSON_DIR)/cJSON_Utils.c
LOG_SOURCES = $(LOG_DIR)/linx_log.c
EXAMPLE_WEBSOCKET_SRC = example_linx_websocket.c

# 目标文件
EXAMPLE_WEBSOCKET_TARGET = $(BUILD_DIR)/example_linx_websocket

# 包含路径
INCLUDES = -I$(PROTOCOLS_DIR) -I$(CJSON_DIR)

# 检测操作系统
UNAME_S := $(shell uname -s)

# Mongoose 依赖检测
MONGOOSE_FOUND = 0
MONGOOSE_CFLAGS = 
MONGOOSE_LIBS = 

# 首先尝试 pkg-config
ifneq ($(shell pkg-config --exists libmongoose 2>/dev/null; echo $$?), 0)
    # pkg-config 失败，尝试手动查找
    ifeq ($(UNAME_S),Darwin)
        # macOS 系统
        ifneq ($(wildcard ../../third/mongoose/install/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I../../third/mongoose/install/include
            MONGOOSE_LIBS = -L../../third/mongoose/install/lib -lmongoose
            MONGOOSE_FOUND = 1
        else ifneq ($(wildcard /opt/homebrew/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I/opt/homebrew/include
            MONGOOSE_LIBS = -L/opt/homebrew/lib -lmongoose
            MONGOOSE_FOUND = 1
        else ifneq ($(wildcard /opt/homebrew/Cellar/mongoose/*/include/mongoose.h),)
            MONGOOSE_INCLUDE_DIR = $(shell find /opt/homebrew/Cellar/mongoose/*/include -name mongoose.h -exec dirname {} \; 2>/dev/null | head -1)
            MONGOOSE_LIB_DIR = $(shell find /opt/homebrew/Cellar/mongoose/*/lib -name "libmongoose.*" -exec dirname {} \; 2>/dev/null | head -1)
            ifneq ($(MONGOOSE_INCLUDE_DIR),)
                MONGOOSE_CFLAGS = -I$(MONGOOSE_INCLUDE_DIR)
                MONGOOSE_LIBS = -L$(MONGOOSE_LIB_DIR) -lmongoose
                MONGOOSE_FOUND = 1
            endif
        else ifneq ($(wildcard /usr/local/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I/usr/local/include
            MONGOOSE_LIBS = -L/usr/local/lib -lmongoose
            MONGOOSE_FOUND = 1
        endif
    else
        # Linux 系统
        ifneq ($(wildcard ../../third/mongoose/install/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I../../third/mongoose/install/include
            MONGOOSE_LIBS = -L../../third/mongoose/install/lib -lmongoose
            MONGOOSE_FOUND = 1
        else ifneq ($(wildcard /usr/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I/usr/include
            MONGOOSE_LIBS = -lmongoose
            MONGOOSE_FOUND = 1
        else ifneq ($(wildcard /usr/local/include/mongoose.h),)
            MONGOOSE_CFLAGS = -I/usr/local/include
            MONGOOSE_LIBS = -L/usr/local/lib -lmongoose
            MONGOOSE_FOUND = 1
        endif
    endif
else
    # pkg-config 成功
    MONGOOSE_CFLAGS = $(shell pkg-config --cflags libmongoose)
    MONGOOSE_LIBS = $(shell pkg-config --libs libmongoose)
    MONGOOSE_FOUND = 1
endif

# 默认目标
.PHONY: all clean help run-websocket run-all check-deps install-deps debug info

all: check-deps $(EXAMPLE_WEBSOCKET_TARGET)

# 检查依赖
check-deps:
	@echo "🔍 检查编译依赖..."
	@echo "操作系统: $(UNAME_S)"
	@which $(CC) > /dev/null || (echo "❌ 错误: 未找到 $(CC) 编译器" && exit 1)
	@echo "✅ $(CC) 编译器已安装"
	@which make > /dev/null || (echo "❌ 错误: 未找到 make 工具" && exit 1)
	@echo "✅ make 工具已安装"
	@echo "检查 pthread 支持..."
	@echo '#include <pthread.h>\nint main(){return 0;}' | $(CC) -x c - -lpthread -o /tmp/pthread_test 2>/dev/null && rm -f /tmp/pthread_test && echo "✅ pthread 库可用" || (echo "❌ pthread 库不可用" && exit 1)
	@echo "检查 cJSON 源文件..."
	@test -f "$(CJSON_DIR)/cJSON.c" && echo "✅ cJSON 源文件存在" || (echo "❌ cJSON 源文件不存在: $(CJSON_DIR)/cJSON.c" && exit 1)
	@echo "检查协议源文件..."
	@test -f "$(PROTOCOLS_DIR)/linx_protocol.c" && echo "✅ linx_protocol.c 存在" || (echo "❌ linx_protocol.c 不存在: $(PROTOCOLS_DIR)/linx_protocol.c" && exit 1)
	@test -f "$(PROTOCOLS_DIR)/linx_websocket.c" && echo "✅ linx_websocket.c 存在" || (echo "❌ linx_websocket.c 不存在: $(PROTOCOLS_DIR)/linx_websocket.c" && exit 1)
	@echo "检查 mongoose 库..."
	@if [ "$(MONGOOSE_FOUND)" = "1" ]; then \
		echo "✅ mongoose 库已找到"; \
		echo "   CFLAGS: $(MONGOOSE_CFLAGS)"; \
		echo "   LIBS: $(MONGOOSE_LIBS)"; \
	else \
		echo "⚠️  警告: 未找到 mongoose 库，WebSocket 示例将无法编译"; \
		echo "   安装方法:"; \
		echo "   macOS: brew install mongoose"; \
		echo "   Ubuntu/Debian: sudo apt-get install libmongoose-dev"; \
	fi
	@echo "✅ 依赖检查完成"

# 安装依赖（仅提示）
install-deps:
	@echo "📦 依赖安装指南"
	@echo "================"
	@echo ""
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		echo "macOS 系统:"; \
		echo "  brew install mongoose"; \
		echo "  brew install pkg-config"; \
	elif [ "$(UNAME_S)" = "Linux" ]; then \
		echo "Ubuntu/Debian 系统:"; \
		echo "  sudo apt-get update"; \
		echo "  sudo apt-get install libmongoose-dev pkg-config build-essential"; \
		echo ""; \
		echo "CentOS/RHEL 系统:"; \
		echo "  sudo yum install mongoose-devel pkg-config gcc make"; \
	else \
		echo "其他系统: 请手动安装 mongoose 库和开发工具"; \
	fi

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 编译 linx_websocket 示例
$(EXAMPLE_WEBSOCKET_TARGET): $(EXAMPLE_WEBSOCKET_SRC) $(PROTOCOL_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) | $(BUILD_DIR)
	@echo "🔨 编译 linx_websocket 示例..."
	@echo "源文件: $<"
	@echo "依赖: $(PROTOCOL_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES)"
	@if [ "$(MONGOOSE_FOUND)" != "1" ]; then \
		echo "❌ 错误: 未找到 mongoose 库"; \
		echo "请运行 'make install-deps' 查看安装方法"; \
		exit 1; \
	else \
		echo "使用 mongoose: $(MONGOOSE_CFLAGS) $(MONGOOSE_LIBS)"; \
		$(CC) $(CFLAGS) $(INCLUDES) $(MONGOOSE_CFLAGS) -o $@ $< $(PROTOCOL_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) $(LDFLAGS) $(MONGOOSE_LIBS); \
		echo "✅ linx_websocket 示例编译完成: $@"; \
	fi

# 运行 linx_websocket 示例
run-websocket: $(EXAMPLE_WEBSOCKET_TARGET)
	@echo "🚀 运行 linx_websocket 示例..."
	@echo "================================"
	@echo "注意: 这是一个 WebSocket 长连接示例，需要网络连接"
	@echo "按 Ctrl+C 可以安全退出程序"
	@$(EXAMPLE_WEBSOCKET_TARGET)

# 运行所有示例
run-all: run-websocket
	@echo ""
	@echo "🎉 所有可用示例运行完成！"

# 调试信息
debug:
	@echo "🐛 调试信息"
	@echo "==========="
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "PROTOCOLS_DIR: $(PROTOCOLS_DIR)"
	@echo "CJSON_DIR: $(CJSON_DIR)"
	@echo "BUILD_DIR: $(BUILD_DIR)"
	@echo "UNAME_S: $(UNAME_S)"
	@echo "MONGOOSE_FOUND: $(MONGOOSE_FOUND)"
	@echo "MONGOOSE_CFLAGS: $(MONGOOSE_CFLAGS)"
	@echo "MONGOOSE_LIBS: $(MONGOOSE_LIBS)"
	@echo ""
	@echo "源文件:"
	@echo "  PROTOCOL_SOURCES: $(PROTOCOL_SOURCES)"
	@echo "  CJSON_SOURCES: $(CJSON_SOURCES)"
	@echo "  EXAMPLE_WEBSOCKET_SRC: $(EXAMPLE_WEBSOCKET_SRC)"
	@echo ""
	@echo "目标文件:"
	@echo "  EXAMPLE_WEBSOCKET_TARGET: $(EXAMPLE_WEBSOCKET_TARGET)"

# 显示项目信息
info:
	@echo "📋 项目信息"
	@echo "==========="
	@echo "项目: linx 协议示例"
	@echo "版本: 1.0"
	@echo "描述: 演示 linx 协议和 WebSocket 长连接的使用"
	@echo ""
	@echo "示例程序:"
	@echo "  example_linx_websocket  - WebSocket 长连接示例"
	@echo ""
	@echo "依赖库:"
	@echo "  cJSON                   - JSON 解析库"
	@echo "  mongoose                - HTTP/WebSocket 库"
	@echo "  pthread                 - 多线程支持"

# 清理构建文件
clean:
	@echo "🧹 清理构建文件..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.o *.so *.dylib
	@rm -f core core.*
	@echo "✅ 清理完成"

# 显示帮助信息
help:
	@echo "📖 linx 协议示例 Makefile 帮助"
	@echo "================================"
	@echo ""
	@echo "可用目标:"
	@echo "  all              - 编译所有示例程序"
	@echo "  check-deps       - 检查编译依赖"
	@echo "  install-deps     - 显示依赖安装指南"
	@echo "  run-websocket    - 编译并运行 linx_websocket 示例"
	@echo "  run-all          - 运行所有可用示例"
	@echo "  debug            - 显示调试信息"
	@echo "  info             - 显示项目信息"
	@echo "  clean            - 清理构建文件"
	@echo "  help             - 显示此帮助信息"
	@echo ""
	@echo "示例用法:"
	@echo "  make                    # 编译所有示例"
	@echo "  make check-deps         # 检查依赖"
	@echo "  make install-deps       # 查看安装指南"
	@echo "  make run-websocket      # 运行 WebSocket 示例"
	@echo "  make run-all            # 运行所有示例"
	@echo "  make debug              # 查看调试信息"
	@echo "  make clean              # 清理构建文件"
	@echo ""
	@echo "依赖要求:"
	@echo "  - gcc 编译器 (支持 C99)"
	@echo "  - pthread 库"
	@echo "  - mongoose 库 (用于 WebSocket 示例)"
	@echo "  - cJSON 库 (已包含源码)"
	@echo ""
	@echo "故障排除:"
	@echo "  - 如果编译失败，运行 'make check-deps' 检查依赖"
	@echo "  - 如果缺少 mongoose，运行 'make install-deps' 查看安装方法"
	@echo "  - 运行 'make debug' 查看详细的编译配置信息"

# 防止文件名冲突
.SUFFIXES:
.SECONDARY: