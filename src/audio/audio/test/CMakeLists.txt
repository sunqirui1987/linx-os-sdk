# LINX 音频测试 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(linx_audio_test LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# 查找必要的包
find_package(PkgConfig REQUIRED)

# 尝试查找PortAudio
pkg_check_modules(PORTAUDIO portaudio-2.0)

if(PORTAUDIO_FOUND)
    message(STATUS "找到PortAudio: ${PORTAUDIO_VERSION}")
    add_definitions(-DPORTAUDIO_AVAILABLE=1)
else()
    message(WARNING "未找到PortAudio，将使用桩实现")
    add_definitions(-DPORTAUDIO_AVAILABLE=0)
endif()

# 设置包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..                    # audio_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..              # src目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../audio              # src目录
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/log   # linx_log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../board # portaudio_mac.h
)

if(PORTAUDIO_FOUND)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
endif()

# 源文件列表
set(AUDIO_TEST_SOURCES
    audio_test_portaudio.c
    ../audio_interface.c
    ../audio_stub.c
    ../../../common/log/linx_log.c
)

# 如果找到PortAudio，添加PortAudio Mac实现
if(PORTAUDIO_FOUND)
    list(APPEND AUDIO_TEST_SOURCES
        ../../../../board/mac/common/audio/audio/portaudio_mac.c
    )
endif()

# 创建可执行文件
add_executable(audio_test ${AUDIO_TEST_SOURCES})

# 链接库
target_link_libraries(audio_test pthread)

if(PORTAUDIO_FOUND)
    target_link_libraries(audio_test ${PORTAUDIO_LIBRARIES})
    target_link_directories(audio_test PRIVATE ${PORTAUDIO_LIBRARY_DIRS})
    target_compile_options(audio_test PRIVATE ${PORTAUDIO_CFLAGS_OTHER})
endif()

# 设置输出目录
set_target_properties(audio_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS audio_test
    RUNTIME DESTINATION bin
)

# 添加测试
enable_testing()

# 基本测试（不需要交互）
add_test(NAME audio_basic_test 
         COMMAND audio_test
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置测试属性
set_tests_properties(audio_basic_test PROPERTIES
    TIMEOUT 30
    LABELS "audio;basic"
)

# 打印配置信息
message(STATUS "音频测试配置:")
message(STATUS "  C编译器: ${CMAKE_C_COMPILER}")
message(STATUS "  C标准: ${CMAKE_C_STANDARD}")
message(STATUS "  PortAudio支持: ${PORTAUDIO_FOUND}")
if(PORTAUDIO_FOUND)
    message(STATUS "  PortAudio版本: ${PORTAUDIO_VERSION}")
    message(STATUS "  PortAudio库: ${PORTAUDIO_LIBRARIES}")
endif()
message(STATUS "  输出目录: ${CMAKE_BINARY_DIR}/bin")

# 添加自定义目标来运行交互式测试
add_custom_target(audio_interactive_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/audio_test --interactive
    DEPENDS audio_test
    COMMENT "运行交互式音频测试"
    VERBATIM
)

# 添加帮助信息
add_custom_target(audio_test_help
    COMMAND echo "可用的音频测试目标:"
    COMMAND echo "  make audio_test              - 编译音频测试"
    COMMAND echo "  make test                    - 运行基本测试"
    COMMAND echo "  make audio_interactive_test  - 运行交互式测试"
    COMMAND echo "  ./bin/audio_test             - 运行基本测试"
    COMMAND echo "  ./bin/audio_test --interactive - 运行交互式测试"
    VERBATIM
)