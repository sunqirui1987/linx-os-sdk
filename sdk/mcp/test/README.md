# MCP C SDK 测试套件

这个目录包含了 MCP (Model Context Protocol) C SDK 的完整测试套件和示例程序。

## 目录结构

```
test/
├── Makefile                    # 构建脚本
├── README.md                   # 本文档
├── test_types.c               # 类型定义测试
├── test_utils.c               # 工具函数测试
├── test_property.c            # 属性管理测试
├── test_tool.c                # 工具管理测试
├── test_server.c              # 服务器功能测试
├── test_integration.c         # 集成测试
├── examples/                  # 示例程序目录
│   ├── calculator_server.c   # 计算器服务器示例
│   ├── file_manager_server.c # 文件管理服务器示例
│   └── weather_server.c      # 天气服务器示例
└── build/                     # 编译输出目录（自动创建）
```

## 快速开始

### 1. 安装依赖

确保系统已安装以下依赖：

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install gcc libcjson-dev make
```

**CentOS/RHEL:**
```bash
sudo yum install gcc cjson-devel make
```

**macOS:**
```bash
brew install gcc cjson make
```

### 2. 编译测试

```bash
# 编译所有测试和示例
make all

# 或者只编译测试
make test-types test-utils test-property test-tool test-server test-integration

# 或者只编译示例
make examples
```

### 3. 运行测试

```bash
# 运行所有测试
make test

# 运行单个测试
make test-types      # 类型定义测试
make test-utils      # 工具函数测试
make test-property   # 属性管理测试
make test-tool       # 工具管理测试
make test-server     # 服务器功能测试
make test-integration # 集成测试
```

### 4. 运行示例程序自动化测试

```bash
# 运行所有示例程序自动化测试
make -examples

# 运行单个示例程序自动化测试
make run-calculator     # 计算器服务器自动化测试
make run-file-manager   # 文件管理服务器自动化测试
make run-weather        # 天气服务器自动化测试
```
