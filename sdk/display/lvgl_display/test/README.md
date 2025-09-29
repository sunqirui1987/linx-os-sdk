# LVGL Display Demo

这是一个完整的LVGL显示模块演示程序，展示了`lvgl_display`模块的各种功能和特性。

## 📋 功能特性

### 🏠 主页面
- **系统状态显示**: 实时显示电池电量、WiFi连接状态、蓝牙状态和音频状态
- **通知功能**: 点击"📢 Notify"按钮发送各种类型的通知消息
- **截图功能**: 点击"📸 Shot"按钮进行屏幕截图并保存为JPEG格式
- **音频控制**: 点击"🔇 Mute"/"🔊 Unmute"按钮切换音频静音状态

### ⚙️ 设置页面
- **电池电量调节**: 使用滑块调节模拟电池电量（0-100%）
- **WiFi开关**: 切换WiFi连接状态
- **蓝牙开关**: 切换蓝牙连接状态
- **省电模式**: 切换显示器省电模式

### 🎨 演示页面
- **渐变背景**: 展示LVGL的渐变色彩效果
- **进度条**: 动态进度条演示
- **加载动画**: 旋转加载指示器
- **表情符号**: 各种emoji表情显示

### 📊 状态栏
- **实时时间**: 显示当前系统时间（HH:MM格式）
- **状态图标**: WiFi、蓝牙和电池状态图标
- **自动更新**: 每秒自动更新时间和状态信息

## 🛠️ 编译要求

### 系统要求
- **操作系统**: macOS, Linux, Windows
- **编译器**: GCC 或 Clang（支持C99标准）
- **CMake**: 版本 3.10 或更高

### 依赖库
- **LVGL**: 轻量级图形库
- **数学库**: libm（用于数学计算）
- **标准C库**: stdio, stdlib, string, unistd, time, signal

## 🚀 编译和运行

### 1. 进入测试目录
```bash
cd /Users/sunqirui/gitlab/aiagent/linx-os-sdk/sdk/display/lvgl_display/test
```

### 2. 创建构建目录
```bash
mkdir build
cd build
```

### 3. 配置CMake
```bash
cmake ..
```

### 4. 编译项目
```bash
make
```

### 5. 运行演示程序
```bash
./lvgl_display_demo
```

或者使用CMake目标：
```bash
make run_lvgl_display_test
```

## 📱 使用说明

### 启动程序
运行程序后，您将看到：
```
🚀 Starting LVGL Display Demo
==============================
✅ LVGL display initialized successfully
🎨 Demo UI created successfully
💡 Use Ctrl+C to exit gracefully
📱 Interact with the UI to test different features
```

### 界面导航
- 程序启动后显示三个标签页：**🏠 Home**、**⚙️ Settings**、**🎨 Demo**
- 点击标签页标题可以切换不同的功能页面
- 顶部状态栏显示当前时间和系统状态

### 交互功能

#### 主页面操作
1. **发送通知**: 点击"📢 Notify"按钮，系统会循环显示不同类型的通知消息
2. **截图功能**: 点击"📸 Shot"按钮，程序会截取当前屏幕并保存为JPEG格式
3. **音频控制**: 点击"🔇 Mute"按钮切换音频静音状态，按钮文字会相应更新

#### 设置页面操作
1. **调节电池**: 拖动电池滑块可以模拟不同的电池电量，状态栏会实时更新
2. **WiFi开关**: 切换WiFi开关，系统会显示连接状态变化的通知
3. **蓝牙开关**: 切换蓝牙开关，系统会显示连接状态变化的通知
4. **省电模式**: 点击省电模式按钮可以切换显示器的省电状态

#### 演示页面
- 展示各种LVGL视觉效果，包括渐变、进度条、动画等
- 所有效果都是实时渲染的

### 自动功能
- **时间更新**: 状态栏时间每秒自动更新
- **电池模拟**: 每30秒自动降低1%电池电量（模拟电池消耗）
- **低电量警告**: 当电池电量低于20%时，会自动显示低电量警告通知

### 退出程序
- 按 `Ctrl+C` 优雅退出程序
- 程序会自动清理资源并显示退出信息

## 🔧 故障排除

### 编译错误
1. **CMake找不到**: 确保已安装CMake并添加到PATH环境变量
2. **LVGL库缺失**: 检查LVGL库是否正确安装和配置
3. **编译器错误**: 确保使用支持C99标准的编译器

### 运行时错误
1. **显示初始化失败**: 检查显示驱动是否正确配置
2. **内存不足**: 确保系统有足够的内存运行程序
3. **权限问题**: 在某些系统上可能需要管理员权限

### 性能优化
- 如果程序运行缓慢，可以调整`DEMO_UPDATE_PERIOD`宏的值（在源码中）
- 减少同时显示的UI元素数量
- 优化图形渲染设置

## 📁 文件结构

```
test/
├── CMakeLists.txt              # CMake构建配置文件
├── README.md                   # 本文档
├── lvgl_display_demo.c         # 完整的演示程序源码
└── test_lvgl_display.c         # 基础测试程序源码
```

## 🎯 测试覆盖

该演示程序测试了以下`lvgl_display`模块功能：

### 核心功能
- ✅ `lvgl_display_create()` - 创建显示对象
- ✅ `lvgl_display_init()` - 初始化显示
- ✅ `lvgl_display_destroy()` - 销毁显示对象

### 状态管理
- ✅ `lvgl_display_set_status()` - 设置状态信息
- ✅ `lvgl_display_update_status_bar()` - 更新状态栏

### 通知系统
- ✅ `lvgl_display_show_notification()` - 显示通知消息

### 电源管理
- ✅ `lvgl_display_set_power_save_mode()` - 设置省电模式

### 截图功能
- ✅ `lvgl_display_snapshot_to_jpeg()` - 截图并保存为JPEG

### 显示锁定
- ✅ `lvgl_display_lock()` - 锁定显示
- ✅ `lvgl_display_unlock()` - 解锁显示

## 🤝 贡献

如果您发现问题或有改进建议，请：
1. 检查现有的问题和功能请求
2. 创建详细的问题报告或功能请求
3. 提交代码改进的Pull Request

## 📄 许可证

本演示程序遵循项目的整体许可证协议。

---

**注意**: 这是一个演示程序，主要用于测试和展示`lvgl_display`模块的功能。在生产环境中使用时，请根据实际需求进行适当的修改和优化。