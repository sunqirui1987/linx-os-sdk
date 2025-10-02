# MCP C SDK 测试构建脚本
# 
# 使用方法：
#   make all      - 编译所有测试和示例
#   make test     - 运行所有测试
#   make examples - 编译所有示例
#   make clean    - 清理编译文件
#   make help     - 显示帮助信息

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0 -fPIC -I../../cjson -I../../log
LDFLAGS = -lm

# 目录设置
SRC_DIR = ..
CJSON_DIR = ../../cjson
LOG_DIR = ../../log
TEST_DIR = .
EXAMPLES_DIR = examples
BUILD_DIR = build

# 源文件
MCP_SOURCES = $(SRC_DIR)/mcp_utils.c $(SRC_DIR)/mcp_property.c $(SRC_DIR)/mcp_tool.c $(SRC_DIR)/mcp_server.c
CJSON_SOURCES = $(CJSON_DIR)/cJSON.c $(CJSON_DIR)/cJSON_Utils.c
LOG_SOURCES = $(LOG_DIR)/linx_log.c

# 测试文件
TEST_SOURCES = test_types.c test_utils.c test_property.c test_tool.c test_server.c test_integration.c
TEST_TARGETS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%)

# 示例文件
EXAMPLE_SOURCES = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_TARGETS = $(EXAMPLE_SOURCES:$(EXAMPLES_DIR)/%.c=$(BUILD_DIR)/%)

# 默认目标
.PHONY: all test clean examples help

all: $(TEST_TARGETS) $(EXAMPLE_TARGETS)

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 编译测试程序
$(BUILD_DIR)/test_%: test_%.c $(MCP_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) test_framework.c | $(BUILD_DIR)
	@echo "编译测试: $@"
	@$(CC) $(CFLAGS) -o $@ $< test_framework.c $(MCP_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) $(LDFLAGS)

# 编译示例程序
$(BUILD_DIR)/%: $(EXAMPLES_DIR)/%.c $(MCP_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) | $(BUILD_DIR)
	@echo "编译示例: $@"
	@$(CC) $(CFLAGS) -o $@ $< $(MCP_SOURCES) $(CJSON_SOURCES) $(LOG_SOURCES) $(LDFLAGS)

# 运行所有测试
test: $(TEST_TARGETS)
	@echo "=========================================="
	@echo "运行 MCP C SDK 测试套件"
	@echo "=========================================="
	@echo ""
	@echo "1. 运行类型定义测试..."
	@$(BUILD_DIR)/test_types
	@echo ""
	@echo "2. 运行工具函数测试..."
	@$(BUILD_DIR)/test_utils
	@echo ""
	@echo "3. 运行属性管理测试..."
	@$(BUILD_DIR)/test_property
	@echo ""
	@echo "4. 运行工具管理测试..."
	@$(BUILD_DIR)/test_tool
	@echo ""
	@echo "5. 运行服务器功能测试..."
	@$(BUILD_DIR)/test_server
	@echo ""
	@echo "6. 运行集成测试..."
	@$(BUILD_DIR)/test_integration
	@echo ""
	@echo "=========================================="
	@echo "所有测试完成！"
	@echo "=========================================="

# 运行单个测试
test-types: $(BUILD_DIR)/test_types
	@echo "运行类型定义测试..."
	@$(BUILD_DIR)/test_types

test-utils: $(BUILD_DIR)/test_utils
	@echo "运行工具函数测试..."
	@$(BUILD_DIR)/test_utils

test-property: $(BUILD_DIR)/test_property
	@echo "运行属性管理测试..."
	@$(BUILD_DIR)/test_property

test-tool: $(BUILD_DIR)/test_tool
	@echo "运行工具管理测试..."
	@$(BUILD_DIR)/test_tool

test-server: $(BUILD_DIR)/test_server
	@echo "运行服务器功能测试..."
	@$(BUILD_DIR)/test_server

test-integration: $(BUILD_DIR)/test_integration
	@echo "运行集成测试..."
	@$(BUILD_DIR)/test_integration

