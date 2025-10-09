# LinX OS SDK - 构建系统设计方案

## 概述

本文档详细描述了LinX OS SDK的构建系统设计，包括CMake配置、依赖管理、跨平台编译、测试集成和部署策略。

## 设计目标

### 1. 跨平台支持
- 支持ESP32、macOS、Linux等多个平台
- 统一的构建接口和配置
- 平台特定的优化和配置

### 2. 模块化构建
- 独立的模块构建
- 灵活的依赖管理
- 可选的功能模块

### 3. 开发效率
- 快速的增量编译
- 并行构建支持
- 开发工具集成

## 构建系统架构

### 1. 目录结构
```
src/
├── CMakeLists.txt                 # 主构建配置
├── cmake/                         # CMake模块和工具
│   ├── modules/                   # 自定义CMake模块
│   │   ├── FindOpus.cmake
│   │   ├── FindLVGL.cmake
│   │   ├── Platform.cmake
│   │   └── Toolchain.cmake
│   ├── toolchains/               # 工具链配置
│   │   ├── esp32.cmake
│   │   ├── macos.cmake
│   │   └── linux.cmake
│   └── scripts/                  # 构建脚本
│       ├── copy_resources.cmake
│       ├── generate_version.cmake
│       └── package.cmake
├── audio/
│   ├── CMakeLists.txt            # 音频模块构建
│   ├── codecs/
│   │   └── CMakeLists.txt
│   ├── play/
│   │   └── CMakeLists.txt
│   └── ...
├── common/
│   ├── CMakeLists.txt            # 通用模块构建
│   ├── cjson/
│   │   └── CMakeLists.txt
│   └── ...
├── display/
│   └── CMakeLists.txt            # 显示模块构建
├── camera/
│   └── CMakeLists.txt            # 摄像头模块构建
├── linxsdk/
│   └── CMakeLists.txt            # LinX SDK核心构建
└── third/                        # 第三方库
    ├── mongoose/
    ├── opus/
    ├── fonts/
    └── liblvgl/
```

## 主构建配置

### 1. 根CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)

# 项目定义
project(linx_sdk 
    VERSION 1.0.0
    DESCRIPTION "LinX OS SDK"
    LANGUAGES C CXX
)

# 设置C/C++标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含自定义CMake模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# 平台检测和配置
include(Platform)
detect_platform()
configure_platform()

# 构建类型配置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 编译选项
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# 平台特定编译选项
if(PLATFORM_ESP32)
    add_compile_options(-Wall -Wextra -Werror)
    add_compile_definitions(ESP32_PLATFORM)
elseif(PLATFORM_MACOS)
    add_compile_options(-Wall -Wextra)
    add_compile_definitions(MACOS_PLATFORM)
elseif(PLATFORM_LINUX)
    add_compile_options(-Wall -Wextra)
    add_compile_definitions(LINUX_PLATFORM)
endif()

# 全局包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 第三方库配置
add_subdirectory(third)

# 核心模块
add_subdirectory(common)
add_subdirectory(audio)
add_subdirectory(display)
add_subdirectory(camera)
add_subdirectory(linxsdk)

# 创建统一的SDK库
add_library(linx_sdk_static STATIC)

# 链接所有模块
target_link_libraries(linx_sdk_static PUBLIC
    linx_common
    linx_audio
    linx_display
    linx_camera
    linx_linxsdk
    linx_third_party
)

