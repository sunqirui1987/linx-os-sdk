# Audio Plugins Unit Tests
cmake_minimum_required(VERSION 3.16)

project(linx_audio_plugins_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找GoogleTest
find_package(GTest REQUIRED)
if(NOT GTest_FOUND)
    # 如果系统没有安装GoogleTest，使用FetchContent下载
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50f33f9142fd2d6828d8d66c3b9b4e.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..                    # plugins目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../..                 # audio目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core            # audio/core目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../builtin            # plugins/builtin目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../utils           # audio/utils目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..           # src目录
)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 定义测试源文件
set(TEST_SOURCES
    test_main.cpp
    test_plugin_interface.cpp
    test_gain_plugin.cpp
)

# 定义被测试的源文件
set(PLUGIN_SOURCES
    ../plugin_manager.c
    ../plugin_interface.c
    ../builtin/gain.c
    ../builtin/plugin_registry.c
)

# 创建测试可执行文件
add_executable(linx_audio_plugins_tests
    ${TEST_SOURCES}
    ${PLUGIN_SOURCES}
)

# 链接库
target_link_libraries(linx_audio_plugins_tests
    GTest::gtest
    GTest::gtest_main
    pthread
    m
)

# 如果有其他依赖库，在这里添加
# target_link_libraries(linx_audio_plugins_tests
#     linx_audio_core
#     linx_common
# )

# 启用测试
enable_testing()

# 添加测试
add_test(NAME PluginTestBase COMMAND linx_audio_plugins_tests --gtest_filter="PluginTestBase*")
add_test(NAME GainPluginTest COMMAND linx_audio_plugins_tests --gtest_filter="GainPluginTest*")

# 设置测试属性
set_tests_properties(PluginTestBase PROPERTIES TIMEOUT 30)
set_tests_properties(GainPluginTest PROPERTIES TIMEOUT 30)

# 生成测试报告
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(linx_audio_plugins_tests PRIVATE DEBUG_BUILD)
endif()

# 代码覆盖率支持（可选）
if(ENABLE_COVERAGE)
    target_compile_options(linx_audio_plugins_tests PRIVATE --coverage)
    target_link_options(linx_audio_plugins_tests PRIVATE --coverage)
endif()