# 编译示例程序
examples: $(EXAMPLE_TARGETS)
	@echo "所有示例程序编译完成！"
	@echo "可执行文件位于 $(BUILD_DIR)/ 目录："
	@ls -la $(BUILD_DIR)/*server 2>/dev/null || echo "没有找到示例程序"

# 运行示例程序自动化测试
test-examples: $(EXAMPLE_TARGETS)
	@echo "=========================================="
	@echo "运行示例程序自动化测试"
	@echo "=========================================="
	@echo ""
	@for example in $(EXAMPLE_TARGETS); do \
		if [ -f "$$example" ]; then \
			echo "运行 $$(basename $$example) 自动化测试..."; \
			$$example || echo "$$(basename $$example) 测试完成"; \
			echo ""; \
		fi; \
	done
	@echo "=========================================="
	@echo "所有示例程序自动化测试完成！"
	@echo "=========================================="

# 运行单个示例程序
run-calculator: $(BUILD_DIR)/calculator_server
	@echo "启动计算器服务器示例..."
	@$(BUILD_DIR)/calculator_server

run-file-manager: $(BUILD_DIR)/file_manager_server
	@echo "启动文件管理服务器示例..."
	@$(BUILD_DIR)/file_manager_server

run-weather: $(BUILD_DIR)/weather_server
	@echo "启动天气服务器示例..."
	@$(BUILD_DIR)/weather_server

# 代码覆盖率测试（需要 gcov）
coverage: CFLAGS += --coverage
coverage: LDFLAGS += --coverage
coverage: clean $(TEST_TARGETS)
	@echo "运行覆盖率测试..."
	@$(MAKE) test
	@echo "生成覆盖率报告..."
	@gcov $(MCP_SOURCES) $(LOG_SOURCES) 2>/dev/null || echo "gcov 不可用"
	@echo "覆盖率报告已生成（*.gcov 文件）"

# 内存泄漏检测（需要 valgrind）
valgrind: $(TEST_TARGETS)
	@echo "运行内存泄漏检测..."
	@if command -v valgrind >/dev/null 2>&1; then \
		for test in $(TEST_TARGETS); do \
			echo "检测 $$test..."; \
			valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $$test; \
		done; \
	else \
		echo "valgrind 不可用，跳过内存泄漏检测"; \
	fi

# 静态代码分析（需要 cppcheck）
static-analysis:
	@echo "运行静态代码分析..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 $(SRC_DIR)/*.c $(TEST_DIR)/*.c; \
	else \
		echo "cppcheck 不可用，跳过静态代码分析"; \
	fi

# 格式化代码（需要 clang-format）
format:
	@echo "格式化代码..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(TEST_DIR)/*.c $(EXAMPLES_DIR)/*.c; \
		echo "代码格式化完成"; \
	else \
		echo "clang-format 不可用，跳过代码格式化"; \
	fi

# 清理编译文件
clean:
	@echo "清理编译文件..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.gcov *.gcda *.gcno
	@echo "清理完成！"

# 显示帮助信息
help:
	@echo "MCP C SDK 测试构建脚本"
	@echo ""
	@echo "可用目标："
	@echo "  all              - 编译所有测试和示例"
	@echo "  test             - 运行所有测试"
	@echo "  test-<name>      - 运行特定测试（types, utils, property, tool, server, integration）"
	@echo "  test-examples    - 运行所有示例程序自动化测试"
	@echo "  examples         - 编译所有示例程序"
	@echo "  run-<example>    - 运行特定示例（calculator, file-manager, weather）"
	@echo "  coverage         - 运行代码覆盖率测试"
	@echo "  valgrind         - 运行内存泄漏检测"
	@echo "  static-analysis  - 运行静态代码分析"
	@echo "  format           - 格式化代码"
	@echo "  clean            - 清理编译文件"
	@echo "  help             - 显示此帮助信息"
	@echo ""
	@echo "示例用法："
	@echo "  make all && make test    # 编译并运行所有测试"
	@echo "  make examples            # 编译示例程序"
	@echo "  make run-calculator      # 运行计算器示例"
	@echo "  make test-examples       # 运行示例程序自动化测试"

# 确保目标不会与文件名冲突
.PHONY: test test-types test-utils test-property test-tool test-server test-integration
.PHONY: test-examples run-calculator run-file-manager run-weather
.PHONY: coverage valgrind static-analysis format