# 设置公共包含目录
target_include_directories(linx_sdk_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 导出目标
export(TARGETS linx_sdk_static FILE LinxSDKTargets.cmake)

# 安装配置
include(cmake/scripts/package.cmake)
```

### 2. 平台检测模块 (`cmake/modules/Platform.cmake`)
```cmake
# 平台检测和配置

function(detect_platform)
    # ESP32平台检测
    if(DEFINED ESP_PLATFORM)
        set(PLATFORM_ESP32 TRUE PARENT_SCOPE)
        set(PLATFORM_NAME "esp32" PARENT_SCOPE)
        message(STATUS "Detected platform: ESP32")
        
    # macOS平台检测
    elseif(APPLE)
        set(PLATFORM_MACOS TRUE PARENT_SCOPE)
        set(PLATFORM_NAME "macos" PARENT_SCOPE)
        message(STATUS "Detected platform: macOS")
        
    # Linux平台检测
    elseif(UNIX AND NOT APPLE)
        set(PLATFORM_LINUX TRUE PARENT_SCOPE)
        set(PLATFORM_NAME "linux" PARENT_SCOPE)
        message(STATUS "Detected platform: Linux")
        
    # Windows平台检测
    elseif(WIN32)
        set(PLATFORM_WINDOWS TRUE PARENT_SCOPE)
        set(PLATFORM_NAME "windows" PARENT_SCOPE)
        message(STATUS "Detected platform: Windows")
        
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endfunction()

function(configure_platform)
    # ESP32平台配置
    if(PLATFORM_ESP32)
        # ESP-IDF特定配置
        if(NOT DEFINED IDF_PATH)
            message(FATAL_ERROR "IDF_PATH not set for ESP32 build")
        endif()
        
        # ESP32特定编译选项
        add_compile_options(
            -mlongcalls
            -Wno-frame-address
        )
        
        # ESP32特定定义
        add_compile_definitions(
            ESP32_PLATFORM=1
            FREERTOS_PLATFORM=1
        )
        
    # macOS平台配置
    elseif(PLATFORM_MACOS)
        # macOS特定编译选项
        add_compile_options(
            -mmacosx-version-min=10.15
        )
        
        # macOS特定定义
        add_compile_definitions(
            MACOS_PLATFORM=1
            UNIX_PLATFORM=1
        )
        
        # 查找系统库
        find_library(COREAUDIO_FRAMEWORK CoreAudio)
        find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
        find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
        
    # Linux平台配置
    elseif(PLATFORM_LINUX)
        # Linux特定编译选项
        add_compile_options(
            -pthread
        )
        
        # Linux特定定义
        add_compile_definitions(
            LINUX_PLATFORM=1
            UNIX_PLATFORM=1
        )
        
        # 查找系统库
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ALSA REQUIRED alsa)
        
    endif()
endfunction()

# 平台特定的库链接
function(link_platform_libraries target)
    if(PLATFORM_ESP32)
        # ESP32平台库
        target_link_libraries(${target} PRIVATE
            idf::driver
            idf::esp_common
            idf::freertos
            idf::nvs_flash
            idf::esp_wifi
            idf::esp_http_client
        )
        
    elseif(PLATFORM_MACOS)
        # macOS平台库
        target_link_libraries(${target} PRIVATE
            ${COREAUDIO_FRAMEWORK}
            ${AUDIOUNIT_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
        )
        
    elseif(PLATFORM_LINUX)
        # Linux平台库
        target_link_libraries(${target} PRIVATE
            ${ALSA_LIBRARIES}
            pthread
            m
        )
        
    endif()
endfunction()
```

## 模块构建配置

### 1. 音频模块 (`audio/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(linx_audio)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/codecs
    ${CMAKE_CURRENT_SOURCE_DIR}/play
    ${CMAKE_CURRENT_SOURCE_DIR}/processor
    ${CMAKE_CURRENT_SOURCE_DIR}/vad
    ${CMAKE_CURRENT_SOURCE_DIR}/wake_words
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/opus/include
)

# 收集源文件
file(GLOB_RECURSE AUDIO_SOURCES
    "audio/*.c"
    "codecs/*.c"
    "play/*.c"
    "processor/*.c"
    "vad/*.c"
    "wake_words/*.c"
)

# 平台特定源文件
if(PLATFORM_ESP32)
    file(GLOB_RECURSE PLATFORM_SOURCES "platform/esp32/*.c")
elseif(PLATFORM_MACOS)
    file(GLOB_RECURSE PLATFORM_SOURCES "platform/macos/*.c")
elseif(PLATFORM_LINUX)
    file(GLOB_RECURSE PLATFORM_SOURCES "platform/linux/*.c")
endif()

list(APPEND AUDIO_SOURCES ${PLATFORM_SOURCES})

# 创建音频库
add_library(linx_audio STATIC ${AUDIO_SOURCES})

# 链接依赖
target_link_libraries(linx_audio PUBLIC
    linx_common
)

# 平台特定链接
link_platform_libraries(linx_audio)

# 条件编译配置
if(ENABLE_OPUS_CODEC)
    target_link_libraries(linx_audio PRIVATE opus)
    target_compile_definitions(linx_audio PRIVATE ENABLE_OPUS_CODEC=1)
endif()

if(ENABLE_VAD)
    target_compile_definitions(linx_audio PRIVATE ENABLE_VAD=1)
endif()

if(ENABLE_WAKE_WORDS)
    target_compile_definitions(linx_audio PRIVATE ENABLE_WAKE_WORDS=1)
endif()

# 导出头文件
target_include_directories(linx_audio PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/audio>
)
```

### 2. 通用模块 (`common/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(linx_common)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/http
    ${CMAKE_CURRENT_SOURCE_DIR}/log
    ${CMAKE_CURRENT_SOURCE_DIR}/std
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/mongoose
)

# 收集源文件
set(COMMON_SOURCES
    # cJSON相关
    cjson/cjson_utils.c
    
    # HTTP客户端
    http/http_client.c
    
    # 日志系统
    log/log.c
    log/log_console.c
    log/log_file.c
    
    # 标准库扩展
    std/vector.c
    std/string_utils.c
    std/memory_pool.c
)

# 平台特定源文件
if(PLATFORM_ESP32)
    list(APPEND COMMON_SOURCES
        platform/esp32/esp32_log.c
        platform/esp32/esp32_http.c
    )
elseif(PLATFORM_MACOS)
    list(APPEND COMMON_SOURCES
        platform/macos/macos_log.c
        platform/macos/macos_http.c
    )
endif()

# 创建通用库
add_library(linx_common STATIC ${COMMON_SOURCES})

# 链接第三方库
target_link_libraries(linx_common PUBLIC
    cjson
    mongoose
)

# 平台特定链接
link_platform_libraries(linx_common)

# 编译定义
target_compile_definitions(linx_common PRIVATE
    CJSON_HIDE_SYMBOLS
    MG_ENABLE_PACKED_FS=1
)

# 导出头文件
target_include_directories(linx_common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/common>
)
```

### 3. 显示模块 (`display/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(linx_display)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log
    ${CMAKE_CURRENT_SOURCE_DIR}/../third/liblvgl
)

# 收集源文件
file(GLOB DISPLAY_SOURCES "*.c")

# 平台特定源文件
if(PLATFORM_ESP32)
    file(GLOB PLATFORM_SOURCES "platform/esp32/*.c")
    list(APPEND DISPLAY_SOURCES ${PLATFORM_SOURCES})
    
    # ESP32特定包含目录
    include_directories(
        ${IDF_PATH}/components/esp_lcd/include
        ${IDF_PATH}/components/driver/include
    )
    
elseif(PLATFORM_MACOS)
    file(GLOB PLATFORM_SOURCES "platform/macos/*.c")
    list(APPEND DISPLAY_SOURCES ${PLATFORM_SOURCES})
    
elseif(PLATFORM_LINUX)
    file(GLOB PLATFORM_SOURCES "platform/linux/*.c")
    list(APPEND DISPLAY_SOURCES ${PLATFORM_SOURCES})
endif()

# 创建显示库
add_library(linx_display STATIC ${DISPLAY_SOURCES})

# 链接依赖
target_link_libraries(linx_display PUBLIC
    linx_common
    lvgl
)

# 平台特定链接
link_platform_libraries(linx_display)

# 条件编译配置
if(ENABLE_LVGL)
    target_compile_definitions(linx_display PRIVATE ENABLE_LVGL=1)
endif()

if(ENABLE_SSD1306)
    target_compile_definitions(linx_display PRIVATE ENABLE_SSD1306=1)
endif()

# 导出头文件
target_include_directories(linx_display PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/display>
)
```

## 第三方库管理

### 1. 第三方库配置 (`third/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(linx_third_party)

# Mongoose HTTP库
add_subdirectory(mongoose)

# Opus音频编解码器
if(ENABLE_OPUS_CODEC)
    add_subdirectory(opus)
endif()

# LVGL图形库
if(ENABLE_LVGL)
    add_subdirectory(liblvgl)
endif()

# 字体资源
add_subdirectory(fonts)

# 创建第三方库集合
add_library(linx_third_party INTERFACE)

# 链接所有第三方库
target_link_libraries(linx_third_party INTERFACE
    mongoose
)

if(ENABLE_OPUS_CODEC)
    target_link_libraries(linx_third_party INTERFACE opus)
endif()

if(ENABLE_LVGL)
    target_link_libraries(linx_third_party INTERFACE lvgl)
endif()

target_link_libraries(linx_third_party INTERFACE linx_fonts)
```

### 2. Mongoose配置 (`third/mongoose/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(mongoose)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 源文件
set(MONGOOSE_SOURCES
    mongoose.c
)

# 创建Mongoose库
add_library(mongoose STATIC ${MONGOOSE_SOURCES})

# 编译定义
target_compile_definitions(mongoose PUBLIC
    MG_ENABLE_PACKED_FS=1
    MG_ENABLE_LINES=1
)

# 平台特定配置
if(PLATFORM_ESP32)
    target_compile_definitions(mongoose PUBLIC
        MG_ARCH=MG_ARCH_ESP32
        MG_ENABLE_TCPIP=1
        MG_ENABLE_DRIVER_ESP32=1
    )
elseif(PLATFORM_MACOS OR PLATFORM_LINUX)
    target_compile_definitions(mongoose PUBLIC
        MG_ARCH=MG_ARCH_UNIX
        MG_ENABLE_SOCKET=1
    )
endif()

# 导出头文件
target_include_directories(mongoose PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/third/mongoose>
)
```

## 构建选项和配置

### 1. 构建选项定义
```cmake
# 功能选项
option(ENABLE_AUDIO "Enable audio support" ON)
option(ENABLE_DISPLAY "Enable display support" ON)
option(ENABLE_CAMERA "Enable camera support" ON)
option(ENABLE_NETWORK "Enable network support" ON)
option(ENABLE_GPIO "Enable GPIO support" ON)

# 编解码器选项
option(ENABLE_OPUS_CODEC "Enable Opus codec" ON)
option(ENABLE_PCM_CODEC "Enable PCM codec" ON)

# 显示选项
option(ENABLE_LVGL "Enable LVGL graphics library" ON)
option(ENABLE_SSD1306 "Enable SSD1306 OLED support" ON)

# 网络选项
option(ENABLE_WIFI "Enable WiFi support" ON)
option(ENABLE_ETHERNET "Enable Ethernet support" OFF)

# 调试选项
option(ENABLE_DEBUG_LOG "Enable debug logging" OFF)
option(ENABLE_MEMORY_DEBUG "Enable memory debugging" OFF)
option(ENABLE_PROFILING "Enable profiling" OFF)

# 测试选项
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
```

### 2. 配置文件生成
```cmake
# 生成配置头文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/linx_config.h
    @ONLY
)

# 配置模板文件 (cmake/config.h.in)
```

```c
#ifndef LINX_CONFIG_H
#define LINX_CONFIG_H

// 版本信息
#define LINX_SDK_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define LINX_SDK_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define LINX_SDK_VERSION_PATCH @PROJECT_VERSION_PATCH@
#define LINX_SDK_VERSION_STRING "@PROJECT_VERSION@"

// 平台定义
#cmakedefine PLATFORM_ESP32
#cmakedefine PLATFORM_MACOS
#cmakedefine PLATFORM_LINUX
#cmakedefine PLATFORM_WINDOWS

// 功能开关
#cmakedefine ENABLE_AUDIO
#cmakedefine ENABLE_DISPLAY
#cmakedefine ENABLE_CAMERA
#cmakedefine ENABLE_NETWORK
#cmakedefine ENABLE_GPIO

// 编解码器支持
#cmakedefine ENABLE_OPUS_CODEC
#cmakedefine ENABLE_PCM_CODEC

// 显示支持
#cmakedefine ENABLE_LVGL
#cmakedefine ENABLE_SSD1306

// 网络支持
#cmakedefine ENABLE_WIFI
#cmakedefine ENABLE_ETHERNET

// 调试选项
#cmakedefine ENABLE_DEBUG_LOG
#cmakedefine ENABLE_MEMORY_DEBUG
#cmakedefine ENABLE_PROFILING

#endif // LINX_CONFIG_H
```

## 测试集成

### 1. 测试框架配置
```cmake
# 测试配置 (仅在BUILD_TESTS=ON时)
if(BUILD_TESTS)
    enable_testing()
    
    # 查找测试框架
    find_package(GTest REQUIRED)
    
    # 添加测试目录
    add_subdirectory(tests)
    
    # 创建测试目标
    add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS all_tests
    )
endif()
```

### 2. 单元测试配置 (`tests/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(linx_tests)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${GTEST_INCLUDE_DIRS}
)

# 测试源文件
set(TEST_SOURCES
    test_audio_interface.cpp
    test_display_interface.cpp
    test_camera_interface.cpp
    test_network_interface.cpp
    test_gpio_interface.cpp
    test_linx_sdk.cpp
    test_common_utils.cpp
)

# 创建测试可执行文件
add_executable(linx_unit_tests ${TEST_SOURCES})

# 链接库
target_link_libraries(linx_unit_tests
    linx_sdk_static
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# 添加测试
add_test(NAME LinxUnitTests COMMAND linx_unit_tests)

# 代码覆盖率
if(ENABLE_COVERAGE)
    target_compile_options(linx_unit_tests PRIVATE --coverage)
    target_link_options(linx_unit_tests PRIVATE --coverage)
endif()
```

## 打包和部署

### 1. 安装配置
```cmake
# 安装头文件
install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 安装库文件
install(TARGETS linx_sdk_static
    EXPORT LinxSDKTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装CMake配置文件
install(EXPORT LinxSDKTargets
    FILE LinxSDKTargets.cmake
    NAMESPACE LinxSDK::
    DESTINATION lib/cmake/LinxSDK
)

# 生成配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    LinxSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    cmake/LinxSDKConfig.cmake.in
    LinxSDKConfig.cmake
    INSTALL_DESTINATION lib/cmake/LinxSDK
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LinxSDKConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LinxSDKConfigVersion.cmake
    DESTINATION lib/cmake/LinxSDK
)
```

### 2. 打包脚本 (`cmake/scripts/package.cmake`)
```cmake
# CPack配置
set(CPACK_PACKAGE_NAME "LinxSDK")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LinX OS SDK")
set(CPACK_PACKAGE_VENDOR "LinX Technologies")
set(CPACK_PACKAGE_CONTACT "support@linx.com")

# 平台特定打包
if(PLATFORM_ESP32)
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "linx-sdk-${PROJECT_VERSION}-esp32")
elseif(PLATFORM_MACOS)
    set(CPACK_GENERATOR "TGZ;ZIP")
    set(CPACK_PACKAGE_FILE_NAME "linx-sdk-${PROJECT_VERSION}-macos")
elseif(PLATFORM_LINUX)
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_PACKAGE_FILE_NAME "linx-sdk-${PROJECT_VERSION}-linux")
endif()

include(CPack)
```

## 开发工具集成

### 1. IDE配置生成
```cmake
# 生成compile_commands.json (用于IDE和静态分析工具)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# VS Code配置
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.vscode)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vscode/c_cpp_properties.json.in
        ${CMAKE_CURRENT_SOURCE_DIR}/.vscode/c_cpp_properties.json
        @ONLY
    )
endif()
```

### 2. 静态分析集成
```cmake
# Clang-tidy
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE AND ENABLE_STATIC_ANALYSIS)
    set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY_EXE})
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
endif()

# Cppcheck
find_program(CPPCHECK_EXE NAMES "cppcheck")
if(CPPCHECK_EXE AND ENABLE_STATIC_ANALYSIS)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXE}
        --enable=all
        --std=c99
        --verbose
        --quiet
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
    )
endif()
```

## 构建脚本和自动化

### 1. 构建脚本 (`scripts/build.sh`)
```bash
#!/bin/bash

# LinX SDK构建脚本

set -e

# 默认配置
BUILD_TYPE="Release"
PLATFORM=""
ENABLE_TESTS="OFF"
ENABLE_EXAMPLES="OFF"
BUILD_DIR="build"
INSTALL_PREFIX=""

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --enable-tests)
            ENABLE_TESTS="ON"
            shift
            ;;
        --enable-examples)
            ENABLE_EXAMPLES="ON"
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --install-prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --platform PLATFORM     Target platform (esp32, macos, linux)"
            echo "  --build-type TYPE        Build type (Debug, Release)"
            echo "  --enable-tests           Enable unit tests"
            echo "  --enable-examples        Enable examples"
            echo "  --build-dir DIR          Build directory"
            echo "  --install-prefix PREFIX  Install prefix"
            echo "  --help                   Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# 平台检测
if [[ -z "$PLATFORM" ]]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
    else
        echo "Error: Cannot detect platform. Please specify --platform"
        exit 1
    fi
fi

echo "Building LinX SDK for $PLATFORM ($BUILD_TYPE)"

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# CMake配置
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DBUILD_TESTS="$ENABLE_TESTS"
    -DBUILD_EXAMPLES="$ENABLE_EXAMPLES"
)

if [[ -n "$INSTALL_PREFIX" ]]; then
    CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX")
fi

# 平台特定配置
case $PLATFORM in
    esp32)
        if [[ -z "$IDF_PATH" ]]; then
            echo "Error: IDF_PATH not set for ESP32 build"
            exit 1
        fi
        CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$IDF_PATH/tools/cmake/toolchain-esp32.cmake")
        ;;
    macos)
        CMAKE_ARGS+=(-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15)
        ;;
    linux)
        # Linux特定配置
        ;;
esac

# 运行CMake
cmake "${CMAKE_ARGS[@]}" ..

# 构建
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 运行测试
if [[ "$ENABLE_TESTS" == "ON" ]]; then
    echo "Running tests..."
    ctest --verbose
fi

echo "Build completed successfully!"
```

### 2. CI/CD配置 (`.github/workflows/build.yml`)
```yaml
name: Build and Test

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-macos:
    runs-on: macos-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        brew install cmake ninja
    
    - name: Build
      run: |
        ./scripts/build.sh --platform macos --enable-tests
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: linx-sdk-macos
        path: build/

  build-linux:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build libasound2-dev
    
    - name: Build
      run: |
        ./scripts/build.sh --platform linux --enable-tests
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: linx-sdk-linux
        path: build/

  build-esp32:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup ESP-IDF
      uses: espressif/esp-idf-action@v1
      with:
        esp-idf-version: v5.0
    
    - name: Build
      run: |
        export IDF_PATH=$GITHUB_WORKSPACE/esp-idf
        ./scripts/build.sh --platform esp32
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: linx-sdk-esp32
        path: build/
```

## 性能优化

### 1. 编译优化
```cmake
# 编译器优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # 通用优化
    add_compile_options(-O3 -DNDEBUG)
    
    # 平台特定优化
    if(PLATFORM_ESP32)
        add_compile_options(-Os -ffunction-sections -fdata-sections)
        add_link_options(-Wl,--gc-sections)
    elseif(PLATFORM_MACOS OR PLATFORM_LINUX)
        add_compile_options(-march=native -mtune=native)
    endif()
    
    # LTO (Link Time Optimization)
    if(ENABLE_LTO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
```

### 2. 并行构建
```cmake
# 并行构建配置
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
    message(STATUS "Building with ${N} parallel jobs")
endif()

# Ninja生成器优化
if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_JOB_POOLS compile=4 link=2)
    set(CMAKE_JOB_POOL_COMPILE compile)
    set(CMAKE_JOB_POOL_LINK link)
endif()
```

## 总结

这个构建系统设计提供了：

1. **跨平台支持**: 统一的构建接口，支持ESP32、macOS、Linux等平台
2. **模块化构建**: 独立的模块构建，灵活的依赖管理
3. **配置灵活性**: 丰富的构建选项和平台特定配置
4. **开发效率**: 快速构建、测试集成、IDE支持
5. **自动化**: CI/CD集成、自动化测试和部署
6. **性能优化**: 编译优化、并行构建、LTO支持
7. **易于维护**: 清晰的结构、详细的文档、标准化的流程

通过这个构建系统，开发者可以轻松地在不同平台上构建、测试和部署LinX OS SDK。