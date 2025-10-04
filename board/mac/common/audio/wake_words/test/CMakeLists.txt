cmake_minimum_required(VERSION 3.10)
project(WakeWordPorcupine)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# 检查是否为Mac平台
if(NOT APPLE)
    message(FATAL_ERROR "This CMakeLists.txt is designed for macOS only")
endif()

# 查找必要的库
find_package(PkgConfig REQUIRED)

# 查找Porcupine库
find_path(PORCUPINE_INCLUDE_DIR
    NAMES pv_porcupine.h
    PATHS
        /usr/local/include/picovoice
        /opt/homebrew/include/picovoice
        /usr/include/picovoice
    DOC "Porcupine include directory"
)

find_library(PORCUPINE_LIBRARY
    NAMES pv_porcupine libpv_porcupine
    PATHS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
    DOC "Porcupine library"
)

if(NOT PORCUPINE_INCLUDE_DIR OR NOT PORCUPINE_LIBRARY)
    message(FATAL_ERROR "Porcupine library not found. Please install it using: brew install picovoice-porcupine")
endif()

message(STATUS "Found Porcupine include: ${PORCUPINE_INCLUDE_DIR}")
message(STATUS "Found Porcupine library: ${PORCUPINE_LIBRARY}")

# 查找pthread库
find_package(Threads REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
    ${PORCUPINE_INCLUDE_DIR}
)

# 源文件
set(WAKE_WORD_SOURCES
    wake_word_porcupine.c
)

set(WAKE_WORD_HEADERS
    wake_word_porcupine.h
    ../../wake_word_interface.h
)

# 创建静态库
add_library(wake_word_porcupine STATIC ${WAKE_WORD_SOURCES})

# 链接库
target_link_libraries(wake_word_porcupine
    ${PORCUPINE_LIBRARY}
    Threads::Threads
)

# 设置库的包含目录
target_include_directories(wake_word_porcupine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PORCUPINE_INCLUDE_DIR}
)

# 创建示例程序
add_executable(wake_word_example wake_word_example.c)
target_link_libraries(wake_word_example
    wake_word_porcupine
    ${PORCUPINE_LIBRARY}
    Threads::Threads
)

# 安装规则
install(TARGETS wake_word_porcupine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${WAKE_WORD_HEADERS}
    DESTINATION include/wake_words
)

install(TARGETS wake_word_example
    RUNTIME DESTINATION bin
)

# 创建pkg-config文件
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/wake_word_porcupine.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wake_word_porcupine.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/wake_word_porcupine.pc"
    DESTINATION lib/pkgconfig
)

# 打印构建信息
message(STATUS "")
message(STATUS "Wake Word Porcupine Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Porcupine include: ${PORCUPINE_INCLUDE_DIR}")
message(STATUS "  Porcupine library: ${PORCUPINE_LIBRARY}")
message(STATUS "")

# 添加自定义目标用于清理
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/Makefile
)

# 添加测试目标
enable_testing()
add_test(NAME wake_word_example_test
    COMMAND wake_word_example --help
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)