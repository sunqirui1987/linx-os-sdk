# CMakeLists.txt for common/std test

cmake_minimum_required(VERSION 3.10)

project(vector_test)

# 设置C99标准
set(CMAKE_C_STANDARD 99)

# 源文件路径（相对于上级目录）
set(STD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 源文件
set(LIB_SOURCES
    ${STD_DIR}/vector.c
)

# 创建静态库
add_library(linx_std STATIC ${LIB_SOURCES})

# 设置包含目录
target_include_directories(linx_std PUBLIC ${STD_DIR})

# 设置编译选项
target_compile_options(linx_std PRIVATE
    -Wall
    -Wextra
    -std=c99
)

# 创建测试程序
add_executable(vector_example vector_example.c)
target_link_libraries(vector_example linx_std)
target_include_directories(vector_example PRIVATE ${STD_DIR})
target_compile_options(vector_example PRIVATE
    -Wall
    -Wextra
    -std=c99
)

# 创建简单测试程序
add_executable(simple_test simple_test.c)
target_link_libraries(simple_test linx_std)
target_include_directories(simple_test PRIVATE ${STD_DIR})
target_compile_options(simple_test PRIVATE
    -Wall
    -Wextra
    -std=c99
)

# 创建STD库测试程序
add_executable(std_test std_test.c)
target_link_libraries(std_test linx_std)
target_include_directories(std_test PRIVATE ${STD_DIR})
target_compile_options(std_test PRIVATE
    -Wall
    -Wextra
    -std=c99